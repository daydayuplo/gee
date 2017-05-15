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

// TODO: High-level file comment.
/****************************************************

Copyright (c) 2012-2013, Tom MacWright 
All rights reserved.

Source of library: https://github.com/tmcw/togeojson

License: https://github.com/tmcw/togeojson/blob/gh-pages/LICENSE

----------------------------------------------------*/

toGeoJSON = {
    kml: function(doc, o) {
        o = o || {};
        var gj = { type: 'FeatureCollection', features: [] }, styleIndex = {},
            geotypes = ['Polygon', 'LineString', 'Point'],
            removeSpace = (/\s*/g),
            trimSpace = (/^\s*|\s*$/g), splitSpace = (/\s+/),
            placemarks = get(doc, 'Placemark'), styles = get(doc, 'Style');

        if (o.styles) for (var k = 0; k < styles.length; k++) {
            styleIndex['#' + styles[k].id] = okhash(styles[k].innerHTML).toString(16);
        }
        for (var j = 0; j < placemarks.length; j++) {
            gj.features = gj.features.concat(getPlacemark(placemarks[j]));
        }
        function okhash(x) {
            if (!x || !x.length) return 0;
            for (var i = 0, h = 0; i < x.length; i++) {
                h = ((h << 5) - h) + x.charCodeAt(i) | 0;
            } return h;
        }
        function get(x, y) { return x.getElementsByTagName(y); }
        function get1(x, y) { var n = get(x, y); return n.length ? n[0] : null; }
        function numarray(x) {
            for (var j = 0, o = []; j < x.length; j++) o[j] = parseFloat(x[j]);
            return o;
        }
        function nodeVal(x) { return x && x.firstChild.nodeValue; }
        function coord1(v) { return numarray(v.replace(removeSpace, '').split(',')); }
        function coord(v) {
            var coords = v.replace(trimSpace, '').split(splitSpace), o = [];
            for (var i = 0; i < coords.length; i++) o.push(coord1(coords[i]));
            return o;
        }
        function getGeometry(root) {
            var geomNode, geomNodes, i, j, k, geoms = [];
            if (get1(root, 'MultiGeometry')) return getGeometry(get1(root, 'MultiGeometry'));
            for (i = 0; i < geotypes.length; i++) {
                geomNodes = get(root, geotypes[i]);
                if (geomNodes) {
                    for (j = 0; j < geomNodes.length; j++) {
                        geomNode = geomNodes[j];
                        if (geotypes[i] == 'Point') {
                            geoms.push({ type: 'Point',
                                coordinates: coord1(nodeVal(get1(geomNode, 'coordinates')))
                            });
                        } else if (geotypes[i] == 'LineString') {
                            geoms.push({ type: 'LineString',
                                coordinates: coord(nodeVal(get1(geomNode, 'coordinates')))
                            });
                        } else if (geotypes[i] == 'Polygon') {
                            var rings = get(geomNode, 'LinearRing'), coords = [];
                            for (k = 0; k < rings.length; k++) {
                                coords.push(coord(nodeVal(get1(rings[k], 'coordinates'))));
                            }
                            geoms.push({ type: 'Polygon', coordinates: coords });
                        }
                    }
                }
            }
            return geoms;
        }
        function getPlacemark(root) {
            var geoms = getGeometry(root), i, properties = {},
                name = nodeVal(get1(root, 'name')),
                styleUrl = nodeVal(get1(root, 'styleUrl')),
                description = nodeVal(get1(root, 'description')),
                extendedData = get1(root, 'ExtendedData');
            if (!geoms.length) return false;
            if (name) properties.name = name;
            if (styleUrl && styleIndex[styleUrl]) {
                properties.styleUrl = styleUrl;
                properties.styleHash = styleIndex[styleUrl];
            }
            if (description) properties.description = description;
            if (extendedData) {
                var datas = get(extendedData, 'Data');
                for (i = 0; i < datas.length; i++) {
                    properties[datas[i].getAttribute('name')] = nodeVal(get1(datas[i], 'value'));
                }
            }
            return [{ type: 'Feature', geometry: (geoms.length === 1) ? geoms[0] : {
                type: 'GeometryCollection',
                geometries: geoms }, properties: properties }];
        }
        return gj;
    }
};

if (typeof module !== 'undefined') module.exports = toGeoJSON;
