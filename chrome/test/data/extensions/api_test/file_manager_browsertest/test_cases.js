// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Expected files before tests are performed. Entries for Local tests.
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_FILES_BEFORE_LOCAL = [
  ['hello.txt', '123 bytes', 'Plain text', 'Sep 4, 1998 12:34 PM'],
  ['world.mpeg', '1,000 bytes', 'MPEG video', 'Jul 4, 2012 10:35 AM'],
  ['My Desktop Background.png', '1 KB', 'PNG image', 'Jan 18, 2038 1:02 AM'],
  ['photos', '--', 'Folder', 'Jan 1, 1980 11:59 PM']
  // ['.warez', '--', 'Folder', 'Oct 26, 1985 1:39 PM']  # should be hidden
].sort();

/**
 * Expected files before tests are performed. Entries for Drive tests.
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_FILES_BEFORE_DRIVE = [
  ['hello.txt', '123 bytes', 'Plain text', 'Sep 4, 1998 12:34 PM'],
  ['world.mpeg', '1,000 bytes', 'MPEG video', 'Jul 4, 2012 10:35 AM'],
  ['My Desktop Background.png', '1 KB', 'PNG image', 'Jan 18, 2038 1:02 AM'],
  ['photos', '--', 'Folder', 'Jan 1, 1980 11:59 PM'],
  ['Test Document.gdoc','--','Google document','Apr 10, 2013 4:20 PM'],
  ['Test Shared Document.gdoc','--','Google document','Mar 20, 2013 10:40 PM']
].sort();

/**
 * Expected files added during some tests.
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_NEWLY_ADDED_FILE = [
  ['newly added file.mp3', '2 KB', 'MP3 audio', 'Sep 4, 1998 12:00 AM']
];

/**
 * Time out value to be used for waitForFiles.
 * @type {number}
 * @const
 */
var WAIT_FOR_FILES_TIME_OUT = 3000;

/**
 * @param {boolean} isDrive True if the test is for Drive.
 * @return {Array.<Array.<string>>} A sorted list of expected entries at the
 *     initial state.
 */
function getExpectedFilesBefore(isDrive) {
  return isDrive ?
      EXPECTED_FILES_BEFORE_DRIVE :
      EXPECTED_FILES_BEFORE_LOCAL;
}

/**
 * Opens a Files.app's main window and waits until it is initialized.
 *
 * @param {string} path Directory to be opened.
 * @param {function(string, Array.<Array.<string>>)} Callback with the app id
 *     and with the file list.
 */
function setupAndWaitUntilReady(path, callback) {
  callRemoteTestUtil('openMainWindow', null, [path], function(appId) {
    callRemoteTestUtil('waitForFileListChange', appId, [0], function(files) {
      callback(appId, files);
    });
  });
}

/**
 * Verifies if there are no Javascript errors in any of the app windows.
 * @param {function()} Completion callback.
 */
function checkIfNoErrorsOccured(callback) {
  callRemoteTestUtil('getErrorCount', null, [], function(count) {
    chrome.test.assertEq(0, count);
    callback();
  });
}

/**
 * Expected files shown in "Recent". Directories (e.g. 'photos') are not in this
 * list as they are not expected in "Recent".
 *
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_FILES_IN_RECENT = [
  ['hello.txt', '123 bytes', 'Plain text', 'Sep 4, 1998 12:34 PM'],
  ['world.mpeg', '1,000 bytes', 'MPEG video', 'Jul 4, 2012 10:35 AM'],
  ['My Desktop Background.png', '1 KB', 'PNG image', 'Jan 18, 2038 1:02 AM'],
  ['Test Document.gdoc','--','Google document','Apr 10, 2013 4:20 PM'],
  ['Test Shared Document.gdoc','--','Google document','Mar 20, 2013 10:40 PM']
].sort();

/**
 * Expected files shown in "Offline", which should have the files
 * "available offline". Google Documents, Google Spreadsheets, and the files
 * cached locally are "available offline".
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_FILES_IN_OFFLINE = [
  ['Test Document.gdoc','--','Google document','Apr 10, 2013 4:20 PM'],
  ['Test Shared Document.gdoc','--','Google document','Mar 20, 2013 10:40 PM']
];

/**
 * Expected files shown in "Shared with me", which should be the entries labeled
 * with "shared-with-me".
 * @type {Array.<Array.<string>>}
 * @const
 */
