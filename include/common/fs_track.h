
#ifndef TP2_FS_TRACK_H
#define TP2_FS_TRACK_H

#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include <string>
#include "TCP_socket.h"

#define FILENAME_SIZE 256
#define SIZE_LENGTH 3

class FS_Track {
private:
    /**
     * Sets message data's size
     */
    void set_Size(uint32_t);

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
     * Parameterized constructor
     * @param opcode Message OPCode: indicates the type of message
     * @param opts Message Opts: Indicates if hash is included or not
     * @param hash Message hash: Indicates which file we are referring. If opts value is false, then this value is ignored
     */
    FS_Track(uint8_t opcode, bool opts, uint64_t hash);

    /**
     * Des-contructor
     */
    ~FS_Track();

    /**
     * Struct used to send data related to registration or update of files from a FS_Node
     */
    struct RegUpdateData {
        uint64_t file_hash;
        std::vector<uint32_t> block_numbers;

        RegUpdateData(uint64_t, std::vector<uint32_t>);

        ~RegUpdateData();

        uint64_t getFileHash();

        std::vector<uint32_t> getBlockNumbers(); //faz deep copy
    };

    /**
     * Struct used by FS_Track server to send the file blocks each node contains
     */
    struct PostFileBlocksData {
        struct in_addr ip;
        std::vector<uint32_t> block_numbers;

        PostFileBlocksData(struct in_addr, std::vector<uint32_t>);

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
    void fs_track_header_read_buffer(void *buf, ssize_t size);

    /**
     * Function that reads the hash value from a buffer
     * @param buf Buffer
     * @param size Buffer's size
     */
    void fs_track_read_hash(void *buf, ssize_t size);

    /**
     * Function that converts a FS_Track message into a buffer
     * @return The buffer containing the FS_Track info and it's size
     */
    std::pair<uint8_t *, uint32_t> fs_track_to_buffer();

    /**
     * Function that reads a buffer and converts it into the FS_Track data
     * @param buf Buffer
     * @param size Buffer's size
     */
    void set_data(void * buf, uint32_t size);

    /**
     * Get FS_Track's OPCode
     * @return FS_Track's OPCode
     */
    uint8_t fs_track_getOpcode();

    /**
     * Get FS_Track's Options
     * @return FS_Track's Options
     */
    uint8_t fs_track_getOpt();

    /**
     * Get FS_Track's Size
     * @return FS_Track's Size
     */
    uint32_t fs_track_getSize();

    /**
     * Get FS_Track's Hash
     * @return FS_Track's Hash
     */
    uint64_t fs_track_getHash();

    /**
     * Function that converts a list of RegUpdateData into FS_Track's data value
     * @param data List of RegUpdateData
     */
    void RegUpdateData_set_data(const std::vector<RegUpdateData> &data);

    /**
     * Function that converts FS_Track's data value into a list of RegUpdateData
     * @return List of RegUpdateData
     */
    std::vector<RegUpdateData> RegUpdateData_get_data();

    /**
     * Function that converts a list of PostFileBlocksData into FS_Track's data value
     * @param data List of PostFileBlocksData
     */
    void PostFileBlocks_set_data(const std::vector<PostFileBlocksData> &data);

    /**
     * Function that converts FS_Track's data value into a list of PostFileBlocksData
     * @return List of PostFileBlocksData
     */
    std::vector<PostFileBlocksData> PostFileBlocks_get_data();

    /**
     * Function that converts a string with error's details into FS_Track's data value
     * @param data String with error's details
     */
    void ErrorMessage_set_data(std::string &details);

    /**
     * Function that converts FS_Track's data value into a string with error's details
     * @return String with error's details
     */
    ErrorMessageData ErrorMessage_get_data();

    /**
     * Send any FS_Track to desired destiny
     * @param ip Message destiny
     * @param message FSTrack message
     */
    static void send_message(ClientTCPSocket& socket, FS_Track& message){
        std::pair<uint8_t *, uint32_t> buf = message.FS_Track::fs_track_to_buffer();
        socket.sendData(buf.first, buf.second);

        delete [] (uint8_t*) buf.first;
    }

    /**
     * Send register message to desired destiny
     * @param socket Socket to send message through
     * @param data RegUpdate data to send
     */
    static void send_reg_message(ClientTCPSocket& socket, std::vector<FS_Track::RegUpdateData>& data){
        FS_Track message = FS_Track(0, false, 0);

        message.RegUpdateData_set_data(data);

        send_message(socket, message);
    }

    /**
     * Send update message to desired destiny
     * @param socket Socket to send message through
     * @param data RegUpdate data to send
     */
    static void send_update_message(ClientTCPSocket& socket, std::vector<FS_Track::RegUpdateData>& data){
        FS_Track message = FS_Track(1, false, 0);

        message.RegUpdateData_set_data(data);

        send_message(socket, message);
    }

    /**
     * Send get message
     * @param socket Socket to send message through
     * @param hash File's hash
     */
    static void send_get_message(ClientTCPSocket& socket, uint64_t hash){
        FS_Track message = FS_Track(2, true, hash);

        send_message(socket, message);
    }

    /**
     * Send post message to desired destiny
     * @param socket Socket to send message through
     * @param hash File's hash
     * @param data Post data to send
     */
    static void send_post_message(ClientTCPSocket& socket, uint64_t hash, std::vector<FS_Track::PostFileBlocksData>& data) {
        FS_Track message = FS_Track(3, true, hash);

        message.PostFileBlocks_set_data(data);

        send_message(socket, message);
    }

    /**
     * Send error message to desired destiny
     * @param socket Socket to send message through
     * @param errorDetails Error details
     */
    static void send_error_message(ClientTCPSocket& socket, std::string& errorDetails) {
        FS_Track message = FS_Track(4, false, 0);

        message.ErrorMessage_set_data(errorDetails);

        send_message(socket, message);
    }

};

#endif //TP2_FS_TRACK_H
