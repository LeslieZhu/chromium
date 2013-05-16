// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/printer_job_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/file_util.h"
#include "base/json/json_reader.h"
#include "base/md5.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/cloud_print/cloud_print_constants.h"
#include "chrome/common/cloud_print/cloud_print_helpers.h"
#include "chrome/service/cloud_print/cloud_print_helpers.h"
#include "chrome/service/cloud_print/job_status_updater.h"
#include "googleurl/src/gurl.h"
#include "grit/generated_resources.h"
#include "net/base/mime_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "printing/backend/print_backend.h"
#include "ui/base/l10n/l10n_util.h"

namespace cloud_print {

PrinterJobHandler::PrinterJobHandler(
    const printing::PrinterBasicInfo& printer_info,
    const PrinterInfoFromCloud& printer_info_cloud,
    const GURL& cloud_print_server_url,
    PrintSystem* print_system,
    Delegate* delegate)
    : print_system_(print_system),
      printer_info_(printer_info),
      printer_info_cloud_(printer_info_cloud),
      cloud_print_server_url_(cloud_print_server_url),
      delegate_(delegate),
      local_job_id_(-1),
      next_json_data_handler_(NULL),
      next_data_handler_(NULL),
      server_error_count_(0),
      print_thread_("Chrome_CloudPrintJobPrintThread"),
      job_handler_message_loop_proxy_(
          base::MessageLoopProxy::current()),
      shutting_down_(false),
      job_check_pending_(false),
      printer_update_pending_(true),
      task_in_progress_(false),
      weak_ptr_factory_(this) {
}

bool PrinterJobHandler::Initialize() {
  if (!print_system_->IsValidPrinter(printer_info_.printer_name))
    return false;

  printer_watcher_ = print_system_->CreatePrinterWatcher(
      printer_info_.printer_name);
  printer_watcher_->StartWatching(this);
  CheckForJobs(kJobFetchReasonStartup);
  return true;
}

std::string PrinterJobHandler::GetPrinterName() const {
  return printer_info_.printer_name;
}

void PrinterJobHandler::CheckForJobs(const std::string& reason) {
  VLOG(1) << "CP_CONNECTOR: Checking for jobs"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", reason: " << reason
          << ", task in progress: " << task_in_progress_;
  job_fetch_reason_ = reason;
  job_check_pending_ = true;
  if (!task_in_progress_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Start, this));
  }
}

void PrinterJobHandler::Shutdown() {
  VLOG(1) << "CP_CONNECTOR: Shutting down printer job handler"
          << ", printer id: " << printer_info_cloud_.printer_id;
  Reset();
  shutting_down_ = true;
  while (!job_status_updater_list_.empty()) {
    // Calling Stop() will cause the OnJobCompleted to be called which will
    // remove the updater object from the list.
    job_status_updater_list_.front()->Stop();
  }
}

// CloudPrintURLFetcher::Delegate implementation.
CloudPrintURLFetcher::ResponseAction PrinterJobHandler::HandleRawResponse(
    const net::URLFetcher* source,
    const GURL& url,
    const net::URLRequestStatus& status,
    int response_code,
    const net::ResponseCookies& cookies,
    const std::string& data) {
  // 415 (Unsupported media type) error while fetching data from the server
  // means data conversion error. Stop fetching process and mark job as error.
  if (next_data_handler_ == (&PrinterJobHandler::HandlePrintDataResponse) &&
      response_code == net::HTTP_UNSUPPORTED_MEDIA_TYPE) {
    VLOG(1) << "CP_CONNECTOR: Job failed (unsupported media type)";
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&PrinterJobHandler::JobFailed, this, JOB_DOWNLOAD_FAILED));
    return CloudPrintURLFetcher::STOP_PROCESSING;
  }
  return CloudPrintURLFetcher::CONTINUE_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction PrinterJobHandler::HandleRawData(
    const net::URLFetcher* source,
    const GURL& url,
    const std::string& data) {
  if (!next_data_handler_)
    return CloudPrintURLFetcher::CONTINUE_PROCESSING;
  return (this->*next_data_handler_)(source, url, data);
}

