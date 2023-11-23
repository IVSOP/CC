#include "UDP_socket.h"
#include "TCP_socket.h"
#include "fs_transfer.h"
#include "fs_track.h"
#include "fs_track_test.h"
#include "checksum.h"
#include "errors.h"
#include "client.h"
#include "socket_common.h"


void test_udp() {
    NodeUDPSocket serverSocket = NodeUDPSocket("127.15.15.15");

    NodeUDPSocket clientSocket = NodeUDPSocket("127.15.15.16");

    if (fork() == 0) {
        char buff[50] = "Hello World";
        sockaddr_in dest;
        memcpy(&dest, &serverSocket.node_addr, sizeof(struct sockaddr_in));
        clientSocket.sendData(buff, 50, &dest);
        serverSocket.closeSocket();
        clientSocket.closeSocket();
        _exit(0);
    } else {
        char buff2[50];
        sockaddr_in ola;
        serverSocket.receiveData(buff2, 50, &ola);
        printf("%s\n", buff2);
        clientSocket.closeSocket();
        serverSocket.closeSocket();
    }

    serverSocket.closeSocket();
    clientSocket.closeSocket();
    printf("TESTE");

}

//testar getters, setters, construtores fs_transfer related
void test_fs_transfer_fields() {

    //criar blocos para dar request
    uint32_t blockIds[10] = {1, 259, 546, 4, 1, 250, 1, 639, 1000, 25913};

    BlockRequestData *blocks = new BlockRequestData(blockIds, 10 * sizeof(uint32_t));

    //criar pacote para enviar
    FS_Transfer_Packet *packet = new FS_Transfer_Packet(2, 524, blocks, 10 * sizeof(uint32_t));

    //testar campos do pacote
    printf("packet opcode: %d, size: %d, hash: %llu, checksum: %d, data: ", packet->getOpcode(), packet->getSize(),
           packet->getId(), packet->getChecksum());

    uint32_t *testBlockIds = static_cast<BlockRequestData *> (packet->getData())->getData();

    for (int i = 0; i < 10; i++) printf("%d,", testBlockIds[i]);

    printf("\n\n");
    //teste com blockSendData agora e setters
    //criar dados de bloco para enviar
    char dataStr[] = "Hello World, teste, teste, peste, leste, ruski, putin";
    int strLen = sizeof(dataStr);
    BlockSendData *data = new BlockSendData(502501, dataStr, strLen);
    // alterar diferentes parâmetros no pacote anterior, inclusive dados (agora são dados de bloco pedido)
    packet->setId(31020120);
    packet->setOpcode(1);
    packet->setData(data, strLen + sizeof(uint32_t)); // será tamanho do array, mais hash associado ao BlockSendData
    // setData já atualiza checksum e size

    //testar campos
    printf("packet opcode: %d, size: %d, hash: %llu checksum: %d, data: ", packet->getOpcode(), packet->getSize(),
           packet->getId(), packet->getChecksum());

    char *blockData = static_cast<BlockSendData *> (packet->getData())->getData();
    printf("blockData block: %d , data: ", static_cast<BlockSendData *> (packet->getData())->getId());
    for (int i = 0; i < strLen; i++) printf("%c", blockData[i]);

    delete (blocks);
    delete (packet);
    delete (data);
}

//testar receber do outro lado de conexão UDP
void test_fs_transfer_sendData() {
    // udp continua a dar erro a bind de socket -> não cheguei a fazer
    char *buf = nullptr;
    FS_Transfer_Packet *packet = new FS_Transfer_Packet();
    packet->fs_transfer_read_buffer(buf, -1);

}

// construtor do client deixou de fazer o que isto precisa
// #include <socket_common.h>
// void new_test_fs_transfer() {
//     Client sender("127.15.15.15");
// 	Client receiver("127.14.14.14");


// 	FS_Transfer_Info info;
// 	info.timestamp = 0;
// 	// memcpy(&info.addr, &receiver.udpSocket.node_addr, sizeof(struct sockaddr_in));
// 	setIPv4("127.14.14.14", &info.addr);
// 	char str[] = "Hello World";
// 	info.packet.setData(str, sizeof(str));
// 	info.packet.setId(69);
// 	info.packet.setOpcode(1);

// 	puts("sending info");
// 	sender.sendInfo(info);
// 	puts("sent");

// 	while (true) {
// 		// faz todo o sentido do mundo, sem isto, o programa manda pacotes infinitamente
// 		// (main thread sai da main e as outras ficam a falar sozinhas e bugam)
// 	}
// }

void test_file_write_block(char * dir) {
    Client c(dir);
    //uint32_t blockID[] = {0,1,2};
    uint32_t blockID[] = {2,3};
    BlockRequestData block = BlockRequestData(blockID,sizeof(blockID));
    char filename[] = "teste.txt";
    c.regNewFile(dir,filename, 1024*2 + 15); // registar ficheiro
    FS_Transfer_Packet packet = FS_Transfer_Packet(0,getFilenameHash(filename,strlen(filename)),&block,sizeof(blockID));
    FS_Transfer_Info info;
    info.packet = packet;
    c.ReqBlockData(info);

    printf("mimir\n");
    sleep(1000);
}

