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

// Note: need to keep this synced with MercatorImageryAsset.cpp

#include <autoingest/khAssetManagerProxy.h>
#include "RasterAssetWidget.h"
#include "ImageryAsset.h"
#include "AssetDerivedImpl.h"

// ****************************************************************************
// ***  ImageryAssetDefs
// ****************************************************************************
ImageryAssetDefs::SubmitFuncType ImageryAssetDefs::kSubmitFunc =
     &khAssetManagerProxy::RasterProductImport;

// ****************************************************************************
// ***  ImageryAsset
// ****************************************************************************
ImageryAsset::ImageryAsset(QWidget* parent) :
    AssetDerived<ImageryAssetDefs, ImageryAsset>(parent)
{ }

ImageryAsset::ImageryAsset(QWidget* parent, const Request& req) :
    AssetDerived<ImageryAssetDefs, ImageryAsset>(parent, req)
{ }
    
ImageryAssetDefs::Request ImageryAsset::FinalMakeNewRequest(void) {
  ImageryAssetDefs::Request ret_val =
      Request(AssetDisplayHelper::AssetType(kAssetDisplayKey));
  ret_val.config.useMercatorProjection = false;
  return ret_val;
}

// Explicit instantiation of base class
template class AssetDerived<ImageryAssetDefs, ImageryAsset>;
