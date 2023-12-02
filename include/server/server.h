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

    void addNewInfo(std::string node, FS_Track::RegUpdateData &newNode);

    std::vector<FS_Track::PostFileBlocksData> getNodesWithFile(HASH_TYPE hash);

    void printMap();

    void registerUpdateNode(std::string node, std::vector<FS_Track::RegUpdateData> data);

    void deleteNode(std::string node);

private:
    std::unordered_map<uint64_t, std::vector<std::pair<std::string, bitMap>>> fileMap;
    std::mutex mtx;
};

#endif
