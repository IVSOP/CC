//
// Created by kard on 19-10-2023.
//

#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include <checksum.h>
#include "client.h"


void test_udp () {
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
void test_fs_transfer_fields(){

	//criar blocos para dar request
	uint32_t blockIds[10] = {1,259,546,4,1,250,1,639,1000,25913};

	BlockRequestData * blocks = new BlockRequestData(blockIds,10 * sizeof(uint32_t));

	//criar pacote para enviar
	FS_Transfer_Packet * packet = new FS_Transfer_Packet(2, 524, blocks, 10 * sizeof(uint32_t));

	//testar campos do pacote
	printf("packet opcode: %d, size: %d, id: %llu, checksum: %d, data: ", packet->getOpcode(), packet->getSize(), packet->getId(), packet->getChecksum());

	uint32_t * testBlockIds = static_cast<BlockRequestData *> (packet->getData())->getData();

	for(int i = 0; i < 10; i++) printf("%d,", testBlockIds[i]);

	printf("\n\n");
	//teste com blockSendData agora e setters
	//criar dados de bloco para enviar
	char dataStr[] = "Hello World, teste, teste, peste, leste, ruski, putin";
	int strLen = sizeof(dataStr);
	BlockSendData * data = new BlockSendData(502501,dataStr,strLen);
	// alterar diferentes parâmetros no pacote anterior, inclusive dados (agora são dados de bloco pedido)
	packet->setId(31020120);
	packet->setOpcode(1);
	packet->setData(data, strLen + sizeof(uint32_t)); // será tamanho do array, mais id associado ao BlockSendData
	// setData já atualiza checksum e size

	//testar campos
	printf("packet opcode: %d, size: %d, id: %llu, checksum: %d, data: ", packet->getOpcode(), packet->getSize(), packet->getId(), packet->getChecksum());

	char * blockData = static_cast<BlockSendData *> (packet->getData())->getData();
	printf("blockData block: %d , data: ", static_cast<BlockSendData *> (packet->getData())->getId());
	for(int i = 0; i < strLen; i++) printf("%c", blockData[i]);

	delete(blocks);
	delete(packet);
	delete(data);
}

//testar receber do outro lado de conexão UDP
void test_fs_transfer_sendData() {
	// udp continua a dar erro a bind de socket -> não cheguei a fazer
	char * buf = nullptr;
	FS_Transfer_Packet * packet = new FS_Transfer_Packet();
	packet->fs_transfer_read_buffer(buf,-1);

}

int main() {
	Client client;

	return 0;
}
