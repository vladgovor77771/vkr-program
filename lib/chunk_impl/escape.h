#pragma once

#include <fstream>
#include <iostream>
#include <string>

namespace lib::chunk_impl {

std::string EscapeDot(const std::string& str);
std::string EscapeNewLines(const std::string& str);

} // namespace lib::chunk_impl
