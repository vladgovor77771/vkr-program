#include "io.h"

#include <sys/mman.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace lib::chunk_impl {

MmapFileReader::MmapFileReader(const char* filename)
    : current_pos_(0) {
    fd_ = open(filename, O_RDONLY);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("Failed to open input file: ") + filename + " " + strerror(errno));
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
}

MmapFileReader::~MmapFileReader() {
    if (data_ != MAP_FAILED) {
        munmap(data_, file_size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}

void MmapFileReader::Seekg(int offset, std::ios_base::seekdir dir) {
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

int MmapFileReader::Peek() const {
    if (current_pos_ < file_size_) {
        return static_cast<char>(data_[current_pos_]);
    }
    return EOF;
}

std::size_t MmapFileReader::Tellg() const {
    return current_pos_;
}

bool MmapFileReader::Eof() const {
    return current_pos_ >= file_size_;
}

void MmapFileReader::Read(char* buffer, std::size_t length) {
    if (current_pos_ + length > file_size_) {
        throw std::out_of_range("Read exceeds file size");
    }
    std::memcpy(buffer, data_ + current_pos_, length);
    current_pos_ += length;
}

void MmapFileReader::Get(char& ch) {
    if (current_pos_ < file_size_) {
        ch = data_[current_pos_++];
    } else {
        throw std::out_of_range("Get position out of range");
    }
}

std::string MmapFileReader::ReadLine() {
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

MmapFileWriter::MmapFileWriter(const char* filename, std::size_t initial_size)
    : filename_(filename)
    , size_(initial_size)
    , current_pos_(0)
    , data_(nullptr) {
    fd_ = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("Failed to open output file: ") + filename + " " + strerror(errno));
    }

    Resize(size_);
    MapFile();
}

MmapFileWriter::~MmapFileWriter() {
    if (data_ && data_ != MAP_FAILED) {
        if (msync(data_, current_pos_, MS_SYNC) == -1) {
            std::cerr << "Failed to sync memory: " << strerror(errno) << '\n';
        }
        if (munmap(data_, size_) == -1) {
            std::cerr << "Failed to unmap memory: " << strerror(errno) << '\n';
        }
    }
    if (fd_ != -1) {
        if (ftruncate(fd_, current_pos_) == -1) {
            std::cerr << "Failed to truncate file: " << strerror(errno) << '\n';
        }
        if (close(fd_) == -1) {
            std::cerr << "Failed to close file: " << strerror(errno) << '\n';
        }
    }
}

void MmapFileWriter::Write(const char* buffer, std::size_t length) {
    while (current_pos_ + length > size_) {
        Resize(size_ * 2);
        MapFile();
    }
    if (!std::memcpy(data_ + current_pos_, buffer, length)) {
        throw std::runtime_error(std::string("Failed to copy memory: ") + strerror(errno));
    }
    current_pos_ += length;
}

void MmapFileWriter::Flush() {
    if (msync(data_, current_pos_, MS_SYNC) == -1) {
        throw std::runtime_error(std::string("Failed to sync file: ") + strerror(errno));
    }
}

void MmapFileWriter::Resize(std::size_t new_size) {
    if (ftruncate(fd_, new_size) == -1) {
        close(fd_);
        throw std::runtime_error(std::string("Failed to resize file: ") + strerror(errno));
    }
    data_ = nullptr;
    size_ = new_size;
}

void MmapFileWriter::MapFile() {
    if (data_ && data_ != MAP_FAILED) {
        if (munmap(data_, size_) == -1) {
            std::cerr << "Failed to unmap memory: " << strerror(errno) << '\n';
        }
    }

    data_ = static_cast<char*>(mmap(nullptr, size_, PROT_WRITE, MAP_SHARED, fd_, 0));
    if (data_ == MAP_FAILED) {
        close(fd_);
        throw std::runtime_error(std::string("Failed to mmap file: ") + strerror(errno));
    }
}

void StdoutStream::Write(const char* buffer, std::size_t length) {
    std::cout.write(buffer, length);
}

void StdoutStream::Flush() {
    std::cout.flush();
}

std::shared_ptr<OStream> GetOutputStream(const std::string& path) {
    if (path == "stdout") {
        return std::make_shared<StdoutStream>();
    } else {
        return std::make_shared<MmapFileWriter>(path.c_str(), 8 * 1024 * 1024);
    }
}

std::shared_ptr<IStream> GetInputStream(const std::string& path) {
    if (path == "stdin") {
        return std::make_shared<StdinStream>();
    } else {
        return std::make_shared<MmapFileReader>(path.c_str());
    }
}

} // namespace lib::chunk_impl
