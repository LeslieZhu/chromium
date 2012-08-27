// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

document.addEventListener('DOMContentLoaded', function() {
  if (document.location.hash)  // File path passed after the #.
    Gallery.openStandalone(decodeURI(document.location.hash.substr(1)));
});

/**
 * Gallery for viewing and editing image files.
 *
 * @param {Object} context Object containing the following:
 *     {function(string)} onNameChange Called every time a selected
 *         item name changes (on rename and on selection change).
 *     {function} onClose
 *     {MetadataCache} metadataCache
 *     {Array.<Object>} shareActions
 *     {string} readonlyDirName Directory name for readonly warning or null.
 *     {DirEntry} saveDirEntry Directory to save to.
 *     {function(string)} displayStringFunction
 * @class
 * @constructor
 */
function Gallery(context) {
  this.container_ = document.querySelector('.gallery');
  this.document_ = document;
  this.context_ = context;
  this.metadataCache_ = context.metadataCache;

  this.dataModel_ = new cr.ui.ArrayDataModel([]);
  this.selectionModel_ = new cr.ui.ListSelectionModel();

  var strf = context.displayStringFunction;
  this.displayStringFunction_ = function(id, formatArgs) {
    var args = Array.prototype.slice.call(arguments);
    args[0] = 'GALLERY_' + id.toUpperCase();
    return strf.apply(null, args);
  };

  this.initListeners_();
  this.initDom_();
}

/**
 * Create and initialize a Gallery object based on a context.
 *
 * @param {Object} context Gallery context.
 * @param {Array.<string>} urls Array of image urls.
 * @param {string} selectedUrl Selected url.
 * @param {boolean} opt_grid True if open in the grid view.
 */
Gallery.open = function(context, urls, selectedUrl, opt_grid) {
  Gallery.instance = new Gallery(context);
  Gallery.instance.load(urls, selectedUrl, opt_grid);
};

/**
 * Create a Gallery object in a tab.
 * @param {string} path File system path to a selected file.
 */
Gallery.openStandalone = function(path) {
  ImageUtil.metrics = metrics;

  var currentDir;
  var urls = [];
  var selectedUrl;

  Gallery.getFileBrowserPrivate().requestLocalFileSystem(function(filesystem) {
    // If the path points to the directory scan it.
    filesystem.root.getDirectory(path, {create: false}, scanDirectory,
        function() {
          // Try to scan the parent directory.
          var pathParts = path.split('/');
          pathParts.pop();
          var parentPath = pathParts.join('/');
          filesystem.root.getDirectory(parentPath, {create: false},
              scanDirectory, open /* no data, just display an error */);
        });
  });

  function scanDirectory(dirEntry) {
    currentDir = dirEntry;
    util.forEachDirEntry(currentDir, function(entry) {
      if (entry == null) {
        open();
      } else if (FileType.isImageOrVideo(entry)) {
        var url = entry.toURL();
        urls.push(url);
        if (entry.fullPath == path)
          selectedUrl = url;
      }
    });
  }

  function onNameChange(name) {
    window.top.document.title = name || currentDir.name;

    var newPath = currentDir.fullPath + '/' + name;
    var location = document.location.origin + document.location.pathname +
                   '#' + encodeURI(newPath);
    history.replaceState(undefined, newPath, location);
  }

  function onClose() {
    document.location = 'main.html?' +
        JSON.stringify({defaultPath: document.location.hash.substr(1)});
  }

  function open() {
    urls.sort();
    Gallery.getFileBrowserPrivate().getStrings(function(strings) {
      loadTimeData.data = strings;
      var context = {
        readonlyDirName: null,
        saveDirEntry: currentDir,
        metadataCache: MetadataCache.createFull(),
        onNameChange: onNameChange,
        onClose: onClose,
        displayStringFunction: strf
      };
      Gallery.open(context, urls, selectedUrl, !selectedUrl);
    });
  }
};

/**
 * Tools fade-out timeout im milliseconds.
 * @type {Number}
 */
Gallery.FADE_TIMEOUT = 3000;

/**
 * First time tools fade-out timeout im milliseconds.
 * @type {Number}
 */
Gallery.FIRST_FADE_TIMEOUT = 1000;

/**
 * Types of metadata Gallery uses (to query the metadata cache).
 */
Gallery.METADATA_TYPE = 'thumbnail|filesystem|media|streaming';

/**
 * Initialize listeners.
 * @private
 */