CloudPrintURLFetcher::ResponseAction PrinterJobHandler::HandleJSONData(
    const net::URLFetcher* source,
    const GURL& url,
    DictionaryValue* json_data,
    bool succeeded) {
  DCHECK(next_json_data_handler_);
  return (this->*next_json_data_handler_)(source, url, json_data, succeeded);
}

// Mark the job fetch as failed and check if other jobs can be printed
void PrinterJobHandler::OnRequestGiveUp() {
  if (job_queue_handler_.JobFetchFailed(job_details_.job_id_)) {
    VLOG(1) << "CP_CONNECTOR: Job failed to load (scheduling retry)";
    CheckForJobs(kJobFetchReasonFailure);
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  } else {
    VLOG(1) << "CP_CONNECTOR: Job failed (giving up after " <<
        kNumRetriesBeforeAbandonJob << " retries)";
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&PrinterJobHandler::JobFailed, this, JOB_DOWNLOAD_FAILED));
  }
}

CloudPrintURLFetcher::ResponseAction PrinterJobHandler::OnRequestAuthError() {
  // We got an Auth error and have no idea how long it will take to refresh
  // auth information (may take forever). We'll drop current request and
  // propagate this error to the upper level. After auth issues will be
  // resolved, GCP connector will restart.
  OnAuthError();
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

std::string PrinterJobHandler::GetAuthHeader() {
  return GetCloudPrintAuthHeaderFromStore();
}

// JobStatusUpdater::Delegate implementation
bool PrinterJobHandler::OnJobCompleted(JobStatusUpdater* updater) {
  bool ret = false;

  job_queue_handler_.JobDone(job_details_.job_id_);

  for (JobStatusUpdaterList::iterator index = job_status_updater_list_.begin();
       index != job_status_updater_list_.end(); index++) {
    if (index->get() == updater) {
      job_status_updater_list_.erase(index);
      ret = true;
      break;
    }
  }
  return ret;
}

void PrinterJobHandler::OnAuthError() {
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  if (delegate_)
    delegate_->OnAuthError();
}

void PrinterJobHandler::OnPrinterDeleted() {
  if (delegate_)
    delegate_->OnPrinterDeleted(printer_info_cloud_.printer_id);
}

void PrinterJobHandler::OnPrinterChanged() {
  printer_update_pending_ = true;
  if (!task_in_progress_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Start, this));
  }
}

void PrinterJobHandler::OnJobChanged() {
  // Some job on the printer changed. Loop through all our JobStatusUpdaters
  // and have them check for updates.
  for (JobStatusUpdaterList::iterator index = job_status_updater_list_.begin();
       index != job_status_updater_list_.end(); index++) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&JobStatusUpdater::UpdateStatus, index->get()));
  }
}

void PrinterJobHandler::OnJobSpoolSucceeded(const PlatformJobId& job_id) {
  DCHECK(base::MessageLoop::current() == print_thread_.message_loop());
  job_spooler_ = NULL;
  job_handler_message_loop_proxy_->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::JobSpooled, this, job_id));
}

void PrinterJobHandler::OnJobSpoolFailed() {
  DCHECK(base::MessageLoop::current() == print_thread_.message_loop());
  job_spooler_ = NULL;
  VLOG(1) << "CP_CONNECTOR: Job failed (spool failed)";
  job_handler_message_loop_proxy_->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::JobFailed, this, PRINT_FAILED));
}

PrinterJobHandler::~PrinterJobHandler() {
  if (printer_watcher_)
    printer_watcher_->StopWatching();
}

