/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/fluid/inference/tensorrt/convert/op_converter.h"

namespace paddle {
namespace inference {
namespace tensorrt {

/*
 * MulOp, IMatrixMultiplyLayer in TRT. This Layer doesn't has weights.
 */
class ConcatOpConverter : public OpConverter {
 public:
  void operator()(const framework::proto::OpDesc& op,
                  const framework::Scope& scope, bool test_mode) override {
    VLOG(3) << "convert a fluid mul op to tensorrt mul layer without bias";

    framework::OpDesc op_desc(op, nullptr);
    // Declare inputs
    std::vector<nvinfer1::ITensor*> itensors;
    for (auto& input_name : op_desc.Input("X")) {
      itensors.push_back(engine_->GetITensor(input_name));
    }
    int axis = boost::get<int>(op_desc.GetAttr("axis"));
    PADDLE_ENFORCE(axis > 0,
                   "The axis attr of Concat op should be large than 0 for trt");

    auto* layer = TRT_ENGINE_ADD_LAYER(engine_, Concatenation, itensors.data(),
                                       itensors.size());
    axis = axis - 1;  // Remove batch dim
    layer->setAxis(axis);
    auto output_name = op_desc.Output("Out")[0];
    layer->setName(("concat (Output: " + output_name + ")").c_str());
    layer->getOutput(0)->setName(output_name.c_str());
    engine_->SetITensor(output_name, layer->getOutput(0));
    if (test_mode) {  // the test framework can not determine which is the
                      // output, so place the declaration inside.
      engine_->DeclareOutput(output_name);
    }
  }
};

}  // namespace tensorrt
}  // namespace inference
}  // namespace paddle

REGISTER_TRT_OP_CONVERTER(concat, ConcatOpConverter);