var EXPECTED_FILES_IN_SHARED_WITH_ME = [
  ['Test Shared Document.gdoc','--','Google document','Mar 20, 2013 10:40 PM']
];

/**
 * Namespace for test cases.
 */
var testcase = {};

/**
 * Namespace for intermediate test cases.
 * */
testcase.intermediate = {};

/**
 * Tests if the files initially added by the C++ side are displayed, and
 * that a subsequently added file shows up.
 *
 * @param {string} path Directory path to be tested.
 */
testcase.intermediate.fileDisplay = function(path) {
  var expectedFilesBefore = getExpectedFilesBefore(path == '/drive/root');
  var expectedFilesAfter =
      expectedFilesBefore.concat(EXPECTED_NEWLY_ADDED_FILE).sort();

  setupAndWaitUntilReady(path, function(appId, actualFilesBefore) {
    chrome.test.assertEq(expectedFilesBefore, actualFilesBefore);
    chrome.test.sendMessage('initial check done', function(reply) {
      chrome.test.assertEq('file added', reply);
      callRemoteTestUtil(
          'waitForFileListChange',
          appId,
          [expectedFilesBefore.length], function(actualFilesAfter) {
            chrome.test.assertEq(expectedFilesAfter, actualFilesAfter);
            checkIfNoErrorsOccured(chrome.test.succeed);
          });
    });
  });
};

/**
 * Tests copying a file to the same directory and waits until the file lists
 * changes.
 *
 * @param {string} path Directory path to be tested.
 */
testcase.intermediate.keyboardCopy = function(path, callback) {
  setupAndWaitUntilReady(path, function(appId) {
    callRemoteTestUtil('copyFile', appId, ['world.mpeg'], function(result) {
      chrome.test.assertFalse(!result);
      callRemoteTestUtil('waitForFileListChange',
                         appId,
                         [getExpectedFilesBefore(path == '/drive/root').length],
                         checkIfNoErrorsOccured.bind(null,
                                                     chrome.test.succeed));
    });
  });
};

/**
 * Tests deleting a file and and waits until the file lists changes.
 * @param {string} path Directory path to be tested.
 */
testcase.intermediate.keyboardDelete = function(path) {
  setupAndWaitUntilReady(path, function(appId) {
    callRemoteTestUtil('deleteFile', appId, ['world.mpeg'], function(result) {
      chrome.test.assertFalse(!result);
      callRemoteTestUtil('waitAndAcceptDialog', appId, [], function() {
        callRemoteTestUtil(
            'waitForFileListChange',
            appId,
            [getExpectedFilesBefore(path == '/drive/root').length],
            checkIfNoErrorsOccured.bind(null, chrome.test.succeed));
      });
    });
  });
};

testcase.fileDisplayDownloads = function() {
  testcase.intermediate.fileDisplay('/Downloads');
};

testcase.keyboardCopyDownloads = function() {
  testcase.intermediate.keyboardCopy('/Downloads');
};

testcase.keyboardDeleteDownloads = function() {
  testcase.intermediate.keyboardDelete('/Downloads');
};

testcase.fileDisplayDrive = function() {
  testcase.intermediate.fileDisplay('/drive/root');
};

testcase.keyboardCopyDrive = function() {
  testcase.intermediate.keyboardCopy('/drive/root');
};

testcase.keyboardDeleteDrive = function() {
  testcase.intermediate.keyboardDelete('/drive/root');
};

/**
 * Tests opening the "Recent" on the sidebar navigation by clicking the icon,
 * and verifies the directory contents. We test if there are only files, since
 * directories are not allowed in "Recent". This test is only available for
 * Drive.
 */
testcase.openSidebarRecent = function() {
  var onFileListChange = function(actualFilesAfter) {
    chrome.test.assertEq(EXPECTED_FILES_IN_RECENT, actualFilesAfter);
    checkIfNoErrorsOccured(chrome.test.succeed);
  };

  setupAndWaitUntilReady('/drive/root', function(appId) {
    // Use the icon for a click target.
    callRemoteTestUtil(
        'selectVolume', appId, ['drive_recent'],
        function(result) {
          chrome.test.assertFalse(!result);
          callRemoteTestUtil(
              'waitForFileListChange',
              appId,
              [getExpectedFilesBefore(true /* isDrive */).length],
              onFileListChange);
        });
  });
};