// Begin Response handlers
CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandlePrinterUpdateResponse(
    const net::URLFetcher* source,
    const GURL& url,
    DictionaryValue* json_data,
    bool succeeded) {
  VLOG(1) << "CP_CONNECTOR: Handling printer update response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  // We are done here. Go to the Stop state
  VLOG(1) << "CP_CONNECTOR: Stopping printer job handler"
          << ", printer id: " << printer_info_cloud_.printer_id;
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandleJobMetadataResponse(
    const net::URLFetcher* source,
    const GURL& url,
    DictionaryValue* json_data,
    bool succeeded) {
  VLOG(1) << "CP_CONNECTOR: Handling job metadata response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  bool job_available = false;
  if (succeeded) {
    std::vector<JobDetails> jobs;
    job_queue_handler_.GetJobsFromQueue(json_data, &jobs);
    if (!jobs.empty()) {
      if (jobs[0].time_remaining_ == base::TimeDelta()) {
        job_available = true;
        job_details_ = jobs[0];

        SetNextDataHandler(&PrinterJobHandler::HandlePrintTicketResponse);
        request_ = CloudPrintURLFetcher::Create();
        request_->StartGetRequest(GURL(job_details_.print_ticket_url_),
                                  this,
                                  kJobDataMaxRetryCount,
                                  std::string());
      } else {
        job_available = false;
        base::MessageLoop::current()->PostDelayedTask(
            FROM_HERE,
            base::Bind(&PrinterJobHandler::RunScheduledJobCheck, this),
            jobs[0].time_remaining_);
      }
    }
  }

  if (!job_available) {
    // If no jobs are available, go to the Stop state.
    VLOG(1) << "CP_CONNECTOR: Stopping printer job handler"
            << ", printer id: " << printer_info_cloud_.printer_id;
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  }
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandlePrintTicketResponse(const net::URLFetcher* source,
                                             const GURL& url,
                                             const std::string& data) {
  VLOG(1) << "CP_CONNECTOR: Handling print ticket response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  if (print_system_->ValidatePrintTicket(printer_info_.printer_name, data)) {
    job_details_.print_ticket_ = data;
    SetNextDataHandler(&PrinterJobHandler::HandlePrintDataResponse);
    request_ = CloudPrintURLFetcher::Create();
    std::string accept_headers = "Accept: ";
    accept_headers += print_system_->GetSupportedMimeTypes();
    request_->StartGetRequest(GURL(job_details_.print_data_url_),
                              this,
                              kJobDataMaxRetryCount,
                              accept_headers);
  } else {
    // The print ticket was not valid. We are done here.
    FailedFetchingJobData();
  }
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandlePrintDataResponse(const net::URLFetcher* source,
                                           const GURL& url,
                                           const std::string& data) {
  VLOG(1) << "CP_CONNECTOR: Handling print data response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  base::Closure next_task;
  if (file_util::CreateTemporaryFile(&job_details_.print_data_file_path_)) {
    int ret = file_util::WriteFile(job_details_.print_data_file_path_,
                                   data.c_str(),
                                   data.length());
    source->GetResponseHeaders()->GetMimeType(
        &job_details_.print_data_mime_type_);
    DCHECK(ret == static_cast<int>(data.length()));
    if (ret == static_cast<int>(data.length())) {
      next_task = base::Bind(&PrinterJobHandler::StartPrinting, this);
    }
  }
  // If there was no task allocated above, then there was an error in
  // saving the print data, bail out here.
  if (next_task.is_null()) {
    VLOG(1) << "CP_CONNECTOR: Error saving print data"
            << ", printer id: " << printer_info_cloud_.printer_id;
    next_task = base::Bind(&PrinterJobHandler::JobFailed, this,
                           JOB_DOWNLOAD_FAILED);
  }
  base::MessageLoop::current()->PostTask(FROM_HERE, next_task);
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandleSuccessStatusUpdateResponse(
    const net::URLFetcher* source,
    const GURL& url,
    DictionaryValue* json_data,
    bool succeeded) {
  VLOG(1) << "CP_CONNECTOR: Handling success status update response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  // The print job has been spooled locally. We now need to create an object
  // that monitors the status of the job and updates the server.
  scoped_refptr<JobStatusUpdater> job_status_updater(
      new JobStatusUpdater(printer_info_.printer_name, job_details_.job_id_,
                           local_job_id_, cloud_print_server_url_,
                           print_system_.get(), this));
  job_status_updater_list_.push_back(job_status_updater);
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&JobStatusUpdater::UpdateStatus, job_status_updater.get()));
  if (succeeded) {
    // Since we just printed successfully, we want to look for more jobs.
    CheckForJobs(kJobFetchReasonQueryMore);
  }
  VLOG(1) << "CP_CONNECTOR: Stopping printer job handler"
          << ", printer id: " << printer_info_cloud_.printer_id;
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

CloudPrintURLFetcher::ResponseAction
PrinterJobHandler::HandleFailureStatusUpdateResponse(
    const net::URLFetcher* source,
    const GURL& url,
    DictionaryValue* json_data,
    bool succeeded) {
  VLOG(1) << "CP_CONNECTOR: Handling failure status update response"
          << ", printer id: " << printer_info_cloud_.printer_id;
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  return CloudPrintURLFetcher::STOP_PROCESSING;
}

void PrinterJobHandler::Start() {
  VLOG(1) << "CP_CONNECTOR: Starting printer job handler"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", task in progress: " << task_in_progress_;
  if (task_in_progress_) {
    // Multiple Starts can get posted because of multiple notifications
    // We want to ignore the other ones that happen when a task is in progress.
    return;
  }
  Reset();
  if (!shutting_down_) {
    // Check if we have work to do.
    if (HavePendingTasks()) {
      if (!task_in_progress_ && printer_update_pending_) {
        printer_update_pending_ = false;
        task_in_progress_ = UpdatePrinterInfo();
        VLOG(1) << "CP_CONNECTOR: Changed task in progress"
                << ", printer id: " << printer_info_cloud_.printer_id
                << ", task in progress: " << task_in_progress_;
      }
      if (!task_in_progress_ && job_check_pending_) {
        task_in_progress_ = true;
        VLOG(1) << "CP_CONNECTOR: Changed task in progress"
                ", printer id: " << printer_info_cloud_.printer_id
                << ", task in progress: " << task_in_progress_;
        job_check_pending_ = false;
        // We need to fetch any pending jobs for this printer
        SetNextJSONHandler(&PrinterJobHandler::HandleJobMetadataResponse);
        request_ = CloudPrintURLFetcher::Create();
        request_->StartGetRequest(
            GetUrlForJobFetch(
                cloud_print_server_url_, printer_info_cloud_.printer_id,
                job_fetch_reason_),
            this,
            kCloudPrintAPIMaxRetryCount,
            std::string());
        last_job_fetch_time_ = base::TimeTicks::Now();
        VLOG(1) << "CP_CONNECTOR: Last job fetch time"
                << ", printer name: " << printer_info_.printer_name.c_str()
                << ", timestamp: " << last_job_fetch_time_.ToInternalValue();
        job_fetch_reason_.clear();
      }
    }
  }
}

void PrinterJobHandler::Stop() {
  VLOG(1) << "CP_CONNECTOR: Stopping printer job handler"
          << ", printer id: " << printer_info_cloud_.printer_id;
  task_in_progress_ = false;
  VLOG(1) << "CP_CONNECTOR: Changed task in progress"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", task in progress: " << task_in_progress_;
  Reset();
  if (HavePendingTasks()) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Start, this));
  }
}