void test_file_read_block(char * dir) {
    Client c(dir);
    char teste[] = "Shiban é mau pastor, não é como o nestor, as threads andam sem motor";
    uint32_t blockRequested = 1;
    BlockSendData block = BlockSendData(blockRequested,teste,sizeof(teste)+ sizeof(uint32_t));
    char filename[] = "teste.txt";
    c.regNewFile(dir,filename,1024*2 + 15); // registar ficheiro
    FS_Transfer_Packet packet = FS_Transfer_Packet(2,getFilenameHash(filename,strlen(filename)),&block,sizeof(teste) + sizeof(uint32_t));
    FS_Transfer_Info info;
    info.packet = packet;
    c.RespondBlockData(info);

    printf("mimir\n");
    sleep(1000);
}

int nadaaver() {

    /*
	if (argc == 1) {
		puts("server IP not passed as argument");
		exit(EXIT_FAILURE);
	}

	char *server_ip = argv[1];
	Client client(server_ip);

	char c;
	FS_Transfer_Info info;
	char IP[20];
	char message[100];
	// NOTA:!!!!!!!!!!!!!!!!!!!!!!! ler com cuidado, ha muitas threads a dar print em background e a ordem fica manhosa
	while (true) {
		puts("send packet? y/n: ");
		std::cin >> c;
		if (c == 'y') {
			printf("please enter IP: ");
			std::cin >> IP;
			setIPv4(IP, &info.addr);
		
			printf("enter message: ");
			std::cin >> message;
			info.packet.setData(message, 100); // muda size e checksum
		
			client.sendInfo(info);
		} else {
			while (true) {
				// fica em loop infinito para dar print as mensagens, com break saia da main
			}
		}
	}
    */

    ClientTCPSocket client = ClientTCPSocket("0.0.0.0");
    FS_Track* data = nullptr;
    std::pair<uint8_t *, uint32_t> buf;

    data = new FS_Track(0);

    setRegUpdateData(data);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(1);

    setRegUpdateData(data);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(2, 1);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(3, 150);

    setPostFileBlocks(data);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(4);

    setErrorMessage(data);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(5);

    buf = data->FS_Track::fsTrackToBuffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    delete[] (uint8_t*) buf.first;

    delete data;

    return 0;
}

void test_node_reg_packet_times() {
    Client c;

    // ----- fingir nodo que envia pedido ---------
    const char* ipAddress = "127.0.0.1";
    const int port = UDP_PORT;

    sockaddr_in ip;
    std::memset(&ip, 0, sizeof(ip));
    ip.sin_family = AF_INET;
    ip.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &ip.sin_addr);

    uint32_t blockID[] = {2,3};
    BlockRequestData block = BlockRequestData(blockID,sizeof(blockID));
    char filename[] = "teste123.txt";
    FS_Transfer_Packet packet = FS_Transfer_Packet(0,getFilenameHash(filename,strlen(filename)),&block,sizeof(blockID));
    FS_Transfer_Info info; // pacote com pedido de blocos enviado
    info.packet = packet;
    info.addr = ip;

    sys_nanoseconds sentTime = std::chrono::system_clock::now();
    std::cout << "Sent time: " << std::endl;
    c.printTimePoint(sentTime);

    // enviar pedido de bloco ---

    c.regPacketSentTime(info,sentTime); 
    c.printFull_node_sent_reg();

    //esperar 3 seg --
    sleep(3); 

    uint32_t blockRequested = 2;
    char teste[] = "dados";
    BlockSendData block2 = BlockSendData(blockRequested,teste,sizeof(teste)+ sizeof(uint32_t));
    FS_Transfer_Packet packet2 = FS_Transfer_Packet(2,getFilenameHash(filename,strlen(filename)),&block2,sizeof(teste) + sizeof(uint32_t));
    FS_Transfer_Info info2; // pacote com dados de bloco recebidos
    info2.packet = packet2;
    info2.addr = ip; //mesmo ip para o qual enviou o pacote antes

    sys_nanoseconds sentTime2 = std::chrono::system_clock::now();
    std::cout << "\nSent time2: " << std::endl;
    c.printTimePoint(sentTime2);

    // enviar mesmo pedido ---
    c.regPacketSentTime(info,sentTime); // new time shouldn't be recorded for same block of same file

    //esperar 3 seg --
    sleep(3); 

    // -- fingir que nodo recebe bloco pedido --------

    sys_nanoseconds receivedTime = std::chrono::system_clock::now();
    std::cout << "\nReceived time: " << std::endl;
    c.printTimePoint(receivedTime);

    // receber pacote ---
    c.updateNodeResponseTime(info2,receivedTime);

    // receber mesmo pacote ---
    c.updateNodeResponseTime(info2,receivedTime); // delivery of same block of same file should be ignored
    double estimatedRTT = c.nodes_tracker[Ip(ip)].RTT();

    //estimatedRTT deve ter em conta diferença entre primeiro pedido e primeira receção

    // estimativa só tem precisao de 7 decimais à direita do segundo??? Acho que é do MAC
    std::cout << "\nestimatedRTT: " << estimatedRTT << "\n" << std::endl; 
    c.printFull_node_sent_reg();
    c.printFull_nodes_tracker();
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        print_error("Not enough arguments");
        return -1;
    }

    Client client = Client(argv[1], argv[2]);

    std::cout << "The end" << std::endl;

    //test_node_reg_packet_times();
    // test_file_read_block(argv[1]);
    //test_file_write_block(argv[1]);
    //nadaaver(argc, argv);
}


