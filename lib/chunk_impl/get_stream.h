#pragma once

#include <fstream>
#include <iostream>
#include <memory>

namespace lib::chunk_impl {

struct OStreamDeleter {
    void operator()(std::ostream* ptr) const;
};

struct IStreamDeleter {
    void operator()(std::istream* ptr) const;
};

std::shared_ptr<std::ostream> GetOutputStream(const std::string& path);
std::shared_ptr<std::istream> GetInputStream(const std::string& path);

} // namespace lib::chunk_impl
