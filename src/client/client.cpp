#include "client.h"
#include "fs_track.h"
#include <dirent.h>
#include "errors.h"
#include <sys/stat.h> // TODO
#include <algorithm>
#include <condition_variable>
#include <filesystem>

#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_IP "0.0.0.0"
#define FILENAME_BUFFER_SIZE 300
#define BUFFER_SIZE 1500
#define MAX_WRR_NON_UPDATE_TRIES 3

Client::Client() // por default sockets aceitam tudo
    : nameToIP(), socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), dispatchTable(), nodes_priority_lock(),
	nodes_priority(), nodes_tracker_lock(), nodes_tracker()
{
    // register
    initUploadLoop();
    assignDispatchTable();
    commandParser("./files"); // folder standard para ficheiros? importa?
	//regDirectory("./files"); // improta?
}

Client::Client(char* dir) // por default sockets aceitam tudo
	: nameToIP(), socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), fileDescriptorMap(), dispatchTable(), nodes_priority_lock(),
	nodes_priority(), nodes_tracker_lock(), nodes_tracker()
{
    // register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser(dir);
}

Client::Client(char* dir, const std::string &IPv4)
	: nameToIP(), socketToServer(IPv4), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), fileDescriptorMap(), dispatchTable(), nodes_priority_lock(),
	nodes_priority(), nodes_tracker_lock(), nodes_tracker()
{
    // register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser(dir);
}

Client::Client(char* dir, const std::string &server_name, const std::string &myIPv4)
: nameToIP(), socketToServer(&(getIpFromName(server_name)->addr)), udpSocket(myIPv4), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), fileDescriptorMap(), dispatchTable(), nodes_priority_lock(),
	nodes_priority(), nodes_tracker_lock(), nodes_tracker()
{
	// register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser(dir);
}

Client::~Client() {
    udpSocket.closeSocket();
}


// void printResponsePacket(FS_Transfer_Info& info) {
// 	BlockSendData * b = static_cast<BlockSendData *> (info.packet.getData());
// 	printf("BlockSendData packet>>>\n");
// 	printf("sending to IP: %s, checksum:%u (it is %s), opcode: %u sizeOfData: %u fileId: %llu, blockId: %d\n", inet_ntoa(info.addr.sin_addr), info.packet.checksum,
// 		info.packet.calculateChecksum() == info.packet.checksum ? "correct" : "wrong",
// 		info.packet.getOpcode(), info.packet.getSize(), info.packet.getId(), b->blockID);
// 	printf("Time packet was initially sent, not current time:\n");
// 	sys_nanoseconds sent_time = sys_nanoseconds(std::chrono::nanoseconds(info.packet.timestamp)); 
// 	NodesRTT::printTimePoint(sent_time);
// 	printf("\n");
// }

// void PrintRequestPacket(FS_Transfer_Info& info) {
// 	BlockRequestData * b = static_cast<BlockRequestData *> (info.packet.getData());
// 	printf("BlockRequestData packet>>>\n");
// 	printf("sending to IP: %s, checksum: %u, opcode: %u, sizeOfData: %u, fileId: %llu\n",inet_ntoa(info.addr.sin_addr), info.packet.checksum,
// 		info.packet.getOpcode(),info.packet.getSize(), info.packet.id);

// 	sys_nanoseconds sent_time = sys_nanoseconds(std::chrono::nanoseconds(info.packet.timestamp)); 
// 	NodesRTT::printTimePoint(sent_time);

// 	printf("blockIDs requested: ");
// 	for (uint32_t i = 0; i < info.packet.getSize() / 4; i++) {
// 		printf("%d ,",b->blockID[i]);
// 	}
// 	printf("\n");
// }


// read from socket into a buffer, does nothing else
void Client::readLoop() {
	puts("Init reading thread");
	// very bad, copying memory for no reason
	FS_Transfer_Info info;
	while (true) {
		udpSocket.receiveData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
		inputBuffer.push(info);

		// printf("packet received from node %s\nData received: ", inet_ntoa(info.addr.sin_addr));

        // for(uint32_t j = 0; j < info.packet.getSize(); j++){
        //     printf("%d ", ((uint32_t*)info.packet.getData())[j]);
        // }

        // puts("");
	}
}

