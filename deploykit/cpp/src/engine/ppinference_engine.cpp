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


#include "include/deploy/engine/ppinference_engine.h"

namespace Deploy {

void PaddleInferenceEngine::Init(const std::string &model_filename,
                                const std::string &params_filename,
                                const PaddleInferenceConfig &engine_config) {
  paddle_infer::Config config;
  config.SetModel(model_filename, params_filename);
  if (engine_config.use_mkl && !engine_config.use_gpu) {
    config.EnableMKLDNN();
    config.SetCpuMathLibraryNumThreads(engine_config.mkl_thread_num);
    config.SetMkldnnCacheCapacity(10);
  }
  if (engine_config.use_gpu) {
    config.EnableUseGpu(100, engine_config.gpu_id);
  } else {
    config.DisableGpu();
  }
  config.SwitchUseFeedFetchOps(false);
  config.SwitchSpecifyInputNames(true);
  #if defined(__arm__) || defined(__aarch64__)
    config.SwitchIrOptim(false);
  #else
    config.SwitchIrOptim(engine_config.use_ir_optim);
  #endif
  config.EnableMemoryOptim();
  if (engine_config.use_trt && engine_config.use_gpu) {
    paddle_infer::PrecisionType precision;
    if (engine_config.precision == 0) {
      precision = paddle_infer::PrecisionType::kFloat32;
    } else if (engine_config.precision == 1) {
      precision = paddle_infer::PrecisionType::kHalf;
    } else if (engine_config.precision == 2) {
      precision = paddle_infer::PrecisionType::kInt8;
    } else {
      std::cerr << "Can not support the set precision" << std::endl;
    }
    config.EnableTensorRtEngine(
      1 << 10 /* workspace_size*/,
      engine_config.batch_size /* max_batch_size*/,
      engine_config.min_subgraph_size /* min_subgraph_size*/,
      precision /* precision*/,
      engine_config.use_static /* use_static*/,
      engine_config.use_calib_mode /* use_calib_mode*/);
  }
  predictor_ = std::move(paddle_infer::CreatePredictor(config));
}

void PaddleInferenceEngine::Infer(const std::vector<DataBlob> &inputs,
                                  std::vector<DataBlob> *outputs) {
  for (int i = 0; i < inputs.size(); i++) {
    auto in_tensor = predictor_->GetInputHandle(inputs[i].name);
    in_tensor->Reshape(inputs[i].shape);
    if (inputs[i].dtype == 0) {
      float *im_tensor_data;
      im_tensor_data = (float*)(inputs[i].data.data());  // NOLINT
      in_tensor->CopyFromCpu(im_tensor_data);
    } else if (inputs[i].dtype == 1) {
      int64_t *im_tensor_data;
      im_tensor_data = (int64_t*)(inputs[i].data.data());  // NOLINT
      in_tensor->CopyFromCpu(im_tensor_data);
    } else if (inputs[i].dtype == 2) {
      int *im_tensor_data;
      im_tensor_data = (int*)(inputs[i].data.data());  // NOLINT
      in_tensor->CopyFromCpu(im_tensor_data);
    } else if (inputs[i].dtype == 3) {
      uint8_t *im_tensor_data;
      im_tensor_data = (uint8_t*)(inputs[i].data.data());  // NOLINT
      in_tensor->CopyFromCpu(im_tensor_data);
    }
  }
  // predict
  predictor_->Run();

  // get output
  auto output_names = predictor_->GetOutputNames();
  for (const auto output_name : output_names) {
    auto output_tensor = predictor_->GetOutputHandle(output_name);
    auto output_tensor_shape = output_tensor->shape();
    DataBlob output;
    output.name = output_name;
    output.shape.assign(output_tensor_shape.begin(), output_tensor_shape.end());
    output.dtype = paddle_infer::DataType(output_tensor->type());
    output.lod = output_tensor->lod();
    int size = 1;
    for (const auto& i : output_tensor_shape) {
      size *= i;
    }
    if (output.dtype == 0) {
      output.data.resize(size * sizeof(float));
      output_tensor->CopyToCpu(reinterpret_cast<float*>(output.data.data()));
    } else if (output.dtype == 1) {
      output.data.resize(size * sizeof(int64_t));
      output_tensor->CopyToCpu(
        reinterpret_cast<int64_t*>(output.data.data()));
    } else if (output.dtype == 2) {
      output.data.resize(size * sizeof(int));
      output_tensor->CopyToCpu(reinterpret_cast<int*>(output.data.data()));
    } else if (output.dtype == 3) {
      output.data.resize(size * sizeof(uint8_t));
      output_tensor->CopyToCpu(
        reinterpret_cast<uint8_t*>(output.data.data()));
    }
    outputs->push_back(std::move(output));
  }
}

}  //  namespace Deploy
