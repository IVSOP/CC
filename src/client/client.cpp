#include "client.h"
#include "fs_track.h"
#include <dirent.h>
#include "errors.h"
#include <sys/stat.h> // TODO

#define SERVER_IP "0.0.0.0"
#define FILENAME_BUFFER_SIZE 300
#define BUFFER_SIZE 1500

Client::Client() // por default sockets aceitam tudo
        : socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), fileDescriptorMap(), dispatchTable()
{
    // register
    initUploadLoop();
    assignDispatchTable();
    commandParser();
}

Client::Client(char* dir) // por default sockets aceitam tudo
: socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), fileDescriptorMap(), dispatchTable()
{
    // register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser();
}

Client::Client(char* dir, const std::string &IPv4)
: socketToServer(IPv4), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile(), fileDescriptorMap(), dispatchTable()
{
    // register
    initUploadLoop();
	assignDispatchTable();
    regDirectory(dir);
    registerWithServer();
    commandParser();
}

Client::~Client() {
    udpSocket.closeSocket();
}

void printPacket(FS_Transfer_Info& info) {
	printf("checksum:%u (it is %s), opc: %u size: %u id: %lu\ndata as string: %s\n", info.packet.checksum,
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
		// falta timestamp!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		std::time(&info.timestamp); // acho que isto funcemina?
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

// loops trying to answer a request
void Client::answerRequestsLoop() {
	FS_Transfer_Info info;
	FS_Transfer_Packet data;
	while (true) {
		info = inputBuffer.pop();
		data = info.packet;

		if (data.checkErrors() == false) { // se houver erro nao faz sentido estar a ler o opcode, check tem de ser feito aqui
			wrongChecksum(info);
		} else {
			uint8_t opcode = data.getOpcode();
			if (opcode <= 1) {
				(this->*dispatchTable[opcode])(data); // assign data to function with respective opcode
			} else {
				perror("No handler found for packet Opcode");
				return;
			}
			//tirei temporariamente
			//printPacket(info);
		}
	}
}

//voltar a meter const FS_Transfer_Info depois. Tirei para testes
void Client::sendInfo(FS_Transfer_Info &info) {
	BlockSendData * block = static_cast<BlockSendData *> (info.packet.getData());
	block->data[(info.packet.getSize() - 4)] = '\0';
	printf("BlockID: %d data: %s\n",block->blockID,block->data);
	outputBuffer.push(info);
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

void Client::commandParser() {
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

            for(auto& postData : receivedData)
                std::cout << postData.ip.s_addr << std::endl;

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

	FILE* file = fopen(filePath, "r");

    if(file == nullptr){
        print_error("Error while opening file.");
        return;
    }

	std::string filename = std::string (fn);

	this->fileDescriptorMap.insert({filename, file});

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
}
//parse requests

void Client::assignDispatchTable() {
	dispatchTable[0] = &Client::ReqBlockData;
	dispatchTable[1] = &Client::RespondBlockData;
}


// preenche buffer dado com bloco de ficheiro, devolve size preenchido
// error checking para bloco não existir?
ssize_t Client::getFileBlock(const std::string& filename, uint32_t blockN, char * buffer) {
	std::ifstream file(filename, std::ios::binary);
	if (file) {
		file.seekg(blockN * BLOCK_SIZE);
		file.read(buffer,BLOCK_SIZE);
		if (file.gcount() == 0) {
			perror("Error seeking block pos");
			return -2;
		}
		return file.gcount();
	}
	else {
		perror("File not found");
		return -1;
	}
}

// write new block to file
void Client::writeFileBlock(const std::string& filename, uint32_t blockN, char * buffer, ssize_t size) {
	std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
	std::vector<char> empty_block(BLOCK_SIZE, 0);
	if (!file) { //se deu erro ao criar
		perror("Error creating file to receive given blocks");
		return;
	}
	// calcular bloco máx atual
	file.seekg(0,std::ios::end);
	uint32_t total_blocks = static_cast<uint32_t> (file.tellg());
	if ((total_blocks / BLOCK_SIZE) % BLOCK_SIZE != 0) { // extremamente ineficiente, mudar depois !!! forma rápida de calcular max block
		total_blocks++;
	}
	//preencher com espaço branco até chegar ao bloco pretendido
	for (uint32_t i=total_blocks;i<blockN;i++) {
		file.write(empty_block.data(),BLOCK_SIZE);
	}

	file.seekg(blockN * BLOCK_SIZE);
	file.write(buffer,size);
	file.close();
}

//interpret block requests
void Client::ReqBlockData(FS_Transfer_Packet& packet) {
	printf("entrou no req\n");
	uint32_t reqSize = packet.getSize() / sizeof(uint32_t); // número de blocos
	ssize_t dataSize;
	uint32_t * blocks = static_cast <BlockRequestData *> (packet.getData())->getData();
	char blockData[BLOCK_SIZE];
	std::string filename = "/Users/peters/Desktop/CC_TP2/teste.txt";
	for (uint32_t i = 0; i < reqSize; i++) {
		printf("processing block: %d\n", blocks[i]);
		//obter bloco de ficheiro

		// TODO: filename = packet.getId()
		dataSize = getFileBlock(filename,blocks[i],blockData);
		if (dataSize < 0) continue; // se não for encontrado esse bloco do ficheiro, salta para o próximo e ignora
		//Construir fs_transfer_packet
		blockSend.setData(blockData,dataSize); //copia duas vezes os dados ??
		blockSend.setId(blocks[i]);
		dataPacket.setId(packet.getId());
		dataPacket.setOpcode(1);
		dataPacket.setData(static_cast <void*> (&blockSend),sizeof(uint32_t) + dataSize); //setData já altera o size

		//dataFinal.addr = ???
		dataFinal.packet = dataPacket;

		Client::sendInfo(dataFinal);
	}
}

//receive block data (previously requested)
void Client::RespondBlockData(FS_Transfer_Packet& packet) {
	printf("entrou no respond\n");
	std::string filename = "/Users/peters/Desktop/CC_TP2/teste.txt";
	BlockSendData * blockData = static_cast<BlockSendData *>(packet.getData());
	// TODO: filename = getFilename(packet.getId())
	writeFileBlock(filename,blockData->getId(),blockData->getData(),packet.getSize() - sizeof(uint32_t));
	// TODO: write to blocksPerFile new block
}

void Client::weightedRoundRobin(uint64_t hash, std::unordered_map<uint32_t , std::vector<uint32_t>>& block_nodes, std::unordered_map<uint32_t, uint32_t>& nodes_requested_blocks){
    std::unordered_map<uint32_t , std::vector<uint32_t>> nodes_blocks;
    uint32_t maxSize = 0;

    for(auto i = block_nodes.begin(); i != block_nodes.end(); i++){
        uint32_t node = selectNode(i->second, nodes_requested_blocks);

        if(nodes_blocks.find(node) == nodes_blocks.end()){
            nodes_blocks.insert({node, std::vector<uint32_t>()});
        }

        nodes_blocks.find(node)->second.emplace_back(i->first);
        maxSize = std::max(maxSize, (uint32_t) nodes_blocks.find(node)->second.size());
    }

    FS_Transfer_Packet packet;
    uint32_t* arr = new uint32_t[maxSize];

    for(auto i = nodes_blocks.begin(); i != nodes_blocks.end(); i++){
        std::copy(i->second.begin(), i->second.end(), arr);
        ssize_t size = i->second.size() * sizeof(uint32_t);

        BlockRequestData data = BlockRequestData(arr, size);

        packet = FS_Transfer_Packet(0, hash, &data, (uint32_t) size);

        // TODO Acabar
    }

    delete [] arr;
}

uint32_t Client::selectNode(std::vector<uint32_t>& available_nodes, std::unordered_map<uint32_t, uint32_t>& nodes_requested_blocks){
    uint32_t size = available_nodes.size();
    uint32_t ans = available_nodes.at(0);
    uint32_t cur;

    for(uint32_t i = 1; i < size; i++){
        cur = available_nodes.at(i);

        if(nodes_requested_blocks.at(cur) > nodes_requested_blocks.at(ans)) ans = cur;
    }

    return ans;
}