void PrinterJobHandler::StartPrinting() {
  VLOG(1) << "CP_CONNECTOR: Starting printing"
          << ", printer id: " << printer_info_cloud_.printer_id;
  // We are done with the request object for now.
  request_ = NULL;
  if (!shutting_down_) {
    if (!print_thread_.Start()) {
      VLOG(1) << "CP_CONNECTOR: Failed to start print thread"
              << ", printer id: " << printer_info_cloud_.printer_id;
      JobFailed(PRINT_FAILED);
    } else {
      print_thread_.message_loop()->PostTask(
          FROM_HERE, base::Bind(&PrinterJobHandler::DoPrint, this, job_details_,
                                printer_info_.printer_name));
    }
  }
}

void PrinterJobHandler::Reset() {
  job_details_.Clear();
  request_ = NULL;
  print_thread_.Stop();
}

void PrinterJobHandler::UpdateJobStatus(PrintJobStatus status,
                                        PrintJobError error) {
  VLOG(1) << "CP_CONNECTOR: Updating job status"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", job id: " << job_details_.job_id_
          << ", job status: " << status;
  if (shutting_down_) {
    VLOG(1) << "CP_CONNECTOR: Job status update aborted (shutting down)"
            << ", printer id: " << printer_info_cloud_.printer_id
            << ", job id: " << job_details_.job_id_;
    return;
  }
  if (job_details_.job_id_.empty()) {
    VLOG(1) << "CP_CONNECTOR: Job status update aborted (empty job id)"
            << ", printer id: " << printer_info_cloud_.printer_id;
    return;
  }

  if (error == SUCCESS) {
    SetNextJSONHandler(
        &PrinterJobHandler::HandleSuccessStatusUpdateResponse);
  } else {
    SetNextJSONHandler(
        &PrinterJobHandler::HandleFailureStatusUpdateResponse);
  }
  request_ = CloudPrintURLFetcher::Create();
  request_->StartGetRequest(GetUrlForJobStatusUpdate(cloud_print_server_url_,
                                                     job_details_.job_id_,
                                                     status),
      this,
      kCloudPrintAPIMaxRetryCount,
      std::string());
}

