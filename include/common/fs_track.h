
#ifndef TP2_FS_TRACK_H
#define TP2_FS_TRACK_H

#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include <string>
#include <csignal>
#include "TCP_socket.h"
#include "bitmap.h"

#define FILENAME_SIZE 256
#define SIZE_LENGTH 3

class FS_Track {
private:
    /**
     * Sets message data's size
     */
    void setSize(uint32_t bytes);

    /**
     * Message data's current size
     */

    uint32_t dataSize;
public:
    /**
     * Default constructor
     */
    FS_Track();

    /**
     * Parameterized constructor. File hash code is not included
     * @param opcode Message OPCode: indicates the type of message
     */
    FS_Track(uint8_t opcode);

    /**
     * Parameterized constructor
     * @param opcode Message OPCode: indicates the type of message
     * @param hash Message hash: Indicates which file we are referring. If opts value is false, then this value is ignored
     */
    FS_Track(uint8_t opcode, uint64_t hash);

    /**
     * Des-contructor
     */
    ~FS_Track();

    /**
     * Struct used to send data related to registration or update of files from a FS_Node
     */
    struct RegUpdateData {
        uint64_t file_hash;
        bitMap block_numbers;

        RegUpdateData(uint64_t, bitMap);

        ~RegUpdateData();

        uint64_t getFileHash();

        bitMap getBlockNumbers(); //faz deep copy
    };

    /**
     * Struct used by FS_Track server to send the file blocks each node contains
     */
    struct PostFileBlocksData {
        struct in_addr ip;
        bitMap block_numbers;

        PostFileBlocksData(struct in_addr, bitMap);

        ~PostFileBlocksData();
    };

    /**
     * Struct used to send details about an error
     */
    struct ErrorMessageData {
        std::string details;

        ErrorMessageData(std::string);

        ~ErrorMessageData();
    };

    /**
     * 8 bits, from which the first 7 refer to the header OPCode and the last one refers to the header options
     */
    uint8_t opcode_opts;

    /**
     * 24 bits used to refer the data size, in bytes
     */
    uint8_t size[SIZE_LENGTH];

    /**
     * 64 bits used to identify a file
     */
    uint64_t hash;

    /**
     * FS_Track protocol data
     */
    void *data;

    /**
     * Function that reads the OPCode, Options and size of a message from a buffer
     * @param buf Buffer
     * @param size Buffer's size
     */
    void fsTrackHeaderReadBuffer(void *buf, ssize_t size);

    /**
     * Function that reads the hash value from a buffer
     * @param buf Buffer
     * @param size Buffer's size
     */
    void fsTrackReadHash(void *buf, ssize_t size);

    /**
     * Function that converts a FS_Track message into a buffer
     * @return The buffer containing the FS_Track info and it's size
     */
    std::pair<uint8_t *, uint32_t> fsTrackToBuffer();

    /**
     * Function that reads a buffer and converts it into the FS_Track data
     * @param buf Buffer
     * @param size Buffer's size
     */
    void setData(void * buf, uint32_t size);

    /**
     * Get FS_Track's OPCode
     * @return FS_Track's OPCode
     */
    uint8_t fsTrackGetOpcode();

    /**
     * Get FS_Track's Options
     * @return FS_Track's Options
     */
    uint8_t fsTrackGetOpt();

    /**
     * Get FS_Track's Size
     * @return FS_Track's Size
     */
    uint32_t fsTrackGetSize();

    /**
     * Get FS_Track's Hash
     * @return FS_Track's Hash
     */
    uint64_t fsTrackGetHash();

    /**
     * Function that converts a list of RegUpdateData into FS_Track's data value
     * @param data List of RegUpdateData
     */
    void regUpdateDataSetData(const std::vector<RegUpdateData> &data);

    /**
     * Function that converts FS_Track's data value into a list of RegUpdateData
     * @return List of RegUpdateData
     */
    std::vector<RegUpdateData> regUpdateDataGetData();

    /**
     * Function that converts a list of PostFileBlocksData into FS_Track's data value
     * @param data List of PostFileBlocksData
     */
    void postFileBlocksSetData(const std::vector<PostFileBlocksData> &data);

    /**
     * Function that converts FS_Track's data value into a list of PostFileBlocksData
     * @return List of PostFileBlocksData
     */
    std::vector<PostFileBlocksData> postFileBlocksGetData();

    /**
     * Function that converts a string with error's details into FS_Track's data value
     * @param data String with error's details
     */
    void errorMessageSetData(std::string &details);

    /**
     * Function that converts FS_Track's data value into a string with error's details
     * @return String with error's details
     */
    ErrorMessageData errorMessageGetData();

    /**
     * Send any FS_Track to desired destiny
     * @param socket Message destiny
     * @param message FSTrack message
     */
    static ssize_t sendMessage(ClientTCPSocket& socket, FS_Track& message){
        std::pair<uint8_t *, uint32_t> buf = message.FS_Track::fsTrackToBuffer();
        ssize_t ans = socket.sendData(buf.first, buf.second);

        delete [] (uint8_t*) buf.first;

        return ans;
    }

