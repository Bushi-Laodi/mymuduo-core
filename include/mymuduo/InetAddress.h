#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

namespace mymuduo
{   
class InetAddress{
public:
    explicit InetAddress(uint16_t port = 0, const std::string ip= "0.0.0.0");
    explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}

    std::string toIp() const;
    std::string toIpPort() const;//const 表示该函数不会修改成员变量
    uint16_t toPort() const;

    const sockaddr_in* getSockAddr() const { return &addr_; }
    void setSockaddr(const struct sockaddr_in& addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};
}