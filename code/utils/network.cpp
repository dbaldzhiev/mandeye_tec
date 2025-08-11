#include "network.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace utils
{
        std::string getInterfaceIp()
        {
                struct ifaddrs* ifaddr = nullptr;
                std::string ip;
                if(getifaddrs(&ifaddr) != 0)
                {
                        return ip;
                }
                for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
                {
                        if(ifa->ifa_addr == nullptr)
                        {
                                continue;
                        }
                        if(ifa->ifa_addr->sa_family == AF_INET)
                        {
                                char addr[INET_ADDRSTRLEN];
                                auto* in = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
                                if(inet_ntop(AF_INET, &in->sin_addr, addr, sizeof(addr)))
                                {
                                        if(std::string(ifa->ifa_name) == "eth0")
                                        {
                                                ip = addr;
                                                break;
                                        }
                                        if(ip.empty())
                                        {
                                                ip = addr;
                                        }
                                }
                        }
                }
                if(ifaddr)
                {
                        freeifaddrs(ifaddr);
                }
                return ip;
        }
} // namespace utils