// write from buffer into socket, does nothing else
void Client::writeLoop() {
	puts("Init writing thread");
	FS_Transfer_Info info;
	while (true) {
		// very bad, copying memory for no reason
		info = outputBuffer.pop();

        udpSocket.sendData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
		printf("packet sent to node %s\n", inet_ntoa(info.addr.sin_addr));
    }
}

// loop trying to answer a request
void Client::answerRequestsLoop() {
	FS_Transfer_Info info;
	double scaleFactor;
	while (true) {
		info = inputBuffer.pop();
		sys_nanoseconds timeArrived = std::chrono::system_clock::now();
		scaleFactor = updateNodeResponseTime(info, timeArrived); // se vier pacote com erro, contabilizo na mesma para RTT, independemente se está correto ou não

        !info.packet.checkErrors() ? wrongChecksum(info) : rightChecksum(info, scaleFactor); // se houver erro nao faz sentido estar a ler o opcode, check tem de ser feito aqui
	}
}

// se checksum falhar, diminui prioridade de nodo
// pode acontecer tanto quando recebe uma request, ou uma reply de outro nodo!!
// n processa o pacote, porque tem dados corrompidos
void Client::wrongChecksum(const FS_Transfer_Info&info) {

	printf("From node %s | WRONG: update value: %d\n", inet_ntoa(info.addr.sin_addr), NODE_VALUE_WRONG);
	updateNodePriority(Ip(info.addr), NODE_VALUE_WRONG);
}

//se checksum correr bem, processa pacote
// só se pacote for response, aumenta prioridade do nodo
void Client::rightChecksum(const FS_Transfer_Info& info, double scaleFactor) {
	uint8_t opcode = info.packet.getOpcode();

    if(opcode > 1) {
        perror("No handler found for packet Opcode");
        return;
    }

    if(opcode == 1) {
		uint64_t timestamp = info.packet.timestamp;
		// prioridade do nodo vai ser maior se tiverem chegado em menos tempo os pacotes, em cada ronda de pedidos
		printf("From node %s | SUCCESS: timestamp: %lu, update value: %d\n", inet_ntoa(info.addr.sin_addr), timestamp, (int32_t) (NODE_VALUE_SUCCESS * 1/timestamp));
		updateNodePriority(Ip(info.addr), static_cast<int32_t> (NODE_VALUE_SUCCESS * scaleFactor));
	}

    (this->*dispatchTable[opcode])(info); // assign data to function with respective opcode

}

// verificar se blocos pedidos deram timeout
// se deram timeout:
// - decrementar prioridade do nodo
// - aumentar RTT do respetivo nodo, para no próximo pedido ter timeout maior
void Client::checkTimeoutNodes(std::unordered_map<Ip, std::vector<uint32_t>>& requested_blocks, uint64_t fileHash, sys_milli_diff timeoutTime) {
	for (auto i = requested_blocks.begin(); i != requested_blocks.end(); i++) {
		for (uint32_t block: i->second) {
            if (this->blocksPerFile[fileHash][block]) continue;

			// se bloco não tiver chegado no tempo definido // excusa de haver locks, se ler errado porque chega no mesmo milisegundo o bloco, conta como timeout na mesma
            printf("From node: %s | TIMEOUT: On blockRequest: %d\n", inet_ntoa(i->first.addr.sin_addr), block);
            updateNodePriority(i->first, NODE_VALUE_TIMEOUT);
            std::unique_lock<std::mutex> lock(this->nodes_tracker_lock);
            nodes_tracker[i->first].receive2(std::chrono::duration_cast<std::chrono::nanoseconds>(timeoutTime));
            this->nodes_tracker_lock.unlock();

            NodesRTT::calcTimeoutTime(this->nodes_tracker[i->first].RTT()); // print do valor atualizado de timeout para esse nodo
            break; // se para um nodo algum bloco não chegar, calcula-se só um timeout, pois à partida se deu timeout para um bloco dará para vários, e n queremos penalizar exageradamente este nodo
		}
	}
}

