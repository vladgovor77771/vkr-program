#include "read.h"

#include <sstream>
#include <iostream>

#include <bin/common.h>

namespace cli {

namespace {

std::optional<std::unordered_set<std::string>> BuildColumnsSet(std::string&& csv_columns) {
    if (csv_columns.empty()) {
        return std::nullopt;
    }
    std::unordered_set<std::string> columns;
    std::istringstream iss(std::move(csv_columns));
    std::string column;

    while (getline(iss, column, ',')) {
        if (column[0] != '.') {
            column = '.' + column;
        }
        columns.insert(std::move(column));
    }

    return columns;
}

}

void RunRead(ReadArgs&& args) {
    auto chunk = GetChunk(std::move(args.path), std::move(args.format));
    auto columns = BuildColumnsSet(std::move(args.columns));
    auto documents = chunk->Read(columns);

    std::cout << documents.size() << std::endl;

    if (args.write_to_stdout) {
        auto output_chunk = GetChunk("stdout", "json");
        output_chunk->Write(documents);
    }
}

} // namespace cli
