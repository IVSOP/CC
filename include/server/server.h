#ifndef SERVER_H
#define SERVER_H

#include <unordered_map>
#include <vector>
#include <mutex>
#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_track.h"

typedef uint64_t HASH_TYPE;
typedef uint32_t IP_TYPE;
typedef uint32_t BLOCK_TYPE;

class Server {
public:
    Server();

    ~Server();

    void addNewInfo(uint32_t ip, FS_Track::RegUpdateData &newNode);

    std::vector<FS_Track::PostFileBlocksData> getNodesWithFile(HASH_TYPE hash);

    void printMap();

    void registerUpdateNode(uint32_t ip, std::vector<FS_Track::RegUpdateData> data);

    void deleteNode(uint32_t ip);

private:
    std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, bitMap>>> fileMap;
    std::mutex mtx;
};

#endif