void PrinterJobHandler::RunScheduledJobCheck() {
  CheckForJobs(kJobFetchReasonRetry);
}

void PrinterJobHandler::SetNextJSONHandler(JSONDataHandler handler) {
  next_json_data_handler_ = handler;
  next_data_handler_ = NULL;
}

void PrinterJobHandler::SetNextDataHandler(DataHandler handler) {
  next_data_handler_ = handler;
  next_json_data_handler_ = NULL;
}

void PrinterJobHandler::JobFailed(PrintJobError error) {
  VLOG(1) << "CP_CONNECTOR: Job failed"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", job id: " << job_details_.job_id_
          << ", error: " << error;
  if (!shutting_down_) {
    UpdateJobStatus(PRINT_JOB_STATUS_ERROR, error);
    // This job failed, but others may be pending.  Schedule a check.
    job_check_pending_ = true;
    job_fetch_reason_ = kJobFetchReasonFailure;
  }
}

void PrinterJobHandler::JobSpooled(PlatformJobId local_job_id) {
  VLOG(1) << "CP_CONNECTOR: Job spooled"
          << ", printer id: " << printer_info_cloud_.printer_id
          << ", job id: " << local_job_id;
  if (!shutting_down_) {
    local_job_id_ = local_job_id;
    UpdateJobStatus(PRINT_JOB_STATUS_IN_PROGRESS, SUCCESS);
    print_thread_.Stop();
  }
}

bool PrinterJobHandler::UpdatePrinterInfo() {
  if (!printer_watcher_) {
    LOG(ERROR) << "CP_CONNECTOR: Printer watcher is missing."
               << " Check printer server url for printer id: "
               << printer_info_cloud_.printer_id;
    return false;
  }

  VLOG(1) << "CP_CONNECTOR: Updating printer info"
          << ", printer id: " << printer_info_cloud_.printer_id;
  // We need to update the parts of the printer info that have changed
  // (could be printer name, description, status or capabilities).
  // First asynchronously fetch the capabilities.
  printing::PrinterBasicInfo printer_info;
  printer_watcher_->GetCurrentPrinterInfo(&printer_info);

  // Asynchronously fetch the printer caps and defaults. The story will
  // continue in OnReceivePrinterCaps.
  print_system_->GetPrinterCapsAndDefaults(
      printer_info.printer_name.c_str(),
      base::Bind(&PrinterJobHandler::OnReceivePrinterCaps,
                 weak_ptr_factory_.GetWeakPtr()));

  // While we are waiting for the data, pretend we have work to do and return
  // true.
  return true;
}

bool PrinterJobHandler::HavePendingTasks() {
  return (job_check_pending_ || printer_update_pending_);
}

void PrinterJobHandler::FailedFetchingJobData() {
  if (!shutting_down_) {
    LOG(ERROR) << "CP_CONNECTOR: Failed fetching job data"
               << ", printer name: " << printer_info_.printer_name
               << ", job id: " << job_details_.job_id_;
    JobFailed(INVALID_JOB_DATA);
  }
}

