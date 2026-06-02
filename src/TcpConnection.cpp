#include "mymuduo/TcpConnection.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Socket.h"
#include "mymuduo/Channel.h"
#include <unistd.h>

namespace mymuduo{

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& local, const InetAddress& peer)
    : loop_(loop), name_(name), state_(kConnecting), socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)), localAddr_(local), 
    peerAddr_(peer), highWaterMark_(64 * 1024 * 1024)
{
    channel_->setReadCallback([this](Timestamp t) { handleRead(t); });
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setCloseCallback([this] { handleClose(); });
    channel_->setErrorCallback([this] { handleError(); });
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() = default;

void TcpConnection::send(const std::string& message){
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) sendInLoop(message.data(), message.size());
        else {
            std::string copy = message;//防止失效，生命周期
            TcpConnectionPtr self = shared_from_this();
            loop_->runInLoop([self, copy] {
                self->sendInLoop(copy.data(), copy.size());
            });
        }
    }
}

void TcpConnection::shutdown(){
    if (state_ == kConnected) {//只有处于连接状态的才能进行关闭连接
        setState(kDisconnecting);//设置状态为正在关闭连接   
        TcpConnectionPtr self = shared_from_this();
        loop_->runInLoop([self] { self->shutdownInLoop(); });
    }
}

void TcpConnection::connectEstablished(){
    loop_->assertInLoopThread();//确保在当前 loop 线程中执行
    setState(kConnected);
    channel_->tie(shared_from_this());//当 channel 发生事件时，TcpConnection 对象一定存在
    channel_->enableReading();
    if (connectionCallback_) connectionCallback_(shared_from_this());//连接状态发生变化时，要进行回调
}

void TcpConnection::connectDestroyed(){
    loop_->assertInLoopThread();//确保在当前 loop 线程中执行
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        if (connectionCallback_) connectionCallback_(shared_from_this());//连接状态发生变化时，要进行回调
    }
    channel_->remove();//把 channel 从 Poller 中删除掉
}

void TcpConnection::handleRead(Timestamp receiveTime){
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        if (messageCallback_) messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);//收到信息就进行信息的回调
    } else if (n == 0) { //为零就断开连接
        handleClose();
    } else {
        errno = savedErrno;
        handleError();
    }
}

void TcpConnection::handleWrite(){
    if (channel_->isWriting()) {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {//判断缓冲区数据是否被清空
                channel_->disableWriting();//数据被清空了，就没有必要继续关注写事件了
                if (writeCompleteCallback_) {//调用写完成的回调函数
                    TcpConnectionPtr self = shared_from_this();
                    loop_->queueInLoop([self] {
                        self->writeCompleteCallback_(self);
                    });
                }
                if (state_ == kDisconnecting) shutdownInLoop();//如果正在关闭连接，并且数据已经全部发送完了，就可以关闭连接了
            }
        }
    }
}

void TcpConnection::handleClose(){
    loop_->assertInLoopThread();
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr guard(shared_from_this());
    if (connectionCallback_) connectionCallback_(guard);//连接状态发生变化时，要进行回调
    if (closeCallback_) closeCallback_(guard);
}

void TcpConnection::handleError(){
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    ::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen);//获取 socket 错误状态
    //缺少错误处理和日志记录
}

void TcpConnection::sendInLoop(const void* data, size_t len){
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) return;//如果连接已经断开了，就没有必要发送数据了

    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {//如果没有关注写事件，并且发送缓冲区没有数据，就可以直接发送了
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                TcpConnectionPtr self = shared_from_this();
                loop_->queueInLoop([self] {
                    self->writeCompleteCallback_(self);
                });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {//如果发送失败了，并且不是因为发送缓冲区满了，就说明发生了错误
                if (errno == EPIPE || errno == ECONNRESET) faultError = true;//如果对方已经关闭了连接，就会收到 EPIPE 或者 ECONNRESET 错误
                else {
                    //缺少错误处理和日志记录
                }
            }
        }
    }

    if (!faultError && remaining > 0) {//如果发生了错误，或者还有数据没有发送完，就把剩余的数据添加到发送缓冲区中，并且关注写事件
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            //如果待发送数据的长度超过了警戒线，并且之前没有超过，就调用高水位回调函数
            TcpConnectionPtr self = shared_from_this();
            size_t size = oldLen + remaining;
            loop_->queueInLoop([self, size] {
                self->highWaterMarkCallback_(self, size);
            });
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->isWriting()) channel_->enableWriting();//如果没有关注写事件，就关注写事件，以便发送剩余的数据
    }
}

void TcpConnection::shutdownInLoop(){
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) socket_->shutdownWrite();
}


}
