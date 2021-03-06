// Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <memory>

#include "include/deploy/common/blob.h"
#include "include/deploy/common/transforms.h"
#include "include/deploy/common/config.h"

namespace Deploy {

class BasePreprocess {
 public:
  virtual bool Init(const ConfigParser &parser) = 0;

  virtual bool Run(const std::vector<cv::Mat> &imgs,
                  std::vector<DataBlob> *inputs,
                  std::vector<ShapeInfo> *shape_infos) = 0;

 protected:
  bool BuildTransform(const ConfigParser &parser);

  bool RunTransform(std::vector<cv::Mat> *imgs);

  bool ShapeInfer(const std::vector<cv::Mat> &imgs,
                  std::vector<ShapeInfo> *shape_infos);

  std::vector<std::shared_ptr<Transform>> transforms;

  std::vector<int> GetMaxSize();

 private:
  int max_w_ = 0;
  int max_h_ = 0;
  std::shared_ptr<Transform> CreateTransform(const std::string &name);
};

}  // namespace Deploy
