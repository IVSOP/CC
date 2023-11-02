#include "UDP_socket.h"
#include "TCP_socket.h"
#include "fs_transfer.h"
#include "fs_track.h"
#include "fs_track_test.h"
#include <checksum.h>
#include "client.h"
#include "cmdline.h"


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
    printf("packet opcode: %d, size: %d, hash: %lu, checksum: %d, data: ", packet->getOpcode(), packet->getSize(),
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
    printf("packet opcode: %d, size: %d, hash: %lu checksum: %d, data: ", packet->getOpcode(), packet->getSize(),
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

#include "socket_common.h"

int main(int argc, char *argv[]) {
    Client client = Client();

    ClientTCPSocket socket = ClientTCPSocket("0.0.0.0");

    bitMap b = bitMap();
    b.push_back(false);
    b.push_back(true);
    b.push_back(true);
    b.push_back(false);

    client.blocksPerFile.insert({1, b});
    client.registerWithServer(socket);

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

    /*
    ClientTCPSocket client = ClientTCPSocket("0.0.0.0");
    FS_Track* data = nullptr;
    std::pair<uint8_t *, uint32_t> buf;

    data = new FS_Track(0, false, 82);

    set_RegUpdateData(data);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(1, true, 124);

    set_RegUpdateData(data);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(2, true, 1);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(3, true, 150);

    set_PostFileBlocks(data);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(4, false, 200);

    set_ErrorMessage(data);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    sleep(5);

    delete[] (uint8_t*) buf.first;

    delete data;

    data = new FS_Track(5, false, 0);

    buf = data->FS_Track::fs_track_to_buffer();

    std::cout << buf.second << std::endl;

    client.sendData(buf.first, buf.second);

    delete[] (uint8_t*) buf.first;

    delete data;

    return 0;
    */
}
