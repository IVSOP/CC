#include "client.h"
#include "fs_track.h"
#include <dirent.h>
#include "errors.h"
#include <sys/stat.h> // TODO
#include <algorithm>
#include <condition_variable>
#include <iomanip>

#define SERVER_IP "0.0.0.0"
#define FILENAME_BUFFER_SIZE 300
#define BUFFER_SIZE 1500

Client::Client() // por default sockets aceitam tudo
    : socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), dispatchTable(), nodes_tracker_lock(),
	nodes_priority(), nodes_tracker(), node_sent_reg()
{
    // register
    initUploadLoop();
    assignDispatchTable();
    commandParser("./files"); // folder standard para ficheiros? importa?
	//regDirectory("./files"); // improta?
}

Client::Client(char* dir) // por default sockets aceitam tudo
	: socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), fileDescriptorMap(), dispatchTable(), nodes_tracker_lock(),
	nodes_priority(), nodes_tracker(), node_sent_reg()
{
    // register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser(dir);
}

Client::Client(char* dir, const std::string &IPv4)
	: socketToServer(IPv4), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), currentBlocksInEachFile(), fileDescriptorMap(), dispatchTable(), nodes_tracker_lock(),
	nodes_priority(), nodes_tracker(), node_sent_reg()
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
    
void printPacket(FS_Transfer_Info& info) {
	printf("checksum:%u (it is %s), opc: %u size: %u id: %llu\ndata as string: %s\n", info.packet.checksum,
		info.packet.calculateChecksum() == info.packet.checksum ? "correct" : "wrong",
		info.packet.getOpcode(), info.packet.getSize(), info.packet.id, reinterpret_cast<char *>(&info.packet.data));
}

// read from socket into a buffer, does nothing else
void Client::readLoop() {
	puts("Init reading thread");
	// very bad, copying memory for no reason
	FS_Transfer_Info info;
	while (true) {
		udpSocket.receiveData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
		
		info.timestamp = std::chrono::system_clock::now(); // isto ainda é preciso??
		inputBuffer.push(info);

		puts("packet received");
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

		puts("packet sent");
    }
}

// loop trying to answer a request
void Client::answerRequestsLoop() {
	FS_Transfer_Info info;
	while (true) {
		info = inputBuffer.pop();
		sys_nanoseconds timeArrived = std::chrono::system_clock::now();
		updateNodeResponseTime(info, timeArrived); // se vier pacote com erro, contabilizo na mesma para RTT, independemente se está correto ou não
		if (info.packet.checkErrors() == false) { // se houver erro nao faz sentido estar a ler o opcode, check tem de ser feito aqui
			wrongChecksum(info);
		} else {
			rightChecksum(info);
		}
	}
}

// o que fazer se checksum falhar
void Client::wrongChecksum(const FS_Transfer_Info&info) {
	updateNodePriority(Ip(info.addr), NODE_VALUE_WRONG);
	// reenvio de nodo será feito no WRR, não precisa de ser feito aqui
}

void Client::rightChecksum(const FS_Transfer_Info& info) {
	updateNodePriority(Ip(info.addr), NODE_VALUE_SUCCESS);
	uint8_t opcode = info.packet.getOpcode();
	if (opcode <= 1) {
		(this->*dispatchTable[opcode])(info); // assign data to function with respective opcode
	} else {
		perror("No handler found for packet Opcode");
		return;
	}
	//tirei temporariamente
	//printPacket(info);
}

//obter prioridade de nodo
uint32_t Client::getNodePriority(const Ip& nodeIp) {
	std::unique_lock<std::mutex> lock(this->nodes_tracker_lock);
	uint32_t res = this->nodes_priority[nodeIp];
	lock.unlock();
	return res;
}

//incrementa valor à prioridade de nodo
void Client::updateNodePriority(const Ip& nodeIp, uint32_t value) {
	std::unique_lock<std::mutex> lock(this->nodes_tracker_lock);
	this->nodes_priority[nodeIp] += value;
	lock.unlock();
}

//assume-se que UDP é zoom fast e não há delays a espera em buffers
// push packet to output buffer
void Client::sendInfo(FS_Transfer_Info &info) {
	sys_nanoseconds receivedTime = std::chrono::system_clock::now(); // voltei a atualizar aqui o tempo para ser mais accurate, caguei onde tiver antes definido já
	regPacketSentTime(info, receivedTime);
	outputBuffer.push(info);
}

//registar tempo de envio de um pacote
void Client::regPacketSentTime(FS_Transfer_Info &info, sys_nanoseconds timestamp) {
	if (info.packet.getOpcode() == 0) {
		BlockRequestData * block = static_cast<BlockRequestData *> (info.packet.getData());
		Ip nodeIp = Ip(info.addr);
		int size = info.packet.getSize() / sizeof(uint32_t); //tamanho do array
		for (int i = 0; i < size; i++) {
			printf("processing block: %d\n", block->getData()[i]);
			insert_regPacket(nodeIp,info.packet.id,block->getData()[i],timestamp);
			
		}
	}
}