//obter prioridade de nodo
uint32_t Client::getNodePriority(const Ip& nodeIp) {
	std::unique_lock<std::mutex> lock(this->nodes_priority_lock);
	uint32_t res = this->nodes_priority[nodeIp];
	lock.unlock();
	return res;
}

//incrementa valor à prioridade de nodo
void Client::updateNodePriority(const Ip& nodeIp, int32_t value) {

	std::unique_lock<std::mutex> lock(this->nodes_priority_lock);
	int32_t newPrio = this->nodes_priority[nodeIp] + value;
	// limit priority range, to make node priority more suscetible to changes
	if (newPrio > MAX_PRIO_VALUE) newPrio = MAX_PRIO_VALUE;
	else if (newPrio < MIN_PRIO_VALUE) newPrio = MIN_PRIO_VALUE;

	this->nodes_priority[nodeIp] = newPrio;

	lock.unlock();

	printf("New node %s priority: %d\n", inet_ntoa(nodeIp.addr.sin_addr), getNodePriority(nodeIp));
}

//assume-se que UDP é zoom fast e não há delays a espera em buffers
// push packet to output buffer
void Client::sendInfo(FS_Transfer_Info &info) {

	outputBuffer.push(info);
}

//registar RTT aquando da chegada de pacote pedido
double Client::updateNodeResponseTime(const FS_Transfer_Info& info, sys_nanoseconds timeArrived) {
	sys_nanoseconds timeSent = sys_nanoseconds(std::chrono::nanoseconds(info.packet.timestamp));
	sys_nano_diff timeDiff = timeArrived - timeSent;
	
	if (info.packet.getOpcode() == 1) { // se pacote recebido for dados de bloco pedido
		Ip nodeIp = Ip(info.addr);

		if (!this->blocksPerFile[info.packet.id][info.packet.data.blockData.blockID]) { // se for a primeira vez que bloco chega, atualiza RTT. Se for bloco duplicado pode deturpar RTT (acho?), por isso ignora-se
       		insert_regRTT(nodeIp,timeDiff);
		}
	}
	double timeScaleFactor = NodesRTT::convertToPriorityFactor(timeDiff);
	return timeScaleFactor;
}


//insere novo valor em nodes_tracker
void Client::insert_regRTT(const Ip& nodeIp, const sys_nano_diff& timeDiff) {
	std::unique_lock<std::mutex> lock(this->nodes_tracker_lock);
	nodes_tracker[nodeIp].receive2(timeDiff);
	this->nodes_tracker_lock.unlock();
}


void Client::printFull_nodes_tracker() {
	std::cout << "\nFULL PRINT FOR NODES_TRACKER----\n" << std::endl;
	for (const auto& outerPair: nodes_tracker) {
		const Ip& ip = outerPair.first;
		std::cout << "IP: " << inet_ntoa(ip.addr.sin_addr) << ", Port: " << ntohs(ip.addr.sin_port) << std::endl;

		const NodesRTT& rtts = outerPair.second;
		printf("RTTs size: %d\n", rtts.size);
		for (uint32_t i=0;i<rtts.size;i++) {
			std::cout << " " << std::endl;
			NodesRTT::printTimeDiff(rtts.arr[i]);
		}
		for (uint32_t i=0;i<NODES_RTT_TRACK_SIZE-rtts.size;i++) {
			std::cout << "no data " << std::endl;
		}
	}
	printf("\n");
}

void Client::printFull_nodes_priority() {
	std::cout << "\nFULL PRINT FOR NODES_PRIORITY----\n" << std::endl;
	for (const auto& pair : this->nodes_priority) {
		std::cout << "nodeIP : " << inet_ntoa(pair.first.addr.sin_addr) << " Priority:" << pair.second;
	}
	printf("\n");
}

