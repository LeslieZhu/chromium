// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var CommandUtil = {};

/**
 * Extracts root on which command event was dispatched.
 *
 * @param {Event} event Command event for which to retrieve root to operate on.
 * @param {cr.ui.List} rootsList Root list to extract root node.
 * @return {DirectoryEntry} Found root.
 */
CommandUtil.getCommandRoot = function(event, rootsList) {
  var result = rootsList.dataModel.item(
                   rootsList.getIndexOfListItem(event.target)) ||
               rootsList.selectedItem;

  return result;
};

/**
 * @param {Event} event Command event for which to retrieve root type.
 * @param {cr.ui.List} rootsList Root list to extract root node.
 * @return {string} Found root.
 */
CommandUtil.getCommandRootType = function(event, rootsList) {
  var root = CommandUtil.getCommandRoot(event, rootsList);

  return root && PathUtil.getRootType(root.fullPath);
};

/**
 * Checks if command can be executed on gdata.
 * @param {Event} event Command event to mark.
 * @param {FileManager} fileManager FileManager to use.
 */
CommandUtil.canExecuteOnGDataOnly = function(event, fileManager) {
  event.canExecute = fileManager.isOnGData();
};

/**
 * Registers handler on specific command on specific node.
 * @param {Node} node Node to register command handler on.
 * @param {string} commandId Command id to respond to.
 * @param {{execute:function, canExecute:function}} handler Handler to use.
 * @param {Object...} var_args Additional arguments to pass to handler.
 */
CommandUtil.registerCommand = function(node, commandId, handler, var_args) {
  var args = Array.prototype.slice.call(arguments, 3);

  node.addEventListener('command', function(event) {
    if (event.command.id == commandId) {
      handler.execute.apply(handler, [event].concat(args));
      event.cancelBubble = true;
    }
  });

  node.addEventListener('canExecute', function(event) {
    if (event.command.id == commandId)
      handler.canExecute.apply(handler, [event].concat(args));
  });
};

var Commands = {};

/**
 * Forwards all command events to standard document handlers.
 */
Commands.defaultCommand = {
  execute: function(event, document) {
    document.execCommand(event.command.id);
  },
  canExecute: function(event, document) {
    event.canExecute = document.queryCommandEnabled(event.command.id);
  }
};

/**
 * Unmounts external drive.
 */
Commands.unmountCommand = {
  execute: function(event, rootsList, fileManager) {
    var root = CommandUtil.getCommandRoot(event, rootsList);
    if (!root) return;

    function doUnmount() {
      fileManager.unmountVolume(PathUtil.getRootPath(root.fullPath));
    }

    if (fileManager.butterBar_.forceDeleteAndHide()) {
      // TODO(dgozman): add completion callback to file copy manager.
      setTimeout(doUnmount, 1000);
    } else {
      doUnmount();
    }
  },
  canExecute: function(event, rootsList) {
    var rootType = CommandUtil.getCommandRootType(event, rootsList);

    event.canExecute = (rootType == RootType.ARCHIVE ||
                        rootType == RootType.REMOVABLE);
  }
};

/**
 * Formats external drive.
 */
Commands.formatCommand = {
  execute: function(event, rootsList, fileManager) {
    var root = CommandUtil.getCommandRoot(event, rootsList);

    if (root) {
      var url = util.makeFilesystemUrl(PathUtil.getRootPath(root.fullPath));
      fileManager.confirm.show(
          loadTimeData.getString('FORMATTING_WARNING'),
          chrome.fileBrowserPrivate.formatDevice.bind(null, url));
    }
  },
  canExecute: function(event, rootsList) {
    event.canExecute = (CommandUtil.getCommandRootType(event, rootsList) ==
                        RootType.REMOVABLE);
  }
};

/**
 * Imports photos from external drive
 */
Commands.importCommand = {
  execute: function(event, rootsList) {
    var root = CommandUtil.getCommandRoot(event, rootsList);

    if (root) {
      chrome.windows.create({url: chrome.extension.getURL('photo_import.html') +
          '#' + PathUtil.getRootPath(root.fullPath), type: 'popup'});
    }
  },
  canExecute: function(event, rootsList) {
    event.canExecute =
        (CommandUtil.getCommandRootType(event, rootsList) != RootType.GDATA);
  }
};

/**
 * Initiates new folder creation.
 */
Commands.newFolderCommand = {
  execute: function(event, fileManager) {
    fileManager.createNewFolder();
  },
  canExecute: function(event, fileManager, directoryModel) {
    event.canExecute = !fileManager.isOnReadonlyDirectory() &&
                       !directoryModel.isSearching() &&
                       !fileManager.isRenamingInProgress();
  }
};

/**
 * Deletes selected files.
 */
Commands.deleteFileCommand = {
  execute: function(event, fileManager) {
    fileManager.deleteSelection();
  },
  canExecute: function(event, fileManager) {
    var selection = fileManager.getSelection();
    event.canExecute = !fileManager.isOnReadonlyDirectory() &&
        selection &&
        selection.totalCount > 0;
  }
};

/**
 * Pastes files from clipboard.
 */
Commands.pasteFileCommand = {
  execute: Commands.defaultCommand.execute,
  canExecute: function(event, document, fileTransferController) {
    event.canExecute = (fileTransferController &&
        fileTransferController.queryPasteCommandEnabled());
  }
};

/**
 * Initiates file renaming.
 */
Commands.renameFileCommand = {
  execute: function(event, fileManager) {
    fileManager.initiateRename();
  },
  canExecute: function(event, fileManager) {
    var selection = fileManager.getSelection();
    event.canExecute =
        !fileManager.isRenamingInProgress() &&
        !fileManager.isOnReadonlyDirectory() &&
        selection &&
        selection.totalCount == 1;
  }
};

/**
 * Opens gdata help.
 */
Commands.gdataHelpCommand = {
  execute: function() {
    window.open(FileManager.GOOGLE_DRIVE_HELP, 'help');
  },
  canExecute: CommandUtil.canExecuteOnGDataOnly
};

/**
 * Opens gdata buy-more-space url.
 */
Commands.gdataBuySpaceCommand = {
  execute: function() {
    window.open(FileManager.GOOGLE_DRIVE_BUY_STORAGE, 'buy-more-space');
  },
  canExecute: CommandUtil.canExecuteOnGDataOnly
};

/**
 * Clears gdata cache.
 */
Commands.gdataClearCacheCommand = {
  execute: function() {
    chrome.fileBrowserPrivate.clearDriveCache();
  },
  canExecute: CommandUtil.canExecuteOnGDataOnly
};

/**
 * Opens drive.google.com.
 */
Commands.gdataGoToDriveCommand = {
  execute: function() {
    window.open(FileManager.GOOGLE_DRIVE_ROOT, 'gdata-root');
  },
  canExecute: CommandUtil.canExecuteOnGDataOnly
};

/**
 * Displays open with dialog for current selection.
 */
Commands.openWithCommand = {
  execute: function(event, fileManager) {
    var tasks = fileManager.getSelection().tasks;
    if (tasks) {
      tasks.showTaskPicker(fileManager.defaultTaskPicker,
          str('OPEN_WITH_BUTTON_LABEL'),
          null,
          function(task) {
            tasks.execute(task.taskId);
          });
    }
  },
  canExecute: function(event, fileManager) {
    var tasks = fileManager.getSelection().tasks;
    event.canExecute = tasks && tasks.size() > 1;
  }
};