void PrinterJobHandler::OnReceivePrinterCaps(
    bool succeeded,
    const std::string& printer_name,
    const printing::PrinterCapsAndDefaults& caps_and_defaults) {
  printing::PrinterBasicInfo printer_info;
  if (printer_watcher_)
    printer_watcher_->GetCurrentPrinterInfo(&printer_info);

  std::string post_data;
  std::string mime_boundary;
  CreateMimeBoundaryForUpload(&mime_boundary);

  if (succeeded) {
    std::string caps_hash =
        base::MD5String(caps_and_defaults.printer_capabilities);
    if (caps_hash != printer_info_cloud_.caps_hash) {
      // Hashes don't match, we need to upload new capabilities (the defaults
      // go for free along with the capabilities)
      printer_info_cloud_.caps_hash = caps_hash;
      net::AddMultipartValueForUpload(kPrinterCapsValue,
          caps_and_defaults.printer_capabilities, mime_boundary,
          caps_and_defaults.caps_mime_type, &post_data);
      net::AddMultipartValueForUpload(kPrinterDefaultsValue,
          caps_and_defaults.printer_defaults, mime_boundary,
          caps_and_defaults.defaults_mime_type, &post_data);
      net::AddMultipartValueForUpload(kPrinterCapsHashValue,
          caps_hash, mime_boundary, std::string(), &post_data);
    }
  } else {
    LOG(ERROR) << "Failed to get printer caps and defaults"
               << ", printer name: " << printer_name;
  }

  std::string tags_hash = GetHashOfPrinterInfo(printer_info);
  if (tags_hash != printer_info_cloud_.tags_hash) {
    printer_info_cloud_.tags_hash = tags_hash;
    post_data += GetPostDataForPrinterInfo(printer_info, mime_boundary);
    // Remove all the existing proxy tags.
    std::string cp_tag_wildcard(kCloudPrintServiceProxyTagPrefix);
    cp_tag_wildcard += ".*";
    net::AddMultipartValueForUpload(kPrinterRemoveTagValue,
        cp_tag_wildcard, mime_boundary, std::string(), &post_data);
  }

  if (printer_info.printer_name != printer_info_.printer_name) {
    net::AddMultipartValueForUpload(kPrinterNameValue,
        printer_info.printer_name, mime_boundary, std::string(), &post_data);
  }
  if (printer_info.printer_description != printer_info_.printer_description) {
    net::AddMultipartValueForUpload(kPrinterDescValue,
      printer_info.printer_description, mime_boundary,
      std::string(), &post_data);
  }
  if (printer_info.printer_status != printer_info_.printer_status) {
    net::AddMultipartValueForUpload(kPrinterStatusValue,
        base::StringPrintf("%d", printer_info.printer_status), mime_boundary,
        std::string(), &post_data);
  }
  printer_info_ = printer_info;
  if (!post_data.empty()) {
    net::AddMultipartFinalDelimiterForUpload(mime_boundary, &post_data);
    std::string mime_type("multipart/form-data; boundary=");
    mime_type += mime_boundary;
    SetNextJSONHandler(&PrinterJobHandler::HandlePrinterUpdateResponse);
    request_ = CloudPrintURLFetcher::Create();
    request_->StartPostRequest(
        GetUrlForPrinterUpdate(
            cloud_print_server_url_, printer_info_cloud_.printer_id),
        this,
        kCloudPrintAPIMaxRetryCount,
        mime_type,
        post_data,
        std::string());
  } else {
    // We are done here. Go to the Stop state
    VLOG(1) << "CP_CONNECTOR: Stopping printer job handler"
            << ", printer name: " << printer_name;
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&PrinterJobHandler::Stop, this));
  }
}

// The following methods are called on |print_thread_|. It is not safe to
// access any members other than |job_handler_message_loop_proxy_|,
// |job_spooler_| and |print_system_|.
void PrinterJobHandler::DoPrint(const JobDetails& job_details,
                                const std::string& printer_name) {
  job_spooler_ = print_system_->CreateJobSpooler();
  DCHECK(job_spooler_);
  if (!job_spooler_)
    return;
  string16 document_name =
      printing::PrintBackend::SimplifyDocumentTitle(
          UTF8ToUTF16(job_details.job_title_));
  if (document_name.empty()) {
    document_name = printing::PrintBackend::SimplifyDocumentTitle(
        l10n_util::GetStringUTF16(IDS_DEFAULT_PRINT_DOCUMENT_TITLE));
  }
  if (!job_spooler_->Spool(job_details.print_ticket_,
                           job_details.print_data_file_path_,
                           job_details.print_data_mime_type_,
                           printer_name,
                           UTF16ToUTF8(document_name),
                           job_details.tags_,
                           this)) {
    OnJobSpoolFailed();
  }
}

}  // namespace cloud_print