void Client::initUploadLoop() {
    // creates a thread that listens in permanently, and writes the data to a buffer
    // another thread tries permanently to write to another buffer
	// another thread works on the things read into the input buffer

	// meter threads num inicializador em vez de dar detach??
    readThread = std::thread(&Client::readLoop, this);
	readThread.detach();
    writeThread = std::thread(&Client::writeLoop, this);
	writeThread.detach();
	answererThread = std::thread(&Client::answerRequestsLoop, this);
	answererThread.detach();
}

std::vector<std::pair<uint32_t, std::vector<Ip>>> Client::getBlockFiles(std::vector<FS_Track::PostFileBlocksData>& data, uint32_t* maxSize, std::vector<Ip>& allNodeIps){
    uint32_t size = 0;
	//obter o bloco com maior numero entre os recebidos de todos os nodos
    for(auto& nodeData : data){
        size = std::max(size, static_cast<uint32_t>(nodeData.block_numbers.size()));
    }

	*maxSize = size;
    std::vector<std::pair<uint32_t, std::vector<Ip>>> ans = std::vector<std::pair<uint32_t, std::vector<Ip>>>();

    for(uint32_t i = 0; i < *maxSize; i++){
        ans.emplace_back(i, std::vector<Ip>());
    }

    for(auto& nodeData : data){
        size = nodeData.block_numbers.size();

		Ip nodeIp = *getIpFromName(nodeData.hostname); // obter ip do nodo dado um nome com DNS

		allNodeIps.push_back(nodeIp); // aproveitar o loop para inserir numa estrutura auxiliar todos os IPs obtidos

        for(uint32_t i = 0; i < size; i++){
            if(!nodeData.block_numbers.at(i)) continue;

            ans.at(i).second.push_back(nodeIp);
        }
    }

    return ans;
}

void Client::commandParser(const char * dir) {
	std::string input;
    std::string command;
    std::string filename;
    FS_Track message;
    uint8_t* buffer = new uint8_t[BUFFER_SIZE];

	while (true) {
        std::cout << "What's the next command?" << std::endl;

        getline(std::cin,input);

        if(input.empty()) break;

        size_t splitAt = input.find(' ');

        command = input.substr(0, splitAt);

        filename = input.substr(splitAt+1);

        if (command == "get") {
            uint64_t hash = getFilenameHash((char*) filename.c_str(), filename.size());
			printf("Asking for file %s (%lu)\n", filename.c_str(), hash);

			FS_Track::sendGetMessage(this->socketToServer, hash);

            message = FS_Track(); // cursed

            if(!FS_Track::readMessage(message, buffer, BUFFER_SIZE, this->socketToServer)){
                print_error("No message received");
                continue;
            }

			puts("Received info from server:");

            printf("opcode: %u opt: %u size: %u hash: %lu\n", message.fsTrackGetOpcode(), (message.fsTrackGetOpt() ? 1 : 0), message.fsTrackGetSize(), message.fsTrackGetHash());

            std::vector<FS_Track::PostFileBlocksData> receivedData = message.postFileBlocksGetData();

            if(receivedData.empty()) {
				print_error("No data received");
                continue;
            }

            puts("Data received:");

            for(const auto& d : receivedData){
                std::cout << d.hostname << " with " << d.block_numbers.size() << " blocks" << std::endl;
            }

			fetchFile(dir,filename.c_str(), hash, receivedData);
        } else {
            printf("Invalid command\n");
        }
    }

    FS_Track::sendByeByeMessage(this->socketToServer);

    delete [] (uint*) buffer;
}

void Client::registerWithServer() {

	puts("Registering with server");
    std::vector<FS_Track::RegUpdateData> data = std::vector<FS_Track::RegUpdateData>();
    for(const auto& pair: this->blocksPerFile){
		printf("Adding file %lu\n", pair.first);
        data.emplace_back(pair.first, pair.second);
    }

	FS_Track::sendRegMessage(this->socketToServer, data);
	puts("File registration sent");
}

