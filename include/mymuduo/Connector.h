#pragma once
#include "mymuduo/InetAddress.h"
#include <functional>
#include <memory>

namespace mymuduo {

class Channel;
class EventLoop;

//用于进行socket连接成功前的一切事物的处理，连接成功之后就交给client来控制这个socket
class Connector : public std::enable_shared_from_this<Connector> {
public:
    using NewConnectionCallback = std::function<void(int)>;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }
    void start();

private:
    enum State { kDisconnected, kConnecting, kConnected };

    void startInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    int removeAndResetChannel();
    void resetChannel();
    void setState(State s) { state_ = s; }

    EventLoop* loop_;
    InetAddress serverAddr_;
    State state_;
    bool connect_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
};

}