#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <unistd.h>

namespace mymuduo{

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
    : loop_(loop), listenning_(false), acceptScoket_(Socket::createNonBlockingSocket()),
    acceptChannel_(loop, acceptScoket_.fd())
{
    acceptScoket_.setReuseAddr(true);
    acceptScoket_.setReusePort(reusePort);
    acceptScoket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([this](Timestamp){ handleRead();});
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptScoket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead(){
    InetAddress peer;
    int connfd = acceptScoket_.accept(&peer);
    if(connfd > 0){
        if(newConnectionCallback_) newConnectionCallback_(connfd, peer);
        else ::close(connfd);
    }
}

}