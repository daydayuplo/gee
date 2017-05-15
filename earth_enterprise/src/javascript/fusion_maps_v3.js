// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview This file presents an example of a page that makes use of the
 *               Google Maps API from a Google Earth Enterprise Server. It
 *               handles the following: examples of how to use the Google Maps
 *               API, a simple layer panel for the layers provided by the
 *               GEE Server search tabs from the GEE Server Search Framework
 *               search results from the GEE Server.
 *
 * This file contains most of the work to customize the look and feel and
 * behaviors in the page as well as the calls to the Maps API. Depends on :
 *   fusion_utils.js : some basic utilities for accessing DOM elements, and a
 *                     few low level utilities used by this example.
 *   search_tabs.js : sample classes for showing/managing search tabs and
 *                    results.
 *
 * ASSUMES:
 *   GEE_SERVER_HOSTNAME is set
 */

/**
 * Load the specified javascript.
 * @param {string}
 *          path the path relative to the current web page of the javascript.
 */
function geeLoadScript(path) {
  document.write('<script src="' + path +
                 '" type="text/javascript"><\/script>');
}

// Load the Maps database info into the JS global variable 'geeServerDefs'.
// This will have the fields:
//  serverUrl : the URL of the server
//  isAuthenticated : true if the server is authenticated
//  searchTabs : a JSON array of search tab definitions.
//  layers : a JSON array of supplemental layer info
// If desired, you can point this to another GEE Server or use a different
// variable for multiple servers.

if (typeof(GEE_SERVER_URL) == 'undefined') {
  var GEE_SERVER_URL = '';
}
geeLoadScript(GEE_SERVER_URL + 'query?request=Json&var=geeServerDefs');

//The div ids for the left panel, map and header.
//These must match the html and css declarations.
var geeDivIds = {
 header: 'header',
 map: 'map',
 mapInner: 'map_inner',
 leftPanelParent: 'left_panel_cell',
 leftPanel: 'left_panel',
 searchTabs: 'search_tabs',
 searchTitle: 'search_results_title',
 searchResults: 'search_results_container',
 layersTitle: 'layers_title',
 layers: 'layers_container',
 collapsePanel: 'collapsePanel',
 collapseShim: 'collapseShim'
};
//Need the static URL for absolute addresses
var GEE_STATIC_URL = window.location.protocol + '//' + window.location.host;
//Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_STATIC_URL + '/earth/images/';
var GEE_MAPS_IMAGE_PATH = GEE_STATIC_URL + '/maps/api/icons/';

//Search Timeout specifies how long to wait (in milliseconds) before
//determining the search failed.
//override with "?search_timeout=xxx" on the URL where xxx is in milliseconds.
var geeSearchTimeout = 5000;  // 5 seconds seems OK

//Initial view parameters are overrideable from the html or the URL.
//Override initial lat lng with "?ll=32.898,-100.30" on the URL.
//Override initial zoom level with "?z=8" on URL
var geeInitialViewLat = 37.422;
var geeInitialViewLon = -122.08;
var geeInitialZoomLevel = 6;

//Default Altitude for panning, keep camera at a high enough level to see
//continents.
var DEFAULT_SINGLE_CLICK_ZOOM_LEVEL = 10;  // City Level.
var DEFAULT_DOUBLE_CLICK_ZOOM_LEVEL = 14;  // Neighborhood level.
var DEFAULT_PAN_TO_ALTITUDE_METERS = 400000;
var DEFAULT_BALLOON_MAX_WIDTH_PIXELS = 500;
//We cache a map of Maps Zoom Levels (0..32) to Earth camera altitudes.
var geeZoomLevelToCameraAltitudeMap = null;
var geeIconUrls = {};  // Cache of frequently used URLs.

// Google Maps specific variables.
var geeMap = null;
var geeBaseIcon = 0;  // To be initialized in LoadMap().

var _mFusionMapServer = '';  // This constant is used by other
                             // GEE Maps API code.

/**
 * Init the Google Maps Map widget.
 * This will be executed onLoad of the document and will
 * initialize: 1) the map 2) the layer panel 3) the search tabs
 *
 * @param {object} geeMapOpts (optional)
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @param {boolean} showSearch (optional)
 *    Whether to show search tab(s). Default is true.
 */

