#pragma once

#include <stack>

#include <rapidjson/document.h>

#include <lib/chunk_impl/chunk.h>

namespace lib::chunk_impl {

class ColumnarChunk: public Chunk {
private:
    std::string schema_path;

public:
    ColumnarChunk(const std::string& chunk_path, const std::string& schema_path);

    std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const override;
    void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const override;
    rapidjson::Document ReadSchema() const;
};

} // namespace lib::chunk_impl
