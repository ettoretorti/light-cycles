#include "network/PeerConnection.hpp"

#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <string_view>
#include <iostream>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

namespace {
struct addrinfo* infoFor(const std::string& ip, int port) {
    struct addrinfo hints, *res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    const auto remotePort = std::to_string(port);
    int status = getaddrinfo(ip.c_str(), remotePort.c_str(), &hints, &res);
    if (status != 0 || res == nullptr) {
        std::cerr << "Error resolving ip (" << ip << ") " << gai_strerror(status) << std::endl;
        return nullptr;
    }
    return res;
}
}

namespace network {
PeerConnection::PeerConnection(int localPort, std::string_view remoteIp, int remotePort) : _remoteIp(remoteIp), _socket(-1), _remotePort(remotePort), _localPort(localPort) {}

PeerConnection::~PeerConnection() {
  if(_socket != -1) {
    close(_socket);
  }
}

bool PeerConnection::init() {
    if (_socket != -1) {
        return true;
    }

    // check the remote address is resolvable
    struct addrinfo hints, *res = nullptr;
    res = infoFor(_remoteIp, _remotePort);
    if (res == nullptr) {
        return false;
    }
    freeaddrinfo(res);
    res = nullptr;

    // open and bind socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    const auto localPort = std::to_string(_localPort);
    int status = getaddrinfo(nullptr, localPort.c_str(), &hints, &res);
    if (status != 0 || res == nullptr) {
        std::cerr << "Error resolving local port (" << localPort << ") " << gai_strerror(status) << std::endl;
        return false;
    }
    _socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_socket == -1) {
        std::cerr << "Error opening socket" << std::endl;
        return false;
    }
    fcntl(_socket, F_SETFL, O_NONBLOCK);
    status = bind(_socket, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    res = nullptr;
    if (status != 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(_socket);
        _socket = -1;
        return false;
    }

    return true;
}

bool PeerConnection::isInitialized() const {
    return _socket != -1;
}

bool PeerConnection::sendPacket(std::string_view data) {
    if (_socket == -1) {
        return false;
    }
    struct addrinfo* res = infoFor(_remoteIp, _remotePort);
    if (res == nullptr) {
        return false;
    }
    const int status = sendto(_socket, data.data(), data.size(), /*flags=*/0, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    res = nullptr;
    assert(status == (int)data.size());
    return status != -1;
}

int PeerConnection::fetchPacket(std::vector<char>* buf) {
    if (_socket == -1) {
        return 0;
    }
    buf->resize(1500);
    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t size;
    int status = recvfrom(_socket, buf->data(), buf->size(), /*flags=*/0, (struct sockaddr*)&addr, &size);
    if (status >= 0) {
        // check sender matches the remote ip

        buf->resize(status);
        return status;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        buf->resize(0);
        return 0;
    } else {
        std::cerr << "Error on receiving from socket: " << strerror(errno) << std::endl;
        return 0;
    }
}

}