void Client::regDirectory(char* dirPath){
    std::string directory = std::string(dirPath);

    if(directory.at(directory.size()-1) != '/'){
        directory.append("/");
    }

	DIR* dir = opendir(directory.c_str());

    if(dir == nullptr){
        print_error("Error while opening specified directory.");
        return;
    }

	struct dirent* ent;


    while((ent = readdir(dir)) != nullptr){
    	// Ignore files "." and ".."
		// VERY UNSAFE, BANDAID FIX, will probably crash for files with 1 char
		// ir pelo tipo em vez do nome??

        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) this->regFile(directory.c_str(), ent->d_name);
    }
}

void Client::regFile(const char* dir, char* fn) {
    char filePath[FILENAME_BUFFER_SIZE];
    snprintf(filePath, FILENAME_BUFFER_SIZE, "%s%s", dir, fn);

	FILE* file = fopen(filePath, "r");

    if(file == nullptr){
        print_error("Error while opening file.");
        return;
    }

	std::string filename = std::string (fn);

	struct stat fileStats;

	if (stat(filePath, &fileStats))
	{
        print_error("Error while reading file size.");
        return;
	}

    uint32_t totalBlocks = fileStats.st_size / BLOCK_SIZE ;
    totalBlocks += (fileStats.st_size % BLOCK_SIZE != 0) ;

    bitMap fileBitMap = bitMap();

    for(uint32_t i = 0; i < totalBlocks; i++) fileBitMap.emplace_back(true);

    uint64_t hash = getFilenameHash(fn, strlen(fn));
    this->blocksPerFile.insert({hash, fileBitMap});
	this->currentBlocksInEachFile.insert({hash,totalBlocks});
	this->fileDescriptorMap.insert({hash, file});

	printf("file: %s hash %lu\n", filePath, hash);
}

//registar ficheiro novo que se faz GET nas estruturas de ficheiros do cliente
void Client::regNewFile(const char* dir, const char* fn, size_t size) {
	char filePath[FILENAME_BUFFER_SIZE];
    snprintf(filePath, FILENAME_BUFFER_SIZE, "%s%s", dir, fn);
	printf("file: %s\n",filePath);

	FILE* file = fopen(filePath, "wb+"); // se ficheiro já existia, reescreve tudo, senão cria novo

    if(!file){
        print_error("Error creating file structures for GET");
        return;
    }

	size_t allocSize = ((size-1) * BLOCK_SIZE) + 1;
	//prealocar ficheiro, escrevendo size bytes no ficheiro // melhor solução???
	std::vector<char> emptyBuffer(allocSize,0);

	if (std::fwrite(emptyBuffer.data(),1,allocSize ,file) != allocSize) {
		print_error("Error preallocing file for GET");
		std::fclose(file);
		return;
	}

	//bitMap a zeros
    bitMap fileBitMap = bitMap(size,false);

	uint64_t hash = getFilenameHash((char *) fn, strlen(fn));
    this->blocksPerFile.insert({hash, fileBitMap});
	this->currentBlocksInEachFile.insert({hash,0});
	this->fileDescriptorMap.insert({hash, file});
}

