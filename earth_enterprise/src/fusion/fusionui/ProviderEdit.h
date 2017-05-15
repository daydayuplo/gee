/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef KHSRC_FUSION_FUSIONUI_PROVIDEREDIT_H__
#define KHSRC_FUSION_FUSIONUI_PROVIDEREDIT_H__

#include <providereditbase.h>
#include <autoingest/.idl/gstProvider.h>

class ProviderEdit : public ProviderEditBase {
 public:
  ProviderEdit(QWidget* parent);
  int configure(const gstProvider&);
  gstProvider getProvider() const;

 private:
  gstProvider orig_;

  // inherited from ProviderEditBase
  virtual void nameChanged(const QString&);
};

#endif  // !KHSRC_FUSION_FUSIONUI_PROVIDEREDIT_H__