Gallery.prototype.initListeners_ = function() {
  this.document_.oncontextmenu = function(e) { e.preventDefault(); };

  this.document_.body.addEventListener('keydown', this.onKeyDown_.bind(this));

  this.inactivityWatcher_ = new MouseInactivityWatcher(
      this.container_, Gallery.FADE_TIMEOUT, this.hasActiveTool.bind(this));

  // Show tools when the user touches the screen.
  this.document_.body.addEventListener('touchstart',
      this.inactivityWatcher_.startActivity.bind(this.inactivityWatcher_));
  var initiateFading =
      this.inactivityWatcher_.stopActivity.bind(this.inactivityWatcher_,
          Gallery.FADE_TIMEOUT);
  this.document_.body.addEventListener('touchend', initiateFading);
  this.document_.body.addEventListener('touchcancel', initiateFading);
};

/**
 * Initializes DOM UI
 * @private
 */
Gallery.prototype.initDom_ = function() {
  var content = util.createChild(this.container_, 'content');
  content.addEventListener('click', this.onContentClick_.bind(this));

  var closeButton = util.createChild(this.container_, 'close tool dimmable');
  util.createChild(closeButton);
  closeButton.addEventListener('click', this.onClose_.bind(this));

  this.toolbar_ = util.createChild(this.container_, 'toolbar tool dimmable');

  this.filenameSpacer_ = util.createChild(this.toolbar_, 'filename-spacer');

  var nameBox = util.createChild(this.filenameSpacer_, 'namebox');

  this.filenameText_ = util.createChild(nameBox);
  this.filenameText_.addEventListener('click',
      this.onFilenameClick_.bind(this));

  this.filenameEdit_ = this.document_.createElement('input');
  this.filenameEdit_.setAttribute('type', 'text');
  this.filenameEdit_.addEventListener('blur',
      this.onFilenameEditBlur_.bind(this));
  this.filenameEdit_.addEventListener('keydown',
      this.onFilenameEditKeydown_.bind(this));
  nameBox.appendChild(this.filenameEdit_);

  util.createChild(this.toolbar_, 'button-spacer');

  this.prompt_ = new ImageEditor.Prompt(
      this.container_, this.displayStringFunction_);

  this.modeButton_ = util.createChild(this.toolbar_, 'button mode');
  this.modeButton_.addEventListener('click', this.toggleMode_.bind(this, null));

  this.gridMode_ = new GridMode(this.container_, content,
      this.dataModel_, this.selectionModel_, this.metadataCache_,
      this.toggleMode_.bind(this, null));

  this.slideMode_ = new SlideMode(this.container_, content,
      this.toolbar_, this.prompt_,
      this.dataModel_, this.selectionModel_,
      this.context_, this.displayStringFunction_);

  var deleteButton = this.document_.createElement('div');
  deleteButton.className = 'button delete';
  deleteButton.title = this.displayStringFunction_('delete');
  deleteButton.addEventListener('click', this.onDelete_.bind(this));
  this.toolbar_.insertBefore(
      deleteButton, this.toolbar_.querySelector('.edit'));

  this.shareButton_ = util.createChild(this.toolbar_, 'button share');
  this.shareButton_.title = this.displayStringFunction_('share');
  this.shareButton_.addEventListener('click', this.toggleShare_.bind(this));

  this.shareMenu_ = util.createChild(this.container_, 'share-menu');
  this.shareMenu_.hidden = true;
  util.createChild(this.shareMenu_, 'bubble-point');

  Gallery.getFileBrowserPrivate().isFullscreen(function(fullscreen) {
    this.originalFullscreen_ = fullscreen;
  }.bind(this));

  this.slideMode_.addEventListener('useraction', this.onUserAction_.bind(this));
  this.slideMode_.addEventListener('content', this.onContentChange_.bind(this));
  this.slideMode_.addEventListener('namechange', this.onSelection_.bind(this));
  this.selectionModel_.addEventListener('change', this.onSelection_.bind(this));
};

/**
 * Load the content.
 *
 * @param {Array.<string>} urls Array of urls.
 * @param {string} selectedUrl Selected url.
 * @param {boolean} opt_grid True if open in the grid view.
 */
Gallery.prototype.load = function(urls, selectedUrl, opt_grid) {
  var items = [];
  for (var index = 0; index < urls.length; ++index) {
    items.push(new Gallery.Item(urls[index]));
  }
  this.dataModel_.push.apply(this.dataModel_, items);

  var selectedIndex = urls.indexOf(selectedUrl);
  if (selectedIndex >= 0)
    this.selectionModel_.setIndexSelected(selectedIndex, true);
  else
    this.onSelection_();

  this.currentMode_ = opt_grid ? this.gridMode_ : this.slideMode_;
  this.currentMode_.enter(function() {
      // Flash the toolbar briefly to show it is there.
      this.inactivityWatcher_.startActivity();
      this.inactivityWatcher_.stopActivity(Gallery.FIRST_FADE_TIMEOUT);
    }.bind(this));
};

