#pragma once

#include <unordered_map>
#include <string>

namespace lib::chunk_impl {

struct TreeNode {
    std::unordered_map<std::string, std::shared_ptr<TreeNode>> children;

    bool IsLeaf() const {
        return children.empty();
    }
    static std::shared_ptr<TreeNode> Default() {
        return std::make_shared<TreeNode>();
    }
};

using TreeNodePtr = std::shared_ptr<TreeNode>;

} // namespace lib::chunk_impl