    /**
     * Send any FS_Track to desired destiny
     * @param socket Message destiny
     * @param message FSTrack message
     */
    static void sendMessage(ServerTCPSocket::SocketInfo& socket, FS_Track& message){
        std::pair<uint8_t *, uint32_t> buf = message.FS_Track::fsTrackToBuffer();
        socket.sendData(buf.first, buf.second);

        delete [] (uint8_t*) buf.first;
    }

    /**
     * Send register message to desired destiny
     * @param socket Socket to send message through
     * @param data RegUpdate data to send
     */
    static void sendRegMessage(ClientTCPSocket& socket, std::vector<FS_Track::RegUpdateData>& data){
        FS_Track message = FS_Track(0);

        message.regUpdateDataSetData(data);

        sendMessage(socket, message);
    }

    /**
     * Send update message to desired destiny
     * @param socket Socket to send message through
     * @param data RegUpdate data to send
     */
    static void sendUpdateMessage(ClientTCPSocket& socket, std::vector<FS_Track::RegUpdateData>& data){
        FS_Track message = FS_Track(1);

        message.regUpdateDataSetData(data);

        printf("Update message with size %d\n", message.fsTrackGetSize());

        sendMessage(socket, message);
    }

    /**
     * Send get message
     * @param socket Socket to send message through
     * @param hash File's hash
     */
    static ssize_t sendGetMessage(ClientTCPSocket& socket, uint64_t hash){
        FS_Track message = FS_Track(2, hash);

        return sendMessage(socket, message);
    }

    /**
     * Send post message to desired destiny
     * @param socket Socket to send message through
     * @param hash File's hash
     * @param data Post data to send
     */
    static void sendPostMessage(ServerTCPSocket::SocketInfo& socket, uint64_t hash, std::vector<FS_Track::PostFileBlocksData>& data) {
        FS_Track message = FS_Track(3, hash);

        message.postFileBlocksSetData(data);

        sendMessage(socket, message);
    }

    /**
     * Send error message to desired destiny
     * @param socket Socket to send message through
     * @param errorDetails Error details
     */
    static void sendErrorMessage(ClientTCPSocket& socket, std::string& errorDetails) {
        FS_Track message = FS_Track(4);

        message.errorMessageSetData(errorDetails);

        sendMessage(socket, message);
    }

    /**
     * Send error message to desired destiny
     * @param socket Socket to send message through
     * @param errorDetails Error details
     */
    static void sendErrorMessage(ServerTCPSocket::SocketInfo& socket, std::string& errorDetails) {
        FS_Track message = FS_Track(4);

        message.errorMessageSetData(errorDetails);

        sendMessage(socket, message);
    }

    /**
     * Send bye bye message to server in order to disconnect client
     * @param socket Socket to send message through
     */
    static void sendByeByeMessage(ClientTCPSocket& socket) {
        FS_Track message = FS_Track(5);

        sendMessage(socket, message);
    }

    static bool readMessage(FS_Track& message, void* buf, ssize_t bufSize, ServerTCPSocket::SocketInfo& connection){
        uint8_t *buffer = (uint8_t*) buf;
        uint32_t bytes, remainBytes;
        uint32_t bufferSize = (uint32_t) bufSize;

        bytes = connection.receiveData(buffer, 4);

        if (bytes <= 0) return false;

        message.fsTrackHeaderReadBuffer(buffer, bytes);

        if (message.fsTrackGetOpt() == 1) {
            bytes = connection.receiveData(buffer, 8);
            message.fsTrackReadHash(buffer, bytes);
        }

        remainBytes = message.fsTrackGetSize();

        while (remainBytes > 0) {
            bytes = connection.receiveData(buffer, std::min(bufferSize, remainBytes));

            message.setData(buffer, bytes);

            remainBytes -= bytes;
        }

        return true;
    }

    static bool readMessage(FS_Track& message, void* buf, ssize_t bufSize, ClientTCPSocket& connection){
        uint8_t *buffer = (uint8_t*) buf;
        uint32_t bytes, remainBytes;
        uint32_t bufferSize = (uint32_t) bufSize;

        bytes = connection.receiveData(buffer, 4);

        if (bytes <= 0) return false;

        message.fsTrackHeaderReadBuffer(buffer, bytes);

        if (message.fsTrackGetOpt() == 1) {
            bytes = connection.receiveData(buffer, 8);
            message.fsTrackReadHash(buffer, bytes);
        }

        remainBytes = message.fsTrackGetSize();

        while (remainBytes > 0) {
            bytes = connection.receiveData(buffer, std::min(bufferSize, remainBytes));

            message.setData(buffer, bytes);

            remainBytes -= bytes;
        }

        return true;
    }

};

#endif //TP2_FS_TRACK_H
