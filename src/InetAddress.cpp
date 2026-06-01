#include <cstring>
#include <netinet/in.h>

#include "mymuduo/InetAddress.h"

namespace mymuduo{

InetAddress::InetAddress(uint16_t port = 0, const std::string ip= "0.0.0.0"){
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

std::string InetAddress::toIp() const{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    int len = strlen(buf);
    sprintf(buf + len, ":%u", ntohs(addr_.sin_port));
    return buf;
}

uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}

}