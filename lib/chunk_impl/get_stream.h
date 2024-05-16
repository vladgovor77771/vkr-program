#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace lib::chunk_impl {

class IStream {
public:
    virtual void Seekg(int offset, std::ios_base::seekdir dir = std::ios_base::cur) = 0;
    virtual int Peek() const = 0;
    virtual std::size_t Tellg() const = 0;
    virtual bool Eof() const = 0;
    virtual void Get(char& ch) = 0;
    virtual void Read(char* buffer, std::size_t length) = 0;
    virtual std::string ReadLine() = 0;
};

class MmapFileReader: public IStream {
public:
    MmapFileReader(const char* filename);
    ~MmapFileReader();

    void Seekg(int offset, std::ios_base::seekdir dir = std::ios_base::cur) override;
    int Peek() const override;
    std::size_t Tellg() const override;
    bool Eof() const override;
    void Read(char* buffer, std::size_t length) override;
    void Get(char& ch) override;
    std::string ReadLine() override;

private:
    int fd_ = -1;
    std::size_t file_size_ = 0;
    std::size_t current_pos_ = 0;
    char* data_ = nullptr;
};

class StdinStream: public IStream {
public:
    StdinStream() = default;
    ~StdinStream() = default;
    void Seekg(int offset, std::ios_base::seekdir dir = std::ios_base::cur) override;
    int Peek() const override;
    std::size_t Tellg() const override;
    bool Eof() const override;
    void Read(char* buffer, std::size_t length) override;
    void Get(char& ch) override;
    std::string ReadLine() override;
};

class OStream {
public:
    virtual void Write(const char* buffer, std::size_t length) = 0;
    virtual void Flush() = 0;
};

class MmapFileWriter: public OStream {
public:
    MmapFileWriter(const char* filename, std::size_t initial_size);
    ~MmapFileWriter();
    void Write(const char* buffer, std::size_t length) override;
    void Flush() override;

private:
    void Resize(std::size_t new_size);
    void MapFile();

    const char* filename_;
    int fd_;
    std::size_t size_;
    std::size_t current_pos_;
    std::size_t max_written_pos_;
    char* data_ = nullptr;
};

class StdoutStream: public OStream {
public:
    StdoutStream() = default;
    ~StdoutStream() = default;
    void Write(const char* buffer, std::size_t length) override;
    void Flush() override;
};

std::shared_ptr<OStream> GetOutputStream(const std::string& path);
std::shared_ptr<IStream> GetInputStream(const std::string& path);

} // namespace lib::chunk_impl
