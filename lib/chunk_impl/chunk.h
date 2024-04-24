#pragma once

#include <unordered_set>
#include <optional>
#include <string>

#include <lib/document/document.h>

namespace lib {

class Chunk {
public:
    const std::string path;

    Chunk(const std::string path)
        : path(path) {
    }

    virtual std::vector<std::shared_ptr<document::Document>> Read(const std::optional<std::unordered_set<std::string>>& columns = std::nullopt) const = 0;
    virtual void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const = 0;
};

} // namespace lib
