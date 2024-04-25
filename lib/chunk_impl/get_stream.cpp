#include "get_stream.h"

namespace lib::chunk_impl {

void OStreamDeleter::operator()(std::ostream* ptr) const {
    if (ptr != &std::cout) {
        std::ofstream* ofs = dynamic_cast<std::ofstream*>(ptr);
        if (ofs) {
            ofs->close();
            delete ofs;
        }
    }
}

std::shared_ptr<std::ostream> GetOutputStream(const std::string& path) {
    if (path == "stdout") {
        return std::shared_ptr<std::ostream>(&std::cout, OStreamDeleter());
    } else {
        std::ofstream* ofs = new std::ofstream(path);
        if (!ofs->is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        return std::shared_ptr<std::ostream>(ofs, OStreamDeleter());
    }
}

void IStreamDeleter::operator()(std::istream* ptr) const {
    if (ptr != &std::cin) {
        std::ifstream* ifs = dynamic_cast<std::ifstream*>(ptr);
        if (ifs) {
            ifs->close();
            delete ifs;
        }
    }
}

std::shared_ptr<std::istream> GetInputStream(const std::string& path) {
    if (path == "stdin") {
        return std::shared_ptr<std::istream>(&std::cin, IStreamDeleter());
    } else {
        std::ifstream* ofs = new std::ifstream(path);
        if (!ofs->is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        return std::shared_ptr<std::istream>(ofs, IStreamDeleter());
    }
}

} // namespace lib::chunk_impl
