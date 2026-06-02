#include "mymuduo/Acceptor.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/InetAddress.h"
#include <unistd.h>

namespace mymuduo{

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
    : loop_(loop), listening_(false), acceptSocket_(Socket::createNonBlockingSocket()),
    acceptChannel_(loop, acceptSocket_.fd())
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([this](Timestamp){ handleRead();});
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead(){
    InetAddress peer;
    int connfd = acceptSocket_.accept(&peer);
    if(connfd > 0){
        if(newConnectionCallback_) newConnectionCallback_(connfd, peer);
        else ::close(connfd);
    }
}

}