/**
 * Close the Gallery.
 * @private
 */
Gallery.prototype.close_ = function() {
  Gallery.getFileBrowserPrivate().isFullscreen(function(fullscreen) {
    if (this.originalFullscreen_ != fullscreen) {
      Gallery.toggleFullscreen();
    }
    this.context_.onClose();
  }.bind(this));
};

/**
 * Handle user's 'Close' action (Escape or a click on the X icon).
 * @private
 */
Gallery.prototype.onClose_ = function() {
  this.executeWhenReady(this.close_.bind(this));
};

/**
 * Execute a function when the editor is done with the modifications.
 * @param {function} callback Function to execute.
 */
Gallery.prototype.executeWhenReady = function(callback) {
  if (this.currentMode_ == this.slideMode_)
    this.slideMode_.editor_.executeWhenReady(callback);
  else
    callback();
};

/**
 * @return {Object} File browser private API.
 */
Gallery.getFileBrowserPrivate = function() {
  return chrome.fileBrowserPrivate || window.top.chrome.fileBrowserPrivate;
};

/**
 * Switches gallery to fullscreen mode and back.
 */
Gallery.toggleFullscreen = function() {
  Gallery.getFileBrowserPrivate().toggleFullscreen();
};

/**
 * @return {boolean} True if some tool is currently active.
 */
Gallery.prototype.hasActiveTool = function() {
  return this.currentMode_.hasActiveTool() ||
      this.isSharing_() || this.isRenaming_();
};

/**
 * Check if the tools are active and notify the inactivity watcher.
 * @private
 */
Gallery.prototype.checkActivity_ = function() {
  if (this.hasActiveTool())
    this.inactivityWatcher_.startActivity();
  else
    this.inactivityWatcher_.stopActivity();
};

/**
* External user action event handler.
* @private
*/
Gallery.prototype.onUserAction_ = function() {
  this.closeShareMenu_();
  this.checkActivity_();
};

/**
 * Mode toggle event handler.
 * @param {function} opt_callback Callback.
 * @private
 */
Gallery.prototype.toggleMode_ = function(opt_callback) {
  this.currentMode_.leave(function() {
    if (this.currentMode_ == this.gridMode_) {
      this.currentMode_ = this.slideMode_;
      this.modeButton_.title = this.displayStringFunction_('mosaic');
    } else {
      this.currentMode_ = this.gridMode_;
      this.modeButton_.title = this.displayStringFunction_('slide');
    }
    this.currentMode_.enter(opt_callback);
    this.updateFilename_();
  }.bind(this));
};

/**
 * Delete event handler.
 * @private
 */
Gallery.prototype.onDelete_ = function() {
  var indexes = this.selectionModel_.selectedIndexes;
  for (var i = 0; i != indexes.length; i++)
    this.dataModel_.splice(indexes[i], 1);

  // TODO: delete actual files.
};

/**
 * @return {Array.<Gallery.Item>} Current selection.
 */
Gallery.prototype.getSelectedItems = function() {
  return this.selectionModel_.selectedIndexes.map(
      this.dataModel_.item.bind(this.dataModel_));
};

/**
 * @return {Gallery.Item} Current single selection.
 */
Gallery.prototype.getSingleSelectedItem = function() {
  var items = this.getSelectedItems();
  if (items.length > 1)
    throw new Error('Unexpected multiple selection');
  return items[0];
};

/**
* Selection change event handler.
* @private
*/
Gallery.prototype.onSelection_ = function() {
  this.updateFilename_();
  this.updateShareMenu_();
};

/**
* Content change event handler.
* @private
*/
Gallery.prototype.onContentChange_ = function() {
  this.updateFilename_();
  this.gridMode_.updateThumbnail(this.getSingleSelectedItem());
};

/**
 * Keydown handler.
 * @param {Event} event Event.
 * @private
 */
Gallery.prototype.onKeyDown_ = function(event) {
  var wasSharing = this.isSharing_();
  this.closeShareMenu_();

  if (this.currentMode_.onKeyDown(event))
    return;

  switch (util.getKeyModifiers(event) + event.keyIdentifier) {
    case 'U+0008': // Backspace.
      // The default handler would call history.back and close the Gallery.
      event.preventDefault();
      break;

    case 'U+001B':  // Escape
      // Swallow Esc if it closed the Share menu, otherwise close the Gallery.
      if (!wasSharing)
        this.onClose_();
      break;
  }
};

// Name box and rename support.

/**
 * Update the displayed current item file name.
 *
 * @private
 */
