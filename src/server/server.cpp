#include "server.h"


Server::Server() : mtx(){
    fileMap = std::unordered_map<uint64_t, std::vector<std::pair<std::string, bitMap > > >();
};

Server::~Server() {
    fileMap.clear();
};

void Server::addNewInfo(std::string node, FS_Track::RegUpdateData &newNode) {
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);

    puts("Entered function");

    uint64_t hash = newNode.getFileHash();
    bitMap receivedBlocks = newNode.getBlockNumbers(); // vetor copiado com blocos novos

    if (fileMap.find(hash) == fileMap.end()) { // procurar se hash do ficheiro já existe
        std::vector<std::pair<std::string, bitMap>> fileVector = std::vector<std::pair<std::string, bitMap>>();
        fileMap.insert({hash, fileVector});
    }

    auto mapIter = fileMap.find(hash);
    std::pair<uint32_t, bitMap> iterPair;

    for (uint32_t i = 0; i < mapIter->second.size(); i++) {
        auto &pair = mapIter->second.at(i);// procurar se nodo já tem blocos desse ficheiro
        if (pair.first == node) {
            mapIter->second.erase(mapIter->second.begin() + i);
            break;
        }
    }

    (mapIter->second).emplace_back(node, receivedBlocks); // criar novo par (nodo, blocos do ficheiro)
}

void Server::printMap() {
    for (const auto &kv: fileMap) {
        auto value = kv.second;
        for (const auto &pair: value) {
            printf("Ip: %s\n", pair.first.c_str());
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
            ans.emplace_back(pair.first, pair.second);
        }
    }

    return ans;
}

void Server::registerUpdateNode(std::string node, std::vector<FS_Track::RegUpdateData> data){
    for(FS_Track::RegUpdateData& regData: data){
        this->addNewInfo(node, regData);
    }
}

void Server::deleteNode(std::string node){
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);

    for (auto & mapKeyValue : fileMap)
        for (uint32_t i = 0; i < mapKeyValue.second.size(); i++)
            if (mapKeyValue.second.at(i).first == node) mapKeyValue.second.erase(mapKeyValue.second.begin() + i);
}
