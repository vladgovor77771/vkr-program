#pragma once

#include <unordered_set>
#include <optional>
#include <string>

#include <lib/chunk_impl/prefix_tree.h>
#include <lib/document/document.h>

namespace lib::chunk_impl {

class Chunk {
public:
    const std::string path;

    Chunk(const std::string path)
        : path(path) {
    }

    virtual std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const = 0;
    virtual void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const = 0;
};

} // namespace lib::chunk_impl
