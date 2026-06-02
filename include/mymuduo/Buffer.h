#pragma once

#include <algorithm>
#include <vector>
#include <string>

namespace mymuduo{

class Buffer{
public:
    const static size_t kcheapPrepend = 8;
    const static size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
    :buffer_(kcheapPrepend + kInitialSize), readerIndex_(kcheapPrepend), writerIndex_(kcheapPrepend){}

    size_t readableBytes() const ;
    size_t writableBytes() const ;
    size_t prependableBytes() const ;
    const char* peek() const ;

    void retrieve(size_t len);
    void retrieveAll();
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);

    void append(const char* data, size_t len);
    void append(const std::string& s);

    ssize_t readFd(int fd, int* savedErrno);
    ssize_t writeFd(int fd, int* savedErrno);


    

private:
    char* begin() ;
    const char* begin() const ;
    char* beginWrite() ;
    void ensureWritableBytes(size_t len);
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

}
