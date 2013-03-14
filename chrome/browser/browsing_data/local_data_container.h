// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_LOCAL_DATA_CONTAINER_H_
#define CHROME_BROWSER_BROWSING_DATA_LOCAL_DATA_CONTAINER_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/string16.h"
#include "chrome/browser/browsing_data/browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_quota_helper.h"
#include "chrome/browser/browsing_data/browsing_data_server_bound_cert_helper.h"
#include "net/ssl/server_bound_cert_store.h"

class BrowsingDataFlashLSOHelper;
class CookiesTreeModel;
class LocalDataContainer;

namespace net {
class CanonicalCookie;
}

// Friendly typedefs for the multiple types of lists used in the model.
namespace {

typedef std::list<net::CanonicalCookie> CookieList;
typedef std::list<BrowsingDataDatabaseHelper::DatabaseInfo> DatabaseInfoList;
typedef std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>
    LocalStorageInfoList;
typedef std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>
    SessionStorageInfoList;
typedef std::list<BrowsingDataIndexedDBHelper::IndexedDBInfo>
    IndexedDBInfoList;
typedef std::list<BrowsingDataFileSystemHelper::FileSystemInfo>
    FileSystemInfoList;
typedef std::list<BrowsingDataQuotaHelper::QuotaInfo> QuotaInfoList;
typedef net::ServerBoundCertStore::ServerBoundCertList ServerBoundCertList;
typedef std::map<GURL, std::list<appcache::AppCacheInfo> > AppCacheInfoMap;
typedef std::vector<std::string> FlashLSODomainList;

} // namespace

// LocalDataContainer ---------------------------------------------------------
// This class is a wrapper around all the BrowsingData*Helper classes. Because
// isolated applications have separate storage, we need different helper
// instances. As such, this class contains the app name and id, along with the
// helpers for all of the data types we need. The browser-wide "app id" will be
// the empty string, as no app can have an empty id.
class LocalDataContainer {
 public:
  LocalDataContainer(
      BrowsingDataCookieHelper* cookie_helper,
      BrowsingDataDatabaseHelper* database_helper,
      BrowsingDataLocalStorageHelper* local_storage_helper,
      BrowsingDataLocalStorageHelper* session_storage_helper,
      BrowsingDataAppCacheHelper* appcache_helper,
      BrowsingDataIndexedDBHelper* indexed_db_helper,
      BrowsingDataFileSystemHelper* file_system_helper,
      BrowsingDataQuotaHelper* quota_helper,
      BrowsingDataServerBoundCertHelper* server_bound_cert_helper,
      BrowsingDataFlashLSOHelper* flash_data_helper);
  virtual ~LocalDataContainer();

  // This method must be called to start the process of fetching the resources.
  // The delegate passed in is called back to deliver the updates.
  void Init(CookiesTreeModel* delegate);

 private:
  friend class CookiesTreeModel;
  friend class CookieTreeAppCacheNode;
  friend class CookieTreeCookieNode;
  friend class CookieTreeDatabaseNode;
  friend class CookieTreeLocalStorageNode;
  friend class CookieTreeSessionStorageNode;
  friend class CookieTreeIndexedDBNode;
  friend class CookieTreeFileSystemNode;
  friend class CookieTreeQuotaNode;
  friend class CookieTreeServerBoundCertNode;
  friend class CookieTreeFlashLSONode;

  // Callback methods to be invoked when fetching the data is complete.
  void OnAppCacheModelInfoLoaded();
  void OnCookiesModelInfoLoaded(const net::CookieList& cookie_list);
  void OnDatabaseModelInfoLoaded(const DatabaseInfoList& database_info);
  void OnLocalStorageModelInfoLoaded(
      const LocalStorageInfoList& local_storage_info);
  void OnSessionStorageModelInfoLoaded(
      const LocalStorageInfoList& local_storage_info);
  void OnIndexedDBModelInfoLoaded(
      const IndexedDBInfoList& indexed_db_info);
  void OnFileSystemModelInfoLoaded(
      const FileSystemInfoList& file_system_info);
  void OnQuotaModelInfoLoaded(const QuotaInfoList& quota_info);
  void OnServerBoundCertModelInfoLoaded(const ServerBoundCertList& cert_list);
  void OnFlashLSOInfoLoaded(const FlashLSODomainList& domains);

  // Pointers to the helper objects, needed to retreive all the types of locally
  // stored data.
  scoped_refptr<BrowsingDataAppCacheHelper> appcache_helper_;
  scoped_refptr<BrowsingDataCookieHelper> cookie_helper_;
  scoped_refptr<BrowsingDataDatabaseHelper> database_helper_;
  scoped_refptr<BrowsingDataLocalStorageHelper> local_storage_helper_;
  scoped_refptr<BrowsingDataLocalStorageHelper> session_storage_helper_;
  scoped_refptr<BrowsingDataIndexedDBHelper> indexed_db_helper_;
  scoped_refptr<BrowsingDataFileSystemHelper> file_system_helper_;
  scoped_refptr<BrowsingDataQuotaHelper> quota_helper_;
  scoped_refptr<BrowsingDataServerBoundCertHelper> server_bound_cert_helper_;
  scoped_refptr<BrowsingDataFlashLSOHelper> flash_lso_helper_;

  // Storage for all the data that was retrieved through the helper objects.
  // The collected data is used for (re)creating the CookiesTreeModel.
  AppCacheInfoMap appcache_info_;
  CookieList cookie_list_;
  DatabaseInfoList database_info_list_;
  LocalStorageInfoList local_storage_info_list_;
  LocalStorageInfoList session_storage_info_list_;
  IndexedDBInfoList indexed_db_info_list_;
  FileSystemInfoList file_system_info_list_;
  QuotaInfoList quota_info_list_;
  ServerBoundCertList server_bound_cert_list_;
  FlashLSODomainList flash_lso_domain_list_;

  // A delegate, which must outlive this object. The update callbacks use the
  // delegate to deliver the updated data to the CookieTreeModel.
  CookiesTreeModel* model_;

  base::WeakPtrFactory<LocalDataContainer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalDataContainer);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_LOCAL_DATA_CONTAINER_H_
