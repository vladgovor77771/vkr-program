#pragma once

#include <string>

namespace cli {

struct TransformArgs {
    std::string input_path;
    std::string input_format;
    std::string output_path;
    std::string output_format;
};

void RunTransform(TransformArgs&& args);

} // namespace cli
