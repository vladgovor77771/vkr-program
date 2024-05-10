#include "get_stream.h"

namespace lib::chunk_impl {

void OStreamDeleter::operator()(std::ostream* ptr) const {
    if (ptr != &std::cout) {
        std::ofstream* ofs = dynamic_cast<std::ofstream*>(ptr);
        if (ofs) {
            ofs->flush();
            ofs->close();
            delete ofs;
        }
    }
}

std::shared_ptr<std::ostream> GetOutputStream(const std::string& path) {
    if (path == "stdout") {
        return std::shared_ptr<std::ostream>(&std::cout, OStreamDeleter());
    } else {
        OStreamDeleter deleter;
        deleter.buffer.resize(32 * 1024);
        std::ofstream* ofs = new std::ofstream();
        ofs->rdbuf()->pubsetbuf(deleter.buffer.data(), deleter.buffer.size());
        ofs->open(path);
        if (!ofs->is_open()) {
            throw std::runtime_error("Failed to open output file: " + path);
        }
        return std::shared_ptr<std::ostream>(ofs, std::move(deleter));
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
        IStreamDeleter deleter;
        deleter.buffer.resize(32 * 1024);
        std::ifstream* ifs = new std::ifstream();
        ifs->rdbuf()->pubsetbuf(deleter.buffer.data(), deleter.buffer.size());
        ifs->open(path);
        if (!ifs->is_open()) {
            throw std::runtime_error("Failed to open input file: " + path);
        }
        return std::shared_ptr<std::istream>(ifs, std::move(deleter));
    }
}

} // namespace lib::chunk_impl