Gallery.prototype.updateFilename_ = function() {
  var displayName = '';
  var fullName = '';

  var selectedItems = this.getSelectedItems();
  if (selectedItems.length == 1) {
    fullName = selectedItems[0].getFileName();
    displayName = ImageUtil.getFileNameFromFullName(fullName);
  } else if (selectedItems.length > 1) {
    displayName =
        this.displayStringFunction_('ITEMS_SELECTED', selectedItems.length);
  }

  this.context_.onNameChange(
      this.currentMode_ == this.slideMode_ ? fullName : '');

  this.filenameEdit_.value = displayName;
  this.filenameText_.textContent = displayName;
};

/**
 * Click event handler on filename edit box
 * @private
 */
Gallery.prototype.onFilenameClick_ = function() {
  // We can't rename files in readonly directory.
  if (this.context_.readonlyDirName)
    return;

  // We can only rename a single file.
  if (this.getSelectedItems().length != 1)
    return;

  ImageUtil.setAttribute(this.filenameSpacer_, 'renaming', true);
  setTimeout(this.filenameEdit_.select.bind(this.filenameEdit_), 0);
  this.inactivityWatcher_.startActivity();
};

/**
 * Blur event handler on filename edit box
 * @private
 */
Gallery.prototype.onFilenameEditBlur_ = function() {
  if (this.filenameEdit_.value && this.filenameEdit_.value[0] == '.') {
    this.prompt_.show('file_hidden_name', 5000);
    this.filenameEdit_.focus();
    return;
  }

  var item = this.getSingleSelectedItem();
  var oldUrl = item.getUrl();

  var onFileExists = function() {
    this.prompt_.show('file_exists', 3000);
    this.filenameEdit_.value = name;
    this.onFilenameClick_();
  }.bind(this);

  var onSuccess = function() {
    this.slideMode_.updateSelectedUrl_(oldUrl, item.getUrl());
    this.updateFilename_();
  }.bind(this);

  if (this.filenameEdit_.value) {
    this.getSingleSelectedItem().rename(this.context_.saveDirEntry,
        this.filenameEdit_.value, onSuccess, onFileExists);
  }

  ImageUtil.setAttribute(this.filenameSpacer_, 'renaming', false);
  this.checkActivity_();
};

/**
 * Keydown event handler on filename edit box
 * @private
 */
Gallery.prototype.onFilenameEditKeydown_ = function() {
  switch (event.keyCode) {
    case 27:  // Escape
      this.updateFilename_();
      this.filenameEdit_.blur();
      break;

    case 13:  // Enter
      this.filenameEdit_.blur();
      break;
  }
  event.stopPropagation();
};

/**
 * @return {Boolean} True if file renaming is currently in progress
 * @private
 */
Gallery.prototype.isRenaming_ = function() {
  return this.filenameSpacer_.hasAttribute('renaming');
};

/**
 * Content area click handler.
 * @private
 */
Gallery.prototype.onContentClick_ = function() {
  this.closeShareMenu_();
  this.filenameEdit_.blur();
};

// Share button support.

/**
 * @return {boolean} True if the Share menu is active.
 * @private
 */
Gallery.prototype.isSharing_ = function() {
  return !this.shareMenu_.hidden;
};

/**
 * Close Share menu if it is open.
 * @private
 */
Gallery.prototype.closeShareMenu_ = function() {
  if (this.isSharing_())
    this.toggleShare_();
};

/**
 * Share button handler.
 * @private
 */
Gallery.prototype.toggleShare_ = function() {
  if (!this.shareButton_.hasAttribute('disabled'))
    this.shareMenu_.hidden = !this.shareMenu_.hidden;
  this.checkActivity_();
};

/**
 * Update available actions list based on the currently selected urls.
 * @private.
 */
Gallery.prototype.updateShareMenu_ = function() {
  var urls =
      this.getSelectedItems().map(function(item) { return item.getUrl() });

  var internalId = util.getExtensionId();
  function isShareAction(task) {
    var task_parts = task.taskId.split('|');
    return task_parts[0] != internalId;
  }

  var api = Gallery.getFileBrowserPrivate();
  var mimeTypes = [];  // TODO(kaznacheev) Collect mime types properly.
  api.getFileTasks(urls, mimeTypes, function(tasks) {
    var wasHidden = this.shareMenu_.hidden;
    this.shareMenu_.hidden = true;
    var items = this.shareMenu_.querySelectorAll('.item');
    for (var i = 0; i != items.length; i++) {
      items[i].parentNode.removeChild(items[i]);
    }

    for (var t = 0; t != tasks.length; t++) {
      var task = tasks[t];
      if (!isShareAction(task)) continue;

      var item = util.createChild(this.shareMenu_, 'item');
      item.textContent = task.title;
      item.style.backgroundImage = 'url(' + task.iconUrl + ')';
      item.addEventListener('click', function(taskId) {
        this.toggleShare_();  // Hide the menu.
        this.executeWhenReady(api.executeTask.bind(api, taskId, urls));
      }.bind(this, task.taskId));
    }

    var empty = this.shareMenu_.querySelector('.item') == null;
    ImageUtil.setAttribute(this.shareButton_, 'disabled', empty);
    this.shareMenu_.hidden = wasHidden || empty;
  }.bind(this));
};

