#include "get_stream.h"

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
    , max_written_pos_(0) {
    fd_ = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to open file");
    }

    Resize(size_);
    MapFile();
}

MmapFileWriter::~MmapFileWriter() {
    if (data_ != MAP_FAILED) {
        msync(data_, max_written_pos_, MS_SYNC);
        munmap(data_, size_);
    }
    if (fd_ != -1) {
        ftruncate(fd_, max_written_pos_);
        close(fd_);
    }
}

void MmapFileWriter::Write(const char* buffer, std::size_t length) {
    while (current_pos_ + length > size_) {
        Resize(size_ * 2);
        MapFile();
    }
    std::memcpy(data_ + current_pos_, buffer, length);
    current_pos_ += length;
    if (current_pos_ > max_written_pos_) {
        max_written_pos_ = current_pos_;
    }
}

void MmapFileWriter::Flush() {
    if (msync(data_, max_written_pos_, MS_SYNC) == -1) {
        throw std::runtime_error("Failed to sync file");
    }
}

void MmapFileWriter::Resize(std::size_t new_size) {
    if (ftruncate(fd_, new_size) == -1) {
        close(fd_);
        throw std::runtime_error("Failed to resize file");
    }
    size_ = new_size;
}

void MmapFileWriter::MapFile() {
    if (data_ != MAP_FAILED) {
        munmap(data_, size_);
    }

    data_ = static_cast<char*>(mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (data_ == MAP_FAILED) {
        close(fd_);
        throw std::runtime_error("Failed to mmap file");
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
