#include "escape.h"

namespace lib::chunk_impl {

std::string EscapeDot(const std::string& str) {
    std::string res;
    res.reserve(str.length());
    for (auto c : str) {
        if (c == '.') {
            res += "\\.";
        } else if (c == '\\') {
            res += "\\\\";
        } else {
            res += c;
        }
    }
    return res;
}

std::string EscapeNewLines(const std::string& str) {
    std::string res;
    res.reserve(str.length());
    for (auto c : str) {
        if (c == '\n') {
            res += "\\\n";
        } else if (c == '\\') {
            res += "\\\\";
        } else {
            res += c;
        }
    }
    return res;
}

} // namespace lib::chunk_impl
