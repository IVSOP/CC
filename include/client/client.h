#ifndef CLIENT_H
#define CLIENT_H

#include <unordered_map>
#include <thread>
#include <chrono>
#include <fstream>
#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include "fs_track.h"
#include "bounded_buffer.h"
#include "bitmap.h"
#include "checksum.h"

#define CLIENT_INPUT_BUFFER_SIZE 10
#define CLIENT_OUTPUT_BUFFER_SIZE 10
#define MAX_BLOCKS_REQUESTS_PER_NODE 200
#define NODES_RTT_TRACK_SIZE 16
// Struct to store sockaddr_in structs

struct Ip {
    struct sockaddr_in addr;

    Ip() = default;

    Ip(struct sockaddr_in addr){
        this->addr = addr;
    }

    bool operator==(const struct Ip &o) {
        return addr.sin_addr.s_addr == o.addr.sin_addr.s_addr;
    }

    bool operator<(const struct Ip &o) {
        return addr.sin_addr.s_addr < o.addr.sin_addr.s_addr;
    }
};

//hash function for unordered_map pair structure
struct KeyHash {
    std::size_t operator()(const std::pair<uint64_t, uint32_t>& p) const {
        // A simple hash combining technique
        // Using bitwise XOR to combine hash values
        std::hash<uint64_t> hash1;
        std::hash<uint32_t> hash2;

        // Combine the hash values using XOR (^)
        return hash1(p.first) ^ (hash2(p.second) + 0x9e3779b9 + (hash1(p.first) << 6) + (hash1(p.first) >> 2));
    }
};

//has function for unordered_map Ip structure
// struct IpHash {
//     std::size_t operator()(const Ip& ip) const {
//         // Combine hash values of the address components
//         std::size_t hash = 0;
//         hash ^= std::hash<uint32_t>{}(ip.addr.sin_addr.s_addr);
//         hash ^= std::hash<uint16_t>{}(ip.addr.sin_port);
//         return hash;
//     }
// };

//Guarda informação de tempo em que ficheiro e bloco foi pedido
// struct NodeTimes {
//     uint64_t hash;
//     uint32_t blockNumber;
//     time_t timeSent;
//     time_t received_time;

//     NodeTimes();
//     NodeTimes(uint64_t hash, uint32_t blockNumber, time_t timeSent) {
//         this->hash = hash;
//         this->blockNumber = blockNumber;
//         this->timeSent = timeSent;
//         //this->received_time -- to be filled on return
//     }


// };

namespace std {
    template <>
    struct hash<Ip> {
        std::size_t operator()(const Ip& ip) const {
            return std::hash<int>()(ip.addr.sin_addr.s_addr);
        }
    };
    template <>
    struct equal_to<Ip> {
        bool operator()(const Ip &lhs, const Ip &rhs) const {
            return lhs.addr.sin_addr.s_addr == rhs.addr.sin_addr.s_addr;
        }
    };
}

// struct para que as threads possam trabalhar sobre os dados recebidos
struct FS_Transfer_Info {

	// por default fica neste por e dizemos que tem ipv4
	// assim basta mudar o Ip para tudo funcionar
	FS_Transfer_Info() {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(UDP_PORT);
	}

    FS_Transfer_Info(const FS_Transfer_Packet& packet, const Ip& ip, const time_t& timestamp){
        this->packet = packet;
        this->addr = ip.addr;
        this->timestamp = timestamp;
    };

    time_t timestamp;
    struct sockaddr_in addr; // has in_addr // in_addr has ip
    FS_Transfer_Packet packet;

    FS_Transfer_Info(FS_Transfer_Packet& packet, uint32_t ip, time_t& timestamp) {
        this->timestamp = timestamp;
        this->addr.sin_family = AF_INET;
		this->addr.sin_port = htons(UDP_PORT);
        this->addr.sin_addr.s_addr = ip;

        memcpy(&this->packet, &packet, FS_TRANSFER_PACKET_SIZE);
    }
    
    constexpr const FS_Transfer_Packet& getTransferPacket() const {
        return packet;
    }

    void setTimestamp(time_t& timsestamp);
    void setAdrr(struct sockaddr_in&) const;
    void setPacket(const FS_Transfer_Packet *, ssize_t size);

};

// 1 thread le do socket, outra pega nisso e realiza trabalho
// 1 thread escreve para socket, main thread faz trabalho em que escreve para o buffer dela
// main thread faz trabalkho relacio
// como buffers ja estao implementados, no futuro podemos usar thread pool

// pode ser otimizado, ha muitas copias de memoria

struct Client {

    using FS_Transfer_Packet_handler = void (Client::*) (FS_Transfer_Info&); //typedef para as funções da dispatch table

    Client();
    Client(char* dir);
	Client(char* dir, const std::string &IPv4); // para set do server IP

    ~Client();

    // main funcs, called in constructor
    void registerWithServer();
    void initUploadLoop();
    void commandParser(const char * dir);
    void assignDispatchTable();

