#include <cstring>
#include <iostream>
#include <cctype>
#include <vector>
#include <string>
#include <sstream>
#include "cmdline.h"

// parser dentro de fs_tracker
void fs_tracker_cmdParser(char *port) {
    printf("FS_Tracker\nport: %s\n", port);
    //start server
}

//parser dentro de fs_node
void fs_node_cmdParser(char *argv[]) {
    printf("FS_Node\n");
    std::string input;
    std::string command;
    std::string filename;

    while (true) {
        getline(std::cin,input);

        if(input.size() == 0) break;

        size_t splitAt = input.find(' ');

        command = input.substr(0, splitAt);

        filename = input.substr(splitAt+1);

        if (command.compare("GET") == 0) {
            // TODO Send server a get message, receive response and retrive blocks from other nodes
            std::cout << filename << std::endl;

            // download
        } else {
            printf("Invalid command\n");
        }
    }

    printf("Acabou, nao tem mais jeito.\n");
}

//recebe os dados diretos do main
//encaminha para fs_tracker ou fs_node
int main_cmdParser(int argc, char *argv[]) {
    if (argc < 2) {
        printf("must run one of: \n FS_Tracker \n; FS_Node <folder> ip port\n");
        return -1;
    }
    if (!strcmp(argv[1], "FS_Tracker")) {
        fs_tracker_cmdParser(argv[2]);
    } else if (!strcmp(argv[1], "FS_Node")) {
        fs_node_cmdParser(argv + 2);
    }
    return 0;
}