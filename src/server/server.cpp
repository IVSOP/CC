#include "server.h"


Server::Server() {
    fileMap = std::unordered_map<int64_t, std::vector<std::pair<uint32_t, std::vector<uint32_t> > > >();
};

Server::~Server() {
    fileMap.clear();
};

void Server::add_new_info(uint32_t ip, FS_Track::RegUpdateData& newNode) {

    uint64_t hash = newNode.getFileHash();
    std::vector<uint32_t> receivedBlocks = newNode.getBlockNumbers(); // vetor copiado com blocos novos

    if (fileMap.find(hash) == fileMap.end()) { // procurar se hash do ficheiro já existe
        std::vector<std::pair<uint32_t, std::vector<uint32_t>>> * fileVector = new std::vector<std::pair<uint32_t, std::vector<uint32_t>>>();
        fileMap.insert({hash,*fileVector});
    }
    auto mapIter = fileMap.find(hash);
    std::pair<uint32_t, std::vector<uint32_t>> iterPair;
    bool found = false;
    for (const auto &pair : mapIter->second) { // procurar se nodo já tem blocos desse ficheiro
        if (pair.first == ip) {
            found = true;
            iterPair = pair;
            break;
        }
    }
    if (found) { // se já existiam blocos do ficheiro
        std::vector<uint32_t> nodeBlocks = iterPair.second;
        nodeBlocks.insert(nodeBlocks.end(), receivedBlocks.begin(), receivedBlocks.end()); // meter novos blocos no vector
    } else { // se ainda não exista par (nodo, blocos do ficheiro)
        (mapIter->second).push_back(make_pair(ip,receivedBlocks)); // criar novo par (nodo, blocos do ficheiro)
    }

};

void Server::print_map() {
    for (const auto &kv: fileMap) {
        auto value = kv.second;
        for (const auto &pair: value) {
            printf("ip: %d\n", pair.first);
            auto vec = pair.second;
            for (uint32_t number: vec) {
                printf("%d ",number);
            }
            printf("\n");
        }
    }
}

std::vector<uint32_t> Server::get_nodes_with_file(uint64_t hash) {
};