//process a file request
void Client::fetchFile(const char * dir, const char * filename, uint64_t hash, std::vector<FS_Track::PostFileBlocksData>& receivedData) {
	puts("Getting the file");
	uint32_t maxSize = 0;
	std::vector<Ip> allNodeIps;
	// par <este bloco, estes nodos>
	std::vector<std::pair<uint32_t, std::vector<Ip>>> block_nodes = getBlockFiles(receivedData, &maxSize, allNodeIps);

    puts("Block nodes data:");

    for(const auto& d : block_nodes){
        std::cout << "Block number: " << d.first << " can be received from nodes: ";

        for(const auto& i : d.second){
            std::cout << inet_ntoa(i.addr.sin_addr) << " ";
        }

        std::cout << std::endl;
    }

	//criar bitMap vazio para ficheiro que se fez get
	regNewFile(dir, filename, maxSize); 

	//inicializar estruturas de nodos
	for(auto& nodeIp : allNodeIps){
		this->nodes_tracker.insert({nodeIp,NodesRTT()});
		this->nodes_priority.insert({nodeIp,0});
	}

	// WRR

	int tmp = -1;
	double rtt = 0;
    bool updatedBlocks;
    int currentTries = 0;
    std::unordered_map<Ip, std::vector<uint32_t>> nodes_blocks = std::unordered_map<Ip, std::vector<uint32_t>>();

	while (tmp == -1) {
        updatedBlocks = false;
		tmp = weightedRoundRobin(hash, block_nodes, nodes_blocks, &rtt, &updatedBlocks);

        if(!updatedBlocks) currentTries++;
        else currentTries = 0;

        if(currentTries > MAX_WRR_NON_UPDATE_TRIES) {
			tmp = 0; // significa que max_tries atingidas
			break;
		}

		//std::cout << "Iteration maxRTT: " << rtt << "s" << std::endl; //ultima iteração dá sempre rtt = 0, mas tá correto, porque n chega a entrar no loop dos nodos
		std::chrono::milliseconds timeoutTime = NodesRTT::calcTimeoutTime(rtt); // tempo de timeout
		std::this_thread::sleep_for(timeoutTime);
		checkTimeoutNodes(nodes_blocks, hash, timeoutTime);
		updateFileNodesServer(hash);
	}

    if(tmp == 1){
        printFull_nodes_tracker();
        printFull_nodes_priority();
        updateFileNodesServer(hash);

        std::cout << "File transfer completed" << std::endl;
    } else{
        std::cout << "There seems to have been an error while requesting the given file. Please try again later. " << std::endl;
        std::filesystem::remove(filename);
    }

	// apagar estruturas
	this->nodes_tracker.clear();
	this->nodes_priority.clear();
}

//dispatch table
void Client::assignDispatchTable() {
	dispatchTable[0] = &Client::ReqBlockData;
	dispatchTable[1] = &Client::RespondBlockData;
}

// preenche buffer dado com bloco de ficheiro, devolve size preenchido
ssize_t Client::getFileBlock(uint64_t fileHash, uint32_t blockN, char * buffer) {
	FILE * file = this->fileDescriptorMap[fileHash];
	if (file) {
		if ((this->blocksPerFile[fileHash].size() - 1) < blockN) {
			perror("Error seeking block pos");
			return -2;
		}

		std::fseek(file,blockN * BLOCK_SIZE,SEEK_SET);
		return static_cast<ssize_t> (std::fread(buffer,1,BLOCK_SIZE,file));

	} else {
		print_error("File not found");
		return -1;
	}
}

// write new block to file
void Client::writeFileBlock(uint64_t fileHash, uint32_t blockN, char * buffer, size_t size) {
	FILE * file = this->fileDescriptorMap[fileHash];

	if (file) {
		// Se bloco pedido não existente
		// printf("bloco de nºmax: %lu, bloco pedido: %d\n", this->blocksPerFile[fileHash].size(), blockN);
		if ((this->blocksPerFile[fileHash].size() - 1) < blockN) {
			print_error("Error seeking block pos");
			return;
		}
		std::fseek(file,blockN * BLOCK_SIZE,SEEK_SET);
		size_t bytesWritten = std::fwrite(buffer,1, size,file);
		// printf("bytes written: %zu\n",bytesWritten);
		if (bytesWritten != static_cast<size_t> (size)) {
			perror("Didn't write correct size of bytes to file");
			return;
		}
		if (fflush(file) != 0) {
			print_error("Error flushing written data");
			return;
		}

	} else {
		print_error("File not found");
		return;
	}
}

