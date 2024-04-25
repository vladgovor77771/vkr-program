#pragma once

#include <lib/chunk_impl/chunk.h>

namespace lib::chunk_impl {

enum class BsonControlChars {
    kNullFlag = 'n',     // length 0
    kBooleanFlag = 'b',  // length 4
    kInt32Flag = 'i',    // length 4
    kUint32Flag = 'u',   // length 4
    kInt64Flag = 'g',    // length 8
    kUint64Flag = 'z',   // length 8
    kFloat32Flag = 'f',  // length 4
    kFloat64Flag = 'd',  // length 8
    kStringFlag = 's',   // length vary
    kDocumentFlag = 'o', // length vary
    kListFlag = 'l',     // length vary
};

class BsonChunk: public Chunk {
public:
    BsonChunk(const std::string& path)
        : Chunk(path) {
    }

    std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const override;
    void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const override;
};

} // namespace lib::chunk_impl