//registar RTT aquando da chegada de pacote pedido
void Client::updateNodeResponseTime(FS_Transfer_Info& info, sys_nanoseconds timeArrived) {
	sys_nanoseconds timeSent;
	if (info.packet.getOpcode() == 2) { // se pacote recebido for dados de bloco pedido
		Ip nodeIp = Ip(info.addr);
		BlockSendData * block = static_cast<BlockSendData *> (info.packet.getData());
		bool found = find_remove_regPacket(nodeIp,info.packet.id,block->blockID, &timeSent);
		if (found) // se registo de bloco recebido ainda existe, aka não estamos a receber dados de bloco duplicado
			insert_regRTT(nodeIp,timeSent,timeArrived); // atualizar lista de RTT do nodo com novo RTT
	}
}

//inserts packet into node_sent_reg
void Client::insert_regPacket(const Ip& nodeIp, uint64_t file, uint32_t blockN, const sys_nanoseconds& startTime) {
	// Se nodo não existir, criar mapa novo
	if (node_sent_reg.find(nodeIp) == node_sent_reg.end()) {
		std::unordered_map<std::pair<uint64_t, uint32_t>, sys_nanoseconds, KeyHash> innerMap = std::unordered_map<std::pair<uint64_t, uint32_t>, sys_nanoseconds, KeyHash>();
		node_sent_reg.emplace(nodeIp, innerMap);
	} 

	auto& innerMap = node_sent_reg[nodeIp];

	// Se par não existe para esse nodo, criar
	std::pair<uint64_t, uint32_t> pair = std::make_pair(file,blockN);
	if (innerMap.find(pair) == innerMap.end()) {
		innerMap.insert({pair,startTime});
	}
	//Se já existe par, está a pedir repetidamente um bloco -> Mantemos tempo mais antigo de pedido de bloco para o cálculo do RTT
}

//finds packet in node_sent_reg and removes it if found
// returns false if not found
bool Client::find_remove_regPacket(const Ip& nodeIp, uint64_t file, uint32_t blockN, sys_nanoseconds * retValue) {
	printf("entered find_remove\n");
	bool result = false;
	//se existir dados para nodo, e respetivo par
	auto outerIt = node_sent_reg.find(nodeIp);

	if (outerIt != node_sent_reg.end()) { // se nodo existir (desnecessario mas so por seguranca)
		std::unordered_map<std::pair<uint64_t, uint32_t>, sys_nanoseconds, KeyHash>& innerMap = outerIt->second;
		std::pair<uint64_t, uint32_t> pair = {file,blockN};
		auto innerIt = innerMap.find(pair);
		if (innerIt != innerMap.end()) {// se par existir -> significa que é primeira vez que este bloco é recebido
			result = true;
			*retValue = innerIt->second;
			innerMap.erase(pair); //dá para receber o iterador na posição atual como argumento 
		}
	}
	return result;
}

//insere novo valor em nodes_tracker
void Client::insert_regRTT(const Ip& nodeIp, const sys_nanoseconds& timeSent, const sys_nanoseconds& timeReceived) {
	if (nodes_tracker.find(nodeIp) == nodes_tracker.end()) {
		nodes_tracker.insert({nodeIp,NodesRTT()});
	}
	nodes_tracker[nodeIp].receive(timeSent,timeReceived);
}

void Client::printFull_node_sent_reg() {
	std::cout << "\nFULL PRINT FOR NODE_SENT_REG----" << std::endl;
	for( const auto& outerPair: node_sent_reg) {
		const Ip& ip = outerPair.first;
 		std::cout << "IP: " << inet_ntoa(ip.addr.sin_addr) << ", Port: " << ntohs(ip.addr.sin_port) << std::endl;

		for(const auto& innerPair: outerPair.second) {
			const std::pair<uint64_t, uint32_t>& key = innerPair.first;
			sys_nanoseconds value = innerPair.second;
			//const std::time_t t_c = std::chrono::system_clock::to_time_t(value);
			std::cout << "  Key: {" << key.first << ", " << key.second << "}, Value: " << std::endl;
			printTimePoint(value);
		}
	}
}

