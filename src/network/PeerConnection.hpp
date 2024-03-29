#pragma once

#include <string>
#include <string_view>
#include <vector>



namespace network {

class PeerConnection {
public:
    PeerConnection(int localPort, std::string_view remoteIp, int remotePort);
    ~PeerConnection();

    bool init();
    bool isInitialized() const;

    int fetchPacket(std::vector<char>* buf);
    bool sendPacket(std::string_view data);

private:
    std::string _remoteIp;
    int _socket;
    int _remotePort;
    int _localPort;
};

}
