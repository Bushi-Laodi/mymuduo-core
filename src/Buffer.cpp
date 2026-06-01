#include "Buffer.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string>
namespace mymuduo{


size_t Buffer::readableBytes() const { 
    return writerIndex_ - readerIndex_; 
}

size_t Buffer::writableBytes() const { 
    return buffer_.size() - writerIndex_; 
}

size_t Buffer::prependableBytes() const { 
    return readerIndex_; 
}

const char* Buffer::peek() const { 
    return begin() + readerIndex_; 
}

char* Buffer::begin() { 
    return buffer_.data(); 
}

const char* Buffer::begin() const { 
    return buffer_.data(); 
}

char* Buffer::beginWrite() { 
    return begin() + writerIndex_; 
}

//去掉一些读区的内容
void Buffer::retrieve(size_t len){
    if(len < readableBytes()){
        readerIndex_ += len;
    }
    else{
        retrieveAll();
    }
}

//去掉全部读区的内容
void Buffer::retrieveAll(){
    readerIndex_ = kcheapPrepend;
    writerIndex_ = kcheapPrepend;
}

//以字符串的形式读取全部数据
std::string Buffer::retrieveAllAsString(){
    return retrieveAsString(readableBytes());
}

//以字符串的形式读取指定长的的字符串
std::string Buffer::retrieveAsString(size_t len){
    std::string resualt(peek(), len);
    retrieve(len);
    return resualt;
}

//追加写入一些数据
void Buffer::append(const char* data, size_t len){
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

void Buffer::append(const std::string& s) { 
    size_t cnt = s.size();
    append(s.data(), cnt);
}

// 从 socket fd 读取数据
ssize_t Buffer::readFd(int fd, int* savedErrno){
    char extraBuffer[65535];
    iovec vec[2];
    size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extraBuffer;
    vec[1].iov_len = sizeof(extraBuffer);
    int ivocnt = writable < sizeof(extraBuffer) ? 2 : 1;
    ssize_t n = ::readv(fd, vec, ivocnt);
    if(n < 0){
        *savedErrno = errno;
    }else if(static_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();
        append(extraBuffer, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno){
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0) *savedErrno = errno;
    return n;
}

void Buffer::ensureWritableBytes(size_t len){
    if(writableBytes() < len) makeSpace(len);
}

void Buffer::makeSpace(size_t len){
    if(writableBytes() + prependableBytes() < len + kcheapPrepend){
        buffer_.resize(writerIndex_ + len);
    }
    else{
        size_t readable = readableBytes();
        //把当前 Buffer 中“可读数据区域”的内容，
        //整体搬到 buffer_ 前面预留区之后的位置。
        std::copy(begin() + readerIndex_, 
                begin() + writerIndex_, 
                begin() + kcheapPrepend);
        readerIndex_ = kcheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}
}