    void readLoop();
    void writeLoop();

    void sendRequest();
	void answerRequestsLoop();

    void fetchFile(const char * dir, const char * filename, std::vector<FS_Track::PostFileBlocksData>);
    void sendInfo(FS_Transfer_Info &info);

    //parse opcode funcs
    void ReqBlockData(FS_Transfer_Info& info);
    void RespondBlockData(FS_Transfer_Info& info);

    void regNewFile(const char* dir, const char* fn, size_t size);
    ssize_t getFileBlock(uint64_t fileHash, uint32_t blockID, char * buffer);
    void writeFileBlock(uint64_t fileHash, uint32_t blockN, char * buffer, size_t size);

	void wrongChecksum(const FS_Transfer_Info &info) {
		// ....................
	}

    void regDirectory(char* directory);
    void regFile(const char* dir, char* fn);

    /**
     * Select node from who we should request a block through a priority
     * @param hash Hash identifying the file we are referring to
     * @param block_nodes List of pairs {block: nodes that have that block available}
     * @return Exit code (sucess or insucess)
     */
    int weightedRoundRobin(uint64_t hash, std::vector<std::pair<uint32_t, std::vector<Ip>>>& block_nodes, double* max_rtt);
    Ip selectBestNode(std::vector<Ip>& available_nodes, std::unordered_map<Ip, std::vector<uint32_t>> nodes_blocks);

    // Node scheduling -----------
    void regPacketSentTime(FS_Transfer_Info& info, time_t& sentTimestamp);
    void updateNodeResponseTime(FS_Transfer_Info& info, time_t& arrivedTimestamp);
    void insert_regPacket(const Ip& nodeIp, uint64_t file, uint32_t blockN, time_t& startTime);
    time_t * find_remove_regPacket(const Ip& nodeIp, uint64_t file, uint32_t blockN);
    void insert_regRTT(const Ip& nodeIp, time_t& timeSent, time_t& timeReceived);
    void printFull_node_sent_reg();
    void printFull_nodes_tracker();

    ClientTCPSocket socketToServer;
    NodeUDPSocket udpSocket; // usamos apenas 1 socket para tudo
    std::thread readThread, writeThread, answererThread; // 1 thread loop read, 1 thread loop write, 1 thread loop responder pedidos (faz upload)

	// consider maling this arrays of pointers to avoid so many copies!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BoundedBuffer<FS_Transfer_Info, CLIENT_INPUT_BUFFER_SIZE> inputBuffer; // meter isto na heap??????????????
    BoundedBuffer<FS_Transfer_Info, CLIENT_OUTPUT_BUFFER_SIZE> outputBuffer; // meter isto na heap??????????????

	// sem controlo de concorrencia por agora, nao planeio usar varias threads aqui
    //track blocks that each file has
	std::unordered_map<uint64_t, bitMap> blocksPerFile;

    // juntar ao array de cima maybe
    std::unordered_map<uint64_t, uint32_t> currentBlocksInEachFile; 
    
    //track file descriptors for each filehash
    std::unordered_map<uint64_t, FILE*> fileDescriptorMap;

    //dispatch table
    std::unordered_map<uint8_t,FS_Transfer_Packet_handler> dispatchTable;

    struct NodesRTT {
        time_t arr[NODES_RTT_TRACK_SIZE]; //Rtt
        uint32_t curr;
        uint32_t size;

        NodesRTT() {
            curr = 0;
            size = 0;
        }

        void receive(time_t& timeDiff){
            this->arr[curr] = timeDiff;
            curr = (curr + 1) % NODES_RTT_TRACK_SIZE;
            if (size < NODES_RTT_TRACK_SIZE) size++; //para saber quais posições têm dados a sério
        }

        double RTT(){
            time_t total = 0;
            for(uint32_t i = 0; i < size; i++){
                total += arr[i];
            }
            return ((double) total / size);
        }
    };

    // Node scheduling 
    std::unordered_map<Ip, NodesRTT> nodes_tracker; // tracks last x amount of RTTs
    // std::unordered_map<std::pair<uint64_t, uint32_t>, >> nodes_regTimes; // tracks current timestamps for sent block requets
    std::unordered_map<Ip, std::unordered_map<std::pair<uint64_t,uint32_t>,time_t, KeyHash>> node_sent_reg;// tracks timestamps for different node requests

    // Keep nodes priority updated (map <Ip, priority>)
    std::unordered_map<Ip, uint32_t> nodes_priority; // priority given to each node

    std::mutex pendingBlocksMutex;
    std::condition_variable pendingBlocksCondition;
	uint32_t numPendingBlocks; // numero de blocos que pedimos mas ainda nao recebemos. esta associado a lock e condition anteriores

    //inicializados só uma vez, alterados com o decorrer
    FS_Transfer_Info dataFinal;
    FS_Transfer_Packet dataPacket;
    BlockSendData blockSend;
};

#endif
