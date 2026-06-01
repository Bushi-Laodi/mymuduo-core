#include "mymuduo/Connector.h"
#include "mymuduo/Channel.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Socket.h"
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace mymuduo {

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      state_(kDisconnected),
      connect_(false) {}

Connector::~Connector() {}

void Connector::start() {
    connect_ = true;
    std::shared_ptr<Connector> self = shared_from_this();
    loop_->runInLoop([self] { self->startInLoop(); });
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    if (connect_) connect();
}

void Connector::connect() {
    int sockfd = Socket::createNonBlockingSocket();
    int ret = ::connect(sockfd,
                        reinterpret_cast<const sockaddr*>(serverAddr_.getSockAddr()),
                        sizeof(sockaddr_in));
    int savedErrno = ret == 0 ? 0 : errno;
    switch (savedErrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        default:
            ::close(sockfd);
            setState(kDisconnected);
            break;
    }
}

void Connector::connecting(int sockfd) {
    setState(kConnecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setErrorCallback([this] { handleError(); });
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this] { resetChannel(); });
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = 0;
        socklen_t len = sizeof(err);
        if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) err = errno;

        if (err) {
            ::close(sockfd);
            setState(kDisconnected);
        } else if (connect_) {
            setState(kConnected);
            if (newConnectionCallback_) newConnectionCallback_(sockfd);
        } else {
            ::close(sockfd);
        }
    }
}

void Connector::handleError() {
    int sockfd = removeAndResetChannel();
    ::close(sockfd);
    setState(kDisconnected);
}

}