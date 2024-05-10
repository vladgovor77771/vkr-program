#pragma once

#include <lib/chunk_impl/chunk.h>

namespace lib::chunk_impl {

class BsonChunk: public Chunk {
public:
    BsonChunk(const std::string& path)
        : Chunk(path) {
    }

    std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const override;
    void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const override;
};

} // namespace lib::chunk_impl