// destiny Ip must be already set in info!!
// reads block requested in info, copies file blocks, sends
void Client::ReqBlockData(const FS_Transfer_Info& info) {
	FS_Transfer_Packet packet = info.packet;
	// printf("entrou no req\n");
	uint32_t reqSize = packet.getSize() / sizeof(uint32_t); // número de blocos pedidos por outro nodo
	ssize_t dataSize;
	uint32_t * blocks = static_cast <BlockRequestData *> (packet.getData())->getData();
	char blockData[BLOCK_SIZE];
	for (uint32_t i = 0; i < reqSize; i++) {
		//printf("processing block: %d\n", blocks[i]);

		//obter bloco de ficheiro
		//sleep(5); // teste de timeout
		dataSize = getFileBlock(packet.getId(),blocks[i],blockData);
		//printf("data read: %s\n",blockData);
		if (dataSize < 0) {
			perror("Couldn't find block pos in file, skipping..");
			continue; // se não for encontrado esse bloco do ficheiro, salta para o próximo e ignora
		}

		//Construir fs_transfer_packet
		blockSend.setData(blockData,dataSize);
		blockSend.setId(blocks[i]);
		dataPacket.setId(packet.getId());
		dataPacket.setOpcode(1);
		dataPacket.setTimestamp(packet.getTimestamp()); // mantém a timestamp enviada inicialmente
		dataPacket.setData(static_cast <void*> (&blockSend),sizeof(uint32_t) + dataSize); //setData já altera o size e atualiza checksum

		dataFinal.addr = info.addr; // endereço destino tem de ser passado no info recebido
		dataFinal.packet = dataPacket;

		//printResponsePacket(dataFinal); // debug
		Client::sendInfo(dataFinal);
	}
}

//write to file received block data (previously requested)
void Client::RespondBlockData(const FS_Transfer_Info& info) {
	FS_Transfer_Packet packet = info.packet;
	// printf("entrou no respond\n");
	BlockSendData * blockData = static_cast<BlockSendData *>(packet.getData());
	uint64_t fileHash = packet.getId();
	writeFileBlock(fileHash,blockData->getId(),blockData->getData(),packet.getSize() - sizeof(uint32_t));

	//atualizar bitMap do respetivo ficheiro
	this->blocksPerFile[fileHash][blockData->getId()] = true;
	this->currentBlocksInEachFile[fileHash]++;
}

//atualiza servidor com nodos possuidos atualmente
void Client::updateFileNodesServer(uint64_t fileHash) {
	bitMap& fileMap = this->blocksPerFile[fileHash];

	//Enviar ao servidor
	puts("Updating with server");
    std::vector<FS_Track::RegUpdateData> data = std::vector<FS_Track::RegUpdateData>();
	//debug

    data.emplace_back(fileHash, fileMap);
	FS_Track::sendUpdateMessage(this->socketToServer, data);
}

//compara número de nodos (aka vector<Ip>.size) que podem fornecer um bloco (uint32_t)
int cmpBlocksAvailability(std::pair<uint32_t , std::vector<Ip>>& a, std::pair<uint32_t , std::vector<Ip>>& b){
	return a.second.size() - b.second.size();
}

//args: hash do ficheiro, pares(bloco, nodos que o possuem)
// define escalonamento de blocos para diferentes nodos
// tem por base prioridades dos nodos
// começa por blocos mais raros, até mais comuns

int Client::weightedRoundRobin(uint64_t hash, std::vector<std::pair<uint32_t, std::vector<Ip>>>& block_nodes, std::unordered_map<Ip, std::vector<uint32_t>>& nodes_blocks, double* max_rtt, bool* updatedBlocks){
    nodes_blocks.clear(); //para cada ip quais blocos se vão pedir
    uint32_t maxSize = 0;
	*max_rtt = 0;

	std::sort(block_nodes.begin(), block_nodes.end(), cmpBlocksAvailability); // blocos mais raros colocados primeiro

    for (auto i = block_nodes.begin(); i != block_nodes.end(); i++){
		
		//se bloco já tiver sido recebido, desde a iteração anterior
		if(this->blocksPerFile.at(hash).at(i->first)){
			block_nodes.erase(i);
            i--;
            *updatedBlocks = true;
			continue;
		}

        Ip node = selectBestNode(i->second, nodes_blocks);

        printf("Selected node with ip %s\n", inet_ntoa(node.addr.sin_addr));

		if(nodes_blocks.at(node).size() >= MAX_BLOCKS_REQUESTS_PER_NODE) continue;

        if (nodes_blocks.find(node) == nodes_blocks.end()) {
            nodes_blocks.insert({node, std::vector<uint32_t>()});
        }

        nodes_blocks.find(node)->second.emplace_back(i->first); //colocar nº do bloco em lista de blocos a pedir a um nodo específico
        maxSize = std::max(maxSize, (uint32_t) nodes_blocks.find(node)->second.size());
    }

	if(block_nodes.size() == 0) return 1;

    FS_Transfer_Packet packet;
	FS_Transfer_Info info;
    uint32_t* arr = new uint32_t[maxSize];

    for (auto i = nodes_blocks.begin(); i != nodes_blocks.end(); i++){
        if(i->second.size() == 0) continue;

		std::unique_lock<std::mutex> lock(nodes_tracker_lock);

        std::copy(i->second.begin(), i->second.end(), arr);
        ssize_t size = i->second.size() * sizeof(uint32_t);
		*max_rtt = std::max(*max_rtt, this->nodes_tracker.at(i->first).RTT());

		lock.unlock();
		
        BlockRequestData data = BlockRequestData(arr, size);
		sys_nanoseconds sentTime = std::chrono::system_clock::now();

        packet = FS_Transfer_Packet(0, hash, sentTime, &data, (uint32_t) size);

		info = FS_Transfer_Info(packet, i->first);

        printf("Sending data to node %s\nData being sent: ", inet_ntoa(info.addr.sin_addr));

        for(uint32_t j = 0; j < i->second.size(); j++){
            printf("%d ", arr[j]);
        }

        puts("");

		// PrintRequestPacket(info); // debug
	
        Client::sendInfo(info);
    }

    delete [] arr;

	return -1;
}

