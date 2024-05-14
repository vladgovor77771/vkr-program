#pragma once

#include <string>

namespace cli {

struct DatasetGeneratorArgs {
    std::string output_path;
    std::string schema_path;
    double sparsity;
    std::size_t docs_count;
    int min_list_size;
    int max_list_size;
};

struct SchemaGeneratorArgs {
    std::string output_path;
    std::size_t depth;
    int min_keys_count;
    int max_keys_count;
    int min_keys_length;
    int max_keys_length;
};

void RunGenerateDataset(DatasetGeneratorArgs&& args);
void RunGenerateSchema(SchemaGeneratorArgs&& args);

} // namespace cli
