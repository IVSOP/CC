#include "server.h"


Server::Server() : mtx(std::mutex()){
    fileMap = std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, std::vector<uint32_t> > > >();
};

Server::~Server() {
    fileMap.clear();
};

void Server::addNewInfo(uint32_t ip, FS_Track::RegUpdateData &newNode) {
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);

    uint64_t hash = newNode.getFileHash();
    std::vector<uint32_t> receivedBlocks = newNode.getBlockNumbers(); // vetor copiado com blocos novos

    if (fileMap.find(hash) == fileMap.end()) { // procurar se hash do ficheiro já existe
        std::vector<std::pair<uint32_t, std::vector<uint32_t>>> fileVector = std::vector<std::pair<uint32_t, std::vector<uint32_t>>>();
        fileMap.insert({hash, fileVector});
    }

    auto mapIter = fileMap.find(hash);
    std::pair<uint32_t, std::vector<uint32_t>> iterPair;
    bool found = false;

    for (const auto &pair: mapIter->second) { // procurar se nodo já tem blocos desse ficheiro
        if (pair.first == ip) {
            found = true;
            iterPair = pair;
            break;
        }
    }

    if (found) { // se já existiam blocos do ficheiro
        std::vector<uint32_t> nodeBlocks = iterPair.second;
        nodeBlocks.insert(nodeBlocks.end(), receivedBlocks.begin(),
                          receivedBlocks.end()); // meter novos blocos no vector
    } else { // se ainda não exista par (nodo, blocos do ficheiro)
        (mapIter->second).emplace_back(ip, receivedBlocks); // criar novo par (nodo, blocos do ficheiro)
    }

};

void Server::printMap() {
    for (const auto &kv: fileMap) {
        auto value = kv.second;
        for (const auto &pair: value) {
            printf("ip: %d\n", pair.first);
            auto vec = pair.second;
            for (uint32_t number: vec) {
                printf("%d ", number);
            }
            printf("\n");
        }
    }
}

std::vector<FS_Track::PostFileBlocksData> Server::getNodesWithFile(uint64_t hash) {
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
    auto mapIter = fileMap.find(hash);

    std::vector<FS_Track::PostFileBlocksData> ans = std::vector<FS_Track::PostFileBlocksData>();
    struct in_addr ip{};

    if(mapIter != fileMap.end()){
        for(const auto& pair: mapIter->second){
            ip.s_addr = pair.first;
            ans.emplace_back(ip, pair.second);
        }
    }

    return ans;
}

void Server::registerUpdateNode(uint32_t ip, std::vector<FS_Track::RegUpdateData> data){
    for(FS_Track::RegUpdateData& regData: data){
        this->addNewInfo(ip, regData);
    }
}

void Server::deleteNode(uint32_t ip){
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);

    for (auto & mapKeyValue : fileMap)
        for (uint32_t i = 0; i < mapKeyValue.second.size(); i++)
            if (mapKeyValue.second.at(i).first == ip) mapKeyValue.second.erase(mapKeyValue.second.begin() + i);
}
