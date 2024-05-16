#include "get_stream.h"

#include <sys/mman.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace lib::chunk_impl {

MmapFile::MmapFile(const char* filename) {
    fd_ = open(filename, O_RDONLY);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to open file");
    }

    file_size_ = lseek(fd_, 0, SEEK_END);
    if (file_size_ == -1) {
        close(fd_);
        throw std::runtime_error("Failed to determine file size");
    }

    data_ = static_cast<char*>(
        mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0));
    if (data_ == MAP_FAILED) {
        close(fd_);
        throw std::runtime_error("Failed to mmap file");
    }

    current_pos_ = 0;
}

MmapFile::~MmapFile() {
    if (data_ != MAP_FAILED) {
        munmap(data_, file_size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}

void MmapFile::Seekg(int offset, std::ios_base::seekdir dir) {
    if (dir == std::ios_base::beg) {
        current_pos_ = offset;
    } else if (dir == std::ios_base::cur) {
        current_pos_ += offset;
    } else if (dir == std::ios_base::end) {
        current_pos_ = file_size_ + offset;
    }
    if (current_pos_ < 0 || current_pos_ > file_size_) {
        throw std::out_of_range("Seek position out of range");
    }
}

int MmapFile::Peek() const {
    if (current_pos_ < file_size_) {
        return static_cast<char>(data_[current_pos_]);
    }
    return EOF;
}

std::size_t MmapFile::Tellg() const {
    return current_pos_;
}

bool MmapFile::Eof() const {
    return current_pos_ >= file_size_;
}

void MmapFile::Read(char* buffer, std::size_t length) {
    if (current_pos_ + length > file_size_) {
        throw std::out_of_range("Read exceeds file size");
    }
    std::memcpy(buffer, data_ + current_pos_, length);
    current_pos_ += length;
}

void MmapFile::Get(char& ch) {
    if (current_pos_ < file_size_) {
        ch = data_[current_pos_++];
    } else {
        throw std::out_of_range("Get position out of range");
    }
}

std::string MmapFile::ReadLine() {
    std::string line;
    while (current_pos_ < file_size_ && data_[current_pos_] != '\n') {
        line += data_[current_pos_++];
    }
    if (current_pos_ < file_size_ && data_[current_pos_] == '\n') {
        current_pos_++;
    }
    return line;
}

void StdinStream::Seekg(int offset, std::ios_base::seekdir dir) {
    std::cin.seekg(offset, dir);
}
int StdinStream::Peek() const {
    return std::cin.peek();
}
std::size_t StdinStream::Tellg() const {
    return std::cin.tellg();
}
bool StdinStream::Eof() const {
    return std::cin.eof();
}
void StdinStream::Read(char* buffer, std::size_t length) {
    std::cin.read(buffer, length);
}
void StdinStream::Get(char& ch) {
    std::cin.get(ch);
}
std::string StdinStream::ReadLine() {
    std::string res;
    std::cin >> res;
    return res;
}

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

std::shared_ptr<IStream> GetInputStream(const std::string& path) {
    if (path == "-") {
        return std::make_shared<StdinStream>();
    } else {
        return std::make_shared<MmapFile>(path.c_str());
    }
}

} // namespace lib::chunk_impl
