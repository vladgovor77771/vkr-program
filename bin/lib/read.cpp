#include "read.h"

#include <chrono>
#include <iostream>
#include <sstream>

#include <bin/lib/common.h>
#include <lib/chunk_impl/io.h>

namespace cli {

namespace {

    std::vector<std::string> SplitByDots(const std::string& input) {
        std::vector<std::string> result;
        std::string current;
        bool escape = false;

        for (char ch : input) {
            if (escape) {
                current += ch;
                escape = false;
            } else {
                if (ch == '\\') {
                    escape = true;
                } else if (ch == '.') {
                    result.push_back(current);
                    current.clear();
                } else {
                    current += ch;
                }
            }
        }

        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }

    void SetNodes(const std::string& str, const lib::chunk_impl::TreeNodePtr& root) {
        auto node = root;
        auto splited = SplitByDots(str);

        for (const auto& part : splited) {
            if (node->children.find(part) == node->children.end()) {
                node->children[part] = lib::chunk_impl::TreeNode::Default();
            }
            node = node->children[part];
        }
    }

    lib::chunk_impl::TreeNodePtr BuildPrefixTree(std::string&& csv_columns, std::string&& csv_columns_file) {
        auto root = lib::chunk_impl::TreeNode::Default();
        if (csv_columns.empty() && csv_columns_file.empty()) {
            return root;
        }

        std::istringstream iss;
        if (csv_columns_file.empty()) {
            iss = std::istringstream(std::move(csv_columns));
        } else {
            auto reader = lib::chunk_impl::GetInputStream(std::move(csv_columns_file));
            auto cols = reader->ReadLine();
            if (cols.empty()) {
                return root;
            }
            iss = std::istringstream(std::move(cols));
        }
        std::string column;

        while (getline(iss, column, ',')) {
            SetNodes(column, root);
        }

        return root;
    }

} // namespace

void RunRead(ReadArgs&& args) {
    const auto chunk = GetChunk(std::move(args.path), std::move(args.format), std::move(args.schema_path));
    const auto columns_tree = BuildPrefixTree(std::move(args.columns), std::move(args.columns_file));

    const auto start = std::chrono::high_resolution_clock::now();
    const auto documents = chunk->Read(columns_tree);
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto duration_read = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    std::cerr << "{\"read_duration_ns\": " << duration_read.count() << "}\n";

    if (args.write_to_stdout) {
        const auto output_chunk = GetChunk("stdout", "json", "");
        output_chunk->Write(documents);
    }
}

} // namespace cli
