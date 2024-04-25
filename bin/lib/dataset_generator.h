#pragma once

#include <string>

namespace cli {

struct DatasetGeneratorArgs {
    std::string output_path;
    size_t depth;
    double sparsity;
    size_t min_keys_count;
    size_t max_keys_count;
    size_t min_keys_length;
    size_t max_keys_length;
};

void RunGenerateDataset(DatasetGeneratorArgs&& args);

} // namespace cli