/**
 * Tests opening the "Offline" on the sidebar navigation by clicking the icon,
 * and checks contenets of the file list. Only the entries "available offline"
 * should be shown. "Available offline" entires are hosted documents and the
 * entries cached by DriveCache.
 */
testcase.openSidebarOffline = function() {
  var onFileListChange = function(actualFilesAfter) {
    chrome.test.assertEq(EXPECTED_FILES_IN_OFFLINE, actualFilesAfter);
    checkIfNoErrorsOccured(chrome.test.succeed);
  };

  setupAndWaitUntilReady('/drive/root/', function(appId) {
    // Use the icon for a click target.
    callRemoteTestUtil(
        'selectVolume', appId, ['drive_offline'],
        function(result) {
          chrome.test.assertFalse(!result);
          callRemoteTestUtil(
              'waitForFileListChange',
              appId,
              [getExpectedFilesBefore(true /* isDrive */).length],
              onFileListChange);
        });
  });
};

/**
 * Tests opening the "Shared with me" on the sidebar navigation by clicking the
 * icon, and checks contents of the file list. Only the entries labeled with
 * "shared-with-me" should be shown.
 */
testcase.openSidebarSharedWithMe = function() {
  var onFileListChange = chrome.test.callbackPass(function(actualFilesAfter) {
    chrome.test.assertEq(EXPECTED_FILES_IN_SHARED_WITH_ME, actualFilesAfter);
  });

  setupAndWaitUntilReady('/drive/root/', function(appId) {
    // Use the icon for a click target.
    callRemoteTestUtil(
        'selectVolume', appId, ['drive_shared_with_me'],
        function(result) {
          chrome.test.assertFalse(!result);
          callRemoteTestUtil(
              'waitForFileListChange',
              appId,
              [getExpectedFilesBefore(true /* isDrive */).length],
              onFileListChange);
        });
  });
};

/**
 * Tests autocomplete with a query 'hello'. This test is only available for
 * Drive.
 */
testcase.autocomplete = function() {
  var EXPECTED_AUTOCOMPLETE_LIST = [
    '\'hello\' - search Drive\n',
    'hello.txt\n',
  ];

  var onAutocompleteListShown = function(autocompleteList) {
    chrome.test.assertEq(EXPECTED_AUTOCOMPLETE_LIST, autocompleteList);
    checkIfNoErrorsOccured(chrome.test.succeed);
  };

  setupAndWaitUntilReady('/drive/root', function(appId, list) {
    callRemoteTestUtil('performAutocompleteAndWait',
                       appId,
                       ['hello', EXPECTED_AUTOCOMPLETE_LIST.length],
                       onAutocompleteListShown);
  });
};

/**
 * Test function to copy from the specified source to the specified destination.
 * @param {string} targetFile Name of target file to be copied.
 * @param {string} srcName Type of source volume. e.g. downloads, drive,
 *     drive_recent, drive_shared_with_me, drive_offline.
 * @param {Array.<Array.<string>>} srcContents Expected initial contents in the
 *     source volume.
 * @param {string} dstName Type of destination volume.
 * @param {Array.<Array.<string>>} dstContents Expected initial contents in the
 *     destination volume.
 */