function geeInitMap(geeMapOpts, showSearch) {
  // check that geeServerDefs are loaded.
  if (typeof(geeServerDefs) == 'undefined') {
    alert('Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
    return;
  }

  // Show search by default.
  if (typeof showSearch == 'undefined') {
    showSearch = true;
  }

  // Cache the folder icon URLs.
  geeIconUrls = {
      openedFolder: geeEarthImage('openfolder.png'),
      closedFolder: geeEarthImage('closedfolder.png'),
      cancelButton: geeEarthImage('cancel.png'),
      collapse: geeMapsImage('collapse.png'),
      expand: geeMapsImage('expand.png'),
      transparent: geeMapsImage('transparent.png'),
      defaultLayerIcon: geeEarthImage('default_layer_icon.png')
  };

  // If no search tabs are defined, we have a default search tab for jumping
  // to a lat lng (without hitting the server).
  // You can also hard code handy search tab functionality here by appending
  // to the existing geeServerDefs["searchTabs"].
  if (showSearch && (geeServerDefs.searchTabs == '')) {
    // Create a default search tab if none are defined by the server.
    geeServerDefs['searchTabs'] = geeDefaultSearchTabs();
  }

  // Must set this variable to bootstrap the local version of the Maps API.
  _mFusionMapServer = GEE_STATIC_URL;

  geeInitLeftPanelDivs();  // The left panel needs some sub-divs for layers etc.

  // Need to resize before initial load of the map! and then again
  // to adjust for the search tabs if any.
  geeResizeDivs();

  // Create the Map...this will create the Map, initialize the layers
  // and search tabs and pan the view to the initial view.
  geeLoadMap(geeServerDefs, geeMapOpts);

  if (showSearch) {
    // Initialize the search tabs.
    // Check for a search_timeout override.
    if (getPageURLParameter('search_timeout')) {
      geeSearchTimeout = getPageURLParameter('search_timeout');
    }
    initializeSearch(geeDivIds.searchTabs, geeServerDefs.searchTabs,
                     geeSearchTimeout);
  }

  // Need to resize after search tabs are filled in.
  geeResizeDivs();  // Resize the map to fill the screen.
}

/**
 * Initialize the Map UI: the map, layers and search UI.
 * @param {object} serverDefs
 *    The object that contains the search and layer info for initializing
 *    the Map UI.
 * @param {object} geeMapOpts (optional)
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 */
function geeLoadMap(serverDefs, geeMapOpts) {
  // Create the map.
  geeMap = new GFusionMap(geeDivIds.map, serverDefs, geeMapOpts);

  // Load the layers into the UI.
  geeInitLayerList(serverDefs.serverUrl, geeDivIds.layers, serverDefs.layers);
}

/**
 * Fill in the LayerDiv with a list of checkboxes for the layers from the
 * current Google Earth Database. Each checkbox refers to a geeToggleLayer
 * callback.
 *
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {String}
 *          layerDivId the div id of the layer panel.
 * @param {Array}
 *          layers the array of layers.
 */
function geeInitLayerList(serverUrl, layerDivId, layers) {
  var layerDiv = document.getElementById(layerDivId);
  var layerList = createElement(layerDiv, 'ul');
  layerList.className = 'layer_list';
  geeCreateLayerElements(serverUrl, layerList, layers);
}

/**
 * Fill in the given list with list items for each child layer.
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Element}
 *          parentList the DOM list element to add these layers to.
 * @param {Array}
 *          childLayers the child layers to be added to this list element.
 */
function geeCreateLayerElements(serverUrl, parentList,
                                childLayers) {
  for (var i = 0; i < childLayers.length; ++i) {
    var layer = childLayers[i];
    if (i == 0 && layer.requestType == 'ImageryMaps') {
      continue;  // Skip the first imagery layer...it will have a button.
    }
    var item = geeCreateLayerItem(serverUrl, parentList, layer);
  }
}

/**
 * Create a single layer item entry with a checkbox and appropriate hover and
 * click behaviors.
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Element}
 *          layerList the list element container for the item being created.
 * @param {KmlLayer}
 *          layer the layer object.
 * @return {Element} the list item for the layer.
 */
function geeCreateLayerItem(serverUrl, layerList, layer) {
  var isChecked = layer.initialState;
  var layerId = layer.id;
  var layerName = layer.label;
  var layerIconUrl = geeLayerIconUrl(serverUrl, layer);

  // Create the list item for the layer.
  var item = geeCreateLayerItemBasic(layerList, layerId, layerName,
                                     layerIconUrl, isChecked, 'geeToggleLayer');

  // Set to zoom in on layer on click
  item.onclick = function(layerObject) {
    return function(e) {
      var lookAt = layerObject.lookAt;
      if (lookAt == 'none') return;
      geeMap.panTo(lookAt.lat, lookAt.lng, lookAt.zoom);
      cancelEvent(e);
    };
  }(layer);

  return item;
}

/**
 * Toggle the visibility of the specified layer.
 * @param {Object}
 *          e  the event argument.
 * @param {string}
 *          checkBoxId the name of the checkbox which maintains the layer's
 *          visibility state.
 * @param {string}
 *          layerId the id of the layer to toggle.
 * @param {string}
 *          layerName the name of the layer to toggle only used for printing
 *          error message.
 */
function geeToggleLayer(e, checkBoxId, layerId, layerName) {
  try {
    var cb = document.getElementById(checkBoxId);
    try {
      if (cb.checked) {
        geeMap.showFusionLayer(layerId);
      } else {
        geeMap.hideFusionLayer(layerId);
      }
    } catch (err2) {
      alert('Failed attempt to enable/disable layer: ' +
            layerName + '\n' + layerId + '\n' + err2);
    }
  } catch (err) {
    alert('Failed attempt to get checkbox for layer: ' +
          layerName + '\n' + err);
  }
  cancelEvent(e);
}

/**
 * Pan and Zoom the Map viewer to the specified lat, lng and zoom level.
 * @param {string}
 *          lat the latitude of the position to pan to.
 * @param {string}
 *          lng the longitude of the position to pan to.
 * @param {Number}
 *          zoomLevel [optional] the zoom level (an integer between 1 : zoomed
 *          out all the way, and 32: zoomed in all the way) indicating the zoom
 *          level for the view.
 */
function geePanTo(lat, lng, zoomLevel) {
  geeMap.panTo(lat, lng, zoomLevel);
}

/**
 * Close the open info window.
 */
function geeCloseInfoWindow() {
  geeMap.closeInfoWindow();
}

/**
 * Open a balloon in the Map.
 * @param {Object}
 *          marker the marker that is attached to the balloon.
 * @param {Object}
 *          balloon the balloon object (no such thing in Maps API).
 * @param {string}
 *          title the title of the balloon contents.
 * @param {string}
 *          innerText the inner text of the balloon text.
 */
function geeOpenBalloon(marker, balloon, title, innerText) {
  // Create the info balloon html here.
  geeMap.openInfoWindow(marker.getPosition(),
                        '<b>' + title + '</b><br/>' + innerText);
}

/*
 * Interface for search_tab.js: The following code implements the GEE Methods
 * required by search_tabs.js.
 */

/**
 * Deprecated. This functionality is now incorporated into geeOpenBalloon.
 * @param {string} opt_name Balloon title.
 * @param {string} opt_description Balloon content.
 */
function geeCreateBalloon(opt_name, opt_description) {
}

/**
 * Create a placemark at the specified location.
 * @param {string}
 *          name the name of the placemark.
 * @param {string}
 *          description the description of the placemark.
 * @param {google.maps.LatLng}
 *          latlng the position of the placemark.
 * @param {string}
 *          iconUrl the URL of the placemark's icon.
 * @param {Object}
 *          balloon the balloon for the placemark (ignored for Maps API).
 * @return {google.maps.Marker} the placemark object.
 */
function geeCreateMarker(name, description, latlng, iconUrl, balloon) {
  // Create icon style for the placemark
  var point = new google.maps.LatLng(parseFloat(latlng.lat),
                                     parseFloat(latlng.lon));
  var marker = new google.maps.Marker({
        icon: iconUrl,
        map: map,
        position: point,
        draggable: false,
        title: name
      });

  google.maps.event.addListener(marker, 'click', function() {
        geeOpenBalloon(marker, null, name, description);
      });
  return marker;
}

 /**
  * Callback for Search Results Manager after results are cleared.
  * We simply clear any active info window.
  */
 function geeClearSearchResults() {
   geeMap.closeInfoWindow();
 }

 /**
  * Deprecated.
  * @param {google.maps.Marker}
  *          marker The marker to be added.
  */
 function geeAddOverlay(marker) {
 }

 /**
  * Removes the marker from the map.
  * @param {google.maps.Marker}
  *          marker The marker to be removed.
  */
 function geeRemoveOverlay(marker) {
   marker.setMap(null);
 }

/**
 * Return the URL for a layer icon (a specific request to the GEE Server).
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Array}
 *          layer the layer object.
 * @return {string} the kml feature object.
 */
function geeLayerIconUrl(serverUrl, layer) {
  return serverUrl + '/query?request=Icon&icon_path=' + layer.icon;
}