void Client::printFull_nodes_tracker() {
	std::cout << "\nFULL PRINT FOR NODES_TRACKER----\n" << std::endl;
	for (const auto& outerPair: nodes_tracker) {
		const Ip& ip = outerPair.first;
		std::cout << "IP: " << inet_ntoa(ip.addr.sin_addr) << ", Port: " << ntohs(ip.addr.sin_port) << std::endl;

		const NodesRTT& rtts = outerPair.second;
		for (uint32_t i=0;i<rtts.size;i++) {
			std::cout << " " << std::endl;
			printTimeDiff(rtts.arr[i]);
		}
		for (uint32_t i=0;i<NODES_RTT_TRACK_SIZE-rtts.size;i++) {
			std::cout << "no data " << std::endl;
		}
	}
}

void Client::printTimePoint(const sys_nanoseconds& timePoint) {
    // Convert time point to std::tm
	auto sys_time_milliseconds = std::chrono::time_point_cast<std::chrono::milliseconds>(timePoint);
    std::time_t t_c = std::chrono::system_clock::to_time_t(sys_time_milliseconds);
	std::tm tm = *std::gmtime(&t_c);

    // Get the fractional seconds
    auto fractional_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(timePoint.time_since_epoch()).count();

    // Print the date with maximum precision
    std::cout << "Date: " << std::put_time(&tm, "%F %T") << "." << std::setprecision(9) 
		<< fractional_seconds - static_cast<int>(fractional_seconds) << " UTC" << std::endl;
}