// selecionar nodo com maior prioridade, entre vários que possuem um bloco específico
// podiamos fazer sort mas por agora fazer assim
Ip Client::selectBestNode(std::vector<Ip>& available_nodes, std::unordered_map<Ip, std::vector<uint32_t>>& nodes_blocks){
    uint32_t size = available_nodes.size();
    Ip ans = available_nodes.at(0);

    if(nodes_blocks.find(ans) == nodes_blocks.end()) nodes_blocks.insert({ans, std::vector<uint32_t>(0)});

    for (uint32_t i = 1; i < size; i++) {
        Ip cur = available_nodes.at(i);

        if(nodes_blocks.find(cur) == nodes_blocks.end()) nodes_blocks.insert({cur, std::vector<uint32_t>(0)});

        if(nodes_blocks.at(cur).size() >= MAX_BLOCKS_REQUESTS_PER_NODE) continue;

        if(nodes_blocks.at(ans).size() >= MAX_BLOCKS_REQUESTS_PER_NODE) ans = cur;

        else if(this->nodes_priority.at(cur) > this->nodes_priority.at(ans)) ans = cur;
    }

    return ans;
}

// gethostbyname is an obsolete function according to man page
Ip *Client::getIpFromName(const std::string name) {
	printf("looking up %s\n", name.c_str());
	// nao me apeteceu usar find() e iterators ate me deu umas dores so de pensar
	if (nameToIP.contains(name) == false) {
		// DNS lookup
		    const char *hostname = name.c_str();
			struct addrinfo hints, *result, *rp;
			int status;

			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			status = getaddrinfo(hostname, NULL, &hints, &result);
			if (status != 0) {
				fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
				exit(EXIT_FAILURE);
			}

			// vou assumir que nao da mais que 1 nome mas o loop fica aqui da mesma
			Ip ip;
			for (rp = result; rp != NULL; rp = rp->ai_next) {
				struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)rp->ai_addr;

				char ip_address[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(ipv4_addr->sin_addr), ip_address, INET_ADDRSTRLEN);
				printf("Name %s resolved to %s\n", hostname, ip_address);
				ip = Ip(*ipv4_addr);
				printf("found %s\n", inet_ntoa(ipv4_addr->sin_addr));

				// porta UDP fica aqui
				ip.addr.sin_family = AF_INET;
				ip.addr.sin_port = htons(UDP_PORT);

				nameToIP[name] = ip;
			}

			freeaddrinfo(result);
	}
	// cursed se der mais do que 1 nome ou !!!!!!!!!!!!!!!!!!!!!
	// printf("returning %s\n", inet_ntoa(nameToIP[name].addr.sin_addr));
	return &nameToIP[name];
}