testcase.intermediate.copyBetweenVolumes = function(targetFile,
                                                    srcName,
                                                    srcContents,
                                                    dstName,
                                                    dstContents) {
  var appId;
  var steps = [
    function() {
      setupAndWaitUntilReady('/Downloads', steps.shift());
    },
    function(inAppId) {
      appId = inAppId;
      callRemoteTestUtil('selectVolume', appId, [srcName], steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('waitForFiles',
                         appId,
                         [srcContents, WAIT_FOR_FILES_TIME_OUT],
                         steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('selectFile', appId, [targetFile], steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('execCommand', appId, ['copy'], steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('selectVolume', appId, [dstName], steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('waitForFiles',
                         appId,
                         [dstContents, WAIT_FOR_FILES_TIME_OUT],
                         steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('execCommand', appId, ['paste'], steps.shift());
    },
    function(result) {
      chrome.test.assertTrue(result);
      callRemoteTestUtil('waitForFileListChange', appId,
                         [dstContents.length], steps.shift());
    },
    function(actualFilesAfter) {
      chrome.test.assertEq(dstContents.length + 1,
                           actualFilesAfter.length);
      var copiedItem = null;
      for (var i = 0; i < srcContents.length; i++) {
        if (srcContents[i][0] == targetFile) {
          copiedItem = srcContents[i];
          break;
        }
      }
      chrome.test.assertTrue(copiedItem != null);
      for (var i = 0; i < dstContents.length; i++) {
        if (dstContents[i][0] == targetFile) {
          copiedItem[0] = copiedItem[0].replace(/\.(?=[^\.]+$)/, ' (1).');
          break;
        }
      }
      // File size can not be obtained on drive_shared_with_me volume.
      var ignoreSize = srcName == 'drive_shared_with_me' ||
                       dstName == 'drive_shared_with_me';
      for (var i = 0; i < actualFilesAfter.length; i++) {
        if (actualFilesAfter[i][0] == copiedItem[0] &&
            (ignoreSize || actualFilesAfter[i][1] == copiedItem[1]) &&
            actualFilesAfter[i][2] == copiedItem[2]) {
          chrome.test.succeed();
          return;
        }
      }
      chrome.test.fail();
    }
  ];
  steps = steps.map(function(f) { return chrome.test.callbackPass(f); });
  steps.shift()();
};

/**
 * Tests copy from drive's root to local's downloads.
 */
testcase.transferFromDriveToDownloads = function() {
  testcase.intermediate.copyBetweenVolumes('hello.txt',
                                           'drive',
                                           EXPECTED_FILES_BEFORE_DRIVE,
                                           'downloads',
                                           EXPECTED_FILES_BEFORE_LOCAL);
};

/**
 * Tests copy from local's downloads to drive's root.
 */
testcase.transferFromDownloadsToDrive = function() {
  testcase.intermediate.copyBetweenVolumes('hello.txt',
                                           'downloads',
                                           EXPECTED_FILES_BEFORE_LOCAL,
                                           'drive',
                                           EXPECTED_FILES_BEFORE_DRIVE);
};

/**
 * Tests copy from drive's shared_with_me to local's downloads.
 */
testcase.transferFromSharedToDownloads = function() {
  testcase.intermediate.copyBetweenVolumes('Test Shared Document.gdoc',
                                           'drive_shared_with_me',
                                           EXPECTED_FILES_IN_SHARED_WITH_ME,
                                           'downloads',
                                           EXPECTED_FILES_BEFORE_LOCAL);
};

/**
 * Tests copy from drive's shared_with_me to drive's root.
 */
testcase.transferFromSharedToDrive = function() {
  testcase.intermediate.copyBetweenVolumes('Test Shared Document.gdoc',
                                           'drive_shared_with_me',
                                           EXPECTED_FILES_IN_SHARED_WITH_ME,
                                           'drive',
                                           EXPECTED_FILES_BEFORE_DRIVE);
};

/**
 * Tests copy from drive's recent to local's downloads.
 */
testcase.transferFromRecentToDownloads = function() {
  testcase.intermediate.copyBetweenVolumes('hello.txt',
                                           'drive_recent',
                                           EXPECTED_FILES_IN_RECENT,
                                           'downloads',
                                           EXPECTED_FILES_BEFORE_LOCAL);
};

/**
 * Tests copy from drive's recent to drive's root.
 */
testcase.transferFromRecentToDrive = function() {
  testcase.intermediate.copyBetweenVolumes('hello.txt',
                                           'drive_recent',
                                           EXPECTED_FILES_IN_RECENT,
                                           'drive',
                                           EXPECTED_FILES_BEFORE_DRIVE);
};

/**
 * Tests copy from drive's offline to local's downloads.
 */
testcase.transferFromOfflineToDownloads = function() {
  testcase.intermediate.copyBetweenVolumes('Test Document.gdoc',
                                           'drive_offline',
                                           EXPECTED_FILES_IN_OFFLINE,
                                           'downloads',
                                           EXPECTED_FILES_BEFORE_LOCAL);
};

/**
 * Tests copy from drive's offline to drive's root.
 */
testcase.transferFromOfflineToDrive = function() {
  testcase.intermediate.copyBetweenVolumes('Test Document.gdoc',
                                           'drive_offline',
                                           EXPECTED_FILES_IN_OFFLINE,
                                           'drive',
                                           EXPECTED_FILES_BEFORE_DRIVE);
};