/**
 * @param {Element} container Main container.
 * @param {Element} content Content container.
 * @param {cr.ui.ArrayDataModel} dataModel Data model.
 * @param {cr.ui.ListSelectionModel} selectionModel Selection model.
 * @param {MetadataCache} metadataCache Metadata cache.
 * @param {function} openSelectedItem Function to open the selected item in the
 *   slide mode.
 * @constructor
 */
function GridMode(container, content, dataModel, selectionModel,
                  metadataCache, openSelectedItem) {
  this.container_ = container;
  this.metadataCache_ = metadataCache;
  this.grid_ = util.createChild(content, 'thumbnail-grid', 'grid');
  cr.ui.Grid.decorate(this.grid_);

  this.grid_.dataModel = dataModel;
  this.grid_.selectionModel = selectionModel;
  this.grid_.itemConstructor =
      GridMode.Item.bind(null, container.ownerDocument, metadataCache);

  this.container_.ownerDocument.defaultView.addEventListener(
      'resize', this.redraw_.bind(this));

  this.openSelectedItem_ = openSelectedItem;
  this.grid_.addEventListener('dblclick', this.openSelectedItem_);
}

/**
 * Enter the mode.
 * @param {function} opt_callback Callback.
 */
GridMode.prototype.enter = function(opt_callback) {
  this.container_.setAttribute('mode', 'grid');
  this.redraw_();
  if (opt_callback) opt_callback();
};

/**
 * Leave the mode.
 * @param {function} opt_callback Callback.
 */
GridMode.prototype.leave = function(opt_callback) {
  if (opt_callback) opt_callback();
};

/**
 * Redraw the grid.
 * @private
 */
GridMode.prototype.redraw_ = function() {
  this.grid_.startBatchUpdates();
  setTimeout(function() {
    this.grid_.columns = 0;
    this.grid_.redraw();
    this.grid_.endBatchUpdates();
  }.bind(this), 0);
};

/**
 * @param {Gallery.Item} item The updated item.
 */
GridMode.prototype.updateThumbnail = function(item) {
  var listItems = this.grid_.querySelectorAll('li');
  for (var i = 0; i != listItems.length; i++) {
    GridMode.Item.updateThumbnail(listItems[i], this.metadataCache_, item);
  }
};

/**
 * @param {Event} event Event.
 * @return {boolean} True if handled.
 */
GridMode.prototype.onKeyDown = function(event) {
  switch (util.getKeyModifiers(event) + event.keyIdentifier) {
    case 'Enter':
      this.openSelectedItem_();
      return true;
  }

  return false;
};

/**
 * @return {boolean} Always true (no tools fading in the Grid mode).
 */
GridMode.prototype.hasActiveTool = function() {
  return true;
};

/**
 * @param {Document} document Document.
 * @param {MetadataCache} metadataCache Metadata cache.
 * @param {Gallery.Item} item Item.
 * @return {Element} Newly created grid item.
 * @constructor
 */
GridMode.Item = function(document, metadataCache, item) {
  var li = document.createElement('li');
  li.__proto__ = GridMode.Item.prototype;
  li.className = 'thumbnail-item';
  li.item = item;

  var box = util.createChild(li, 'img-container');

  GridMode.Item.updateThumbnail(li, metadataCache, item);
  return li;
};

/**
 * @param {Element} li Grid item.
 * @param {MetadataCache} metadataCache Metadata cache.
 * @param {Gallery.Item} item Gallery item.
 */
GridMode.Item.updateThumbnail = function(li, metadataCache, item) {
  if (item != li.item)
    return;

  var box = li.querySelector('.img-container');
  var url = item.getUrl();
  metadataCache.get(url, Gallery.METADATA_TYPE,
      function(metadata) {
        new ThumbnailLoader(url, metadata).load(box, false /* fit */);
      });
};

GridMode.Item.prototype = {
  __proto__: cr.ui.ListItem.prototype,
  get label() {},
  set label(value) {}
};
