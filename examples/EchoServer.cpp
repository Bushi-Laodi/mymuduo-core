#include "mymuduo/Buffer.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/InetAddress.h"
#include "mymuduo/TcpConnection.h"
#include "mymuduo/TcpServer.h"
#include <iostream>

int main(){
    mymuduo::EventLoop loop;
    mymuduo::InetAddress listenAddr(8000);
    mymuduo::TcpServer server(&loop, listenAddr, "EchoServer");

    server.setConnectionCallback([](const mymuduo::TcpConnectionPtr& conn) {
        std::cout << conn->name()
                  << (conn->connected() ? " connected" : " disconnected")
                  << std::endl;
    });

    server.setMessageCallback([](const mymuduo::TcpConnectionPtr& conn,
                                 mymuduo::Buffer* buffer,
                                 const Timestamp) {
        std::string message = buffer->retrieveAllAsString();
        conn->send(message);
    });

    server.start();
    loop.loop();
    return 0;
}
