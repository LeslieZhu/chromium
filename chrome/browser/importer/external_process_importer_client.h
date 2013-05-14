// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/string16.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/importer/importer_data_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/utility_process_host_client.h"

class ExternalProcessImporterHost;
struct ImportedBookmarkEntry;
struct ImportedFaviconUsage;
class InProcessImporterBridge;
class TemplateURL;

namespace content {
struct PasswordForm;
class UtilityProcessHost;
}

// This class is the client for the out of process profile importing.  It
// collects notifications from this process host and feeds data back to the
// importer host, who actually does the writing.
class ExternalProcessImporterClient : public content::UtilityProcessHostClient {
 public:
  ExternalProcessImporterClient(ExternalProcessImporterHost* importer_host,
                                const importer::SourceProfile& source_profile,
                                uint16 items,
                                InProcessImporterBridge* bridge);

  // Launches the task to start the external process.
  void Start();

  // Called by the ExternalProcessImporterHost on import cancel.
  void Cancel();

  // UtilityProcessHostClient implementation:
  virtual void OnProcessCrashed(int exit_code) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Message handlers
  void OnImportStart();
  void OnImportFinished(bool succeeded, const std::string& error_msg);
  void OnImportItemStart(int item);
  void OnImportItemFinished(int item);
  void OnHistoryImportStart(size_t total_history_rows_count);
  void OnHistoryImportGroup(const history::URLRows& history_rows_group,
                            int visit_source);
  void OnHomePageImportReady(const GURL& home_page);
  void OnBookmarksImportStart(const string16& first_folder_name,
                              size_t total_bookmarks_count);
  void OnBookmarksImportGroup(
      const std::vector<ImportedBookmarkEntry>& bookmarks_group);
  void OnFaviconsImportStart(size_t total_favicons_count);
  void OnFaviconsImportGroup(
      const std::vector<ImportedFaviconUsage>& favicons_group);
  void OnPasswordFormImportReady(const content::PasswordForm& form);
  // WARNING: This function takes ownership of (and deletes) the pointers in
  // |template_urls|!
  void OnKeywordsImportReady(const std::vector<TemplateURL*>& template_urls,
                             bool unique_on_host_and_path);

 protected:
  virtual ~ExternalProcessImporterClient();

 private:
  // Notifies the importerhost that import has finished, and calls Release().
  void Cleanup();

  // Cancel import process on IO thread.
  void CancelImportProcessOnIOThread();

  // Report item completely downloaded on IO thread.
  void NotifyItemFinishedOnIOThread(importer::ImportItem import_item);

  // Creates a new UtilityProcessHost, which launches the import process.
  void StartProcessOnIOThread(content::BrowserThread::ID thread_id);

  // These variables store data being collected from the importer until the
  // entire group has been collected and is ready to be written to the profile.
  history::URLRows history_rows_;
  std::vector<ImportedBookmarkEntry> bookmarks_;
  std::vector<ImportedFaviconUsage> favicons_;

  // Usually some variation on IDS_BOOKMARK_GROUP_...; the name of the folder
  // under which imported bookmarks will be placed.
  string16 bookmarks_first_folder_name_;

  // Total number of bookmarks to import.
  size_t total_bookmarks_count_;

  // Total number of history items to import.
  size_t total_history_rows_count_;

  // Total number of favicons to import.
  size_t total_favicons_count_;

  // Notifications received from the ProfileImportProcessHost are passed back
  // to process_importer_host_, which calls the ProfileWriter to record the
  // import data.  When the import process is done, process_importer_host_
  // deletes itself.
  ExternalProcessImporterHost* process_importer_host_;

  // Handles sending messages to the external process.  Deletes itself when
  // the external process dies (see
  // BrowserChildProcessHost::OnChildDisconnected).
  base::WeakPtr<content::UtilityProcessHost> utility_process_host_;

  // Data to be passed from the importer host to the external importer.
  const importer::SourceProfile& source_profile_;
  uint16 items_;

  // Takes import data coming over IPC and delivers it to be written by the
  // ProfileWriter.
  scoped_refptr<InProcessImporterBridge> bridge_;

  // True if import process has been cancelled.
  bool cancelled_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProcessImporterClient);
};

#endif  // CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