void Client::printTimeDiff(const sys_nano_diff& timeDiff) {
    // Convert nanoseconds to seconds for printing
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(timeDiff).count();
	 std::cout.precision(15);
    // Print the duration in seconds with maximum precision
	std::cout << "Duration in nanoseconds: " << timeDiff.count() << "ns" << std::endl;
    std::cout << "Duration in seconds: " << seconds << " seconds" << std::endl;
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

std::vector<std::pair<uint32_t, std::vector<Ip>>> Client::getBlockFiles(std::vector<FS_Track::PostFileBlocksData>& data, uint32_t* maxSize){
    uint32_t size;
    for(auto& nodeData : data){
        size = nodeData.block_numbers.size();
        *maxSize = std::max(*maxSize, size);
    }

    std::vector<std::pair<uint32_t, std::vector<Ip>>> ans = std::vector<std::pair<uint32_t, std::vector<Ip>>>();

    for(uint32_t i = 0; i < *maxSize; i++){
        ans.emplace_back(i, std::vector<Ip>());
    }

    for(auto& nodeData : data){
        size = nodeData.block_numbers.size();
			
		struct sockaddr_in addr;
		addr.sin_addr = nodeData.ip;
		addr.sin_port = htons(UDP_PORT);
		addr.sin_family = AF_INET;

        for(uint32_t i = 0; i < size; i++){
            if(!nodeData.block_numbers.at(i)) continue;


            ans.at(i).second.emplace_back(addr);
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

			FS_Track::sendGetMessage(this->socketToServer, hash);

            message = FS_Track();

            if(!FS_Track::readMessage(message, buffer, BUFFER_SIZE, this->socketToServer)){
                std::cout << "No message received" << std::endl;
                continue;
            }

            std::cout << (uint32_t) message.fsTrackGetOpcode() << ' ' << (message.fsTrackGetOpt() ? 1 : 0) << ' ' << message.fsTrackGetSize() << ' ' << message.fsTrackGetHash() << std::endl;

            std::vector<FS_Track::PostFileBlocksData> receivedData = message.postFileBlocksGetData();

            if(receivedData.empty()) {
                std::cout << "No data received" << std::endl;
                continue;
            }

			fetchFile(dir,filename.c_str(), hash, receivedData);

        } else {
            printf("Invalid command\n");
        }
    }

    delete [] (uint*) buffer;
}

void Client::registerWithServer(){
    std::vector<FS_Track::RegUpdateData> data = std::vector<FS_Track::RegUpdateData>();
    for(const auto& pair: this->blocksPerFile){

        data.emplace_back(pair.first, pair.second);
    }

	FS_Track::sendRegMessage(this->socketToServer, data);
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


    // Ignore files "." and ".."
    for(int i = 0; i < 2; i++) ent = readdir(dir);

    while((ent = readdir(dir)) != nullptr){
        this->regFile(directory.c_str(), ent->d_name);
    }
}

void Client::regFile(const char* dir, char* fn){
    char filePath[FILENAME_BUFFER_SIZE];
    snprintf(filePath, FILENAME_BUFFER_SIZE, "%s%s", dir, fn);
	// printf("file: %s\n",filePath);

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

	//prealocar ficheiro, escrevendo size bytes no ficheiro // melhor solução???
	std::vector<char> emptyBuffer(size * BLOCK_SIZE,0);

	if (std::fwrite(emptyBuffer.data(),1,size * BLOCK_SIZE ,file) != size * BLOCK_SIZE) {
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

	uint32_t maxSize = 0;
	std::vector<std::pair<uint32_t, std::vector<Ip>>> block_nodes = getBlockFiles(receivedData, &maxSize);

	// printf("file maxSize: %d", maxSize);
	// for (auto& nodeData : receivedData) {
	// 	printf("\n%d ",nodeData.ip.s_addr);
	// 	for (auto val : nodeData.block_numbers)
	// 		std::cout << (val == true) << std::endl;
	// }
	//criar bitMap vazio para ficheiro que se fez get
	regNewFile(dir, filename, maxSize); 

	//inicializar estruturas de nodos
	//TODO: apagar mais a cima nos regRTT os ifs constantes de adicionar novo nodo, alocar tudo aqui é mais facil
	for(auto& nodeData : receivedData){
		struct sockaddr_in addr;
		addr.sin_addr = nodeData.ip;
		addr.sin_port = htons(UDP_PORT);
		addr.sin_family = AF_INET;
		Ip nodeIp = Ip(addr);

		this->nodes_tracker.insert({nodeIp,NodesRTT()});
		this->nodes_priority.insert({nodeIp,0});
	}

	// WRR

	int tmp = -1;
	double rtt = 0;

	while (tmp == -1) {
		tmp = weightedRoundRobin(hash, block_nodes, &rtt);
		
		std::this_thread::sleep_for(std::chrono::milliseconds((int) ((rtt+10)*5)));
		//se der timeout onde digo isso??
	}

	std::cout << "File transfer completed\n" << std::endl;

	// apagar estruturas
	this->nodes_tracker.clear();
	this->nodes_priority.clear();
	this->node_sent_reg.clear();
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
		dataPacket.setData(static_cast <void*> (&blockSend),sizeof(uint32_t) + dataSize); //setData já altera o size e atualiza checksum

		dataFinal.addr = info.addr; // endereço destino tem de ser passado no info recebido
		dataFinal.packet = dataPacket;
		// dataFinal.timestamp = std::time(nullptr); // tempo é definido mesmo antes do push, não necessário aqui

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

//compara número de nodos (aka vector<Ip>.size) que podem fornecer um bloco (uint32_t)
int cmpBlocksAvailability(std::pair<uint32_t , std::vector<Ip>>& a, std::pair<uint32_t , std::vector<Ip>>& b){
	return a.second.size() - b.second.size();
}

//args: hash do ficheiro, pares(bloco, nodos que o possuem)
// define escalonamento de blocos para diferentes nodos
// tem por base prioridades dos nodos
// começa por blocos mais raros, até mais comuns

int Client::weightedRoundRobin(uint64_t hash, std::vector<std::pair<uint32_t, std::vector<Ip>>>& block_nodes, double* max_rtt){
    std::unordered_map<Ip, std::vector<uint32_t>> nodes_blocks; //para cada ip quais blocos se vão pedir
    uint32_t maxSize = 0;
	*max_rtt = 0;

	std::sort(block_nodes.begin(), block_nodes.end(), cmpBlocksAvailability); // blocos mais raros colocados primeiro

    for (auto i = block_nodes.begin(); i != block_nodes.end(); i++){
		
		if(this->blocksPerFile.at(hash).at(i->first)){
			block_nodes.erase(i);
            i--;
			continue;
		}

        Ip node = selectBestNode(i->second, nodes_blocks);

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
		std::unique_lock<std::mutex> lock(nodes_tracker_lock);
        std::copy(i->second.begin(), i->second.end(), arr);
        ssize_t size = i->second.size() * sizeof(uint32_t);
		*max_rtt = std::max(*max_rtt, this->nodes_tracker.at(i->first).RTT());
		lock.unlock();

        BlockRequestData data = BlockRequestData(arr, size);

        packet = FS_Transfer_Packet(0, hash, &data, (uint32_t) size);

		info = FS_Transfer_Info(packet, i->first);

        Client::sendInfo(info);

		// apaguei daqui!!!!!!!!!!!
    }

    delete [] arr;

	return -1;
}

// selecionar nodo com maior prioridade, entre vários que possuem um bloco específico
// podiamos fazer sort mas por agora fazer assim
Ip Client::selectBestNode(std::vector<Ip>& available_nodes, std::unordered_map<Ip, std::vector<uint32_t>>& nodes_blocks){
    uint32_t size = available_nodes.size();
    Ip ans = available_nodes.at(0);

    for (uint32_t i = 1; i < size; i++) {
        Ip cur = available_nodes.at(i);

		if(nodes_blocks.at(cur).size() >= MAX_BLOCKS_REQUESTS_PER_NODE) continue;

		if(nodes_blocks.at(ans).size() >= MAX_BLOCKS_REQUESTS_PER_NODE) cur = ans;

        else if(this->nodes_priority.at(cur) > this->nodes_priority.at(ans)) ans = cur;
    }

    return ans;
}
