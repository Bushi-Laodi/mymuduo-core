#pragma once
#include <functional>
#include "Channel.h"
#include "Socket.h"


namespace mymuduo{
class EventLoop;
class InetAdress;
class Acceptor{
public:
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb){ newConnectionCallback_ = cb; }
    void listen();
    bool listening(){return listening_;}


private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;

};
}
