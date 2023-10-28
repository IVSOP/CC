#include <cstring>
#include <iostream>
#include <cctype>
#include "cmdline.h"

// parser dentro de fs_tracker
void fs_tracker_cmdParser(char *port) {
    printf("FS_Tracker\nport: %s\n", port);
    //start server
}

//parser dentro de fs_node
void fs_node_cmdParser(char *argv[]) {
    printf("FS_Node\n");
    char input[MAX_INPUT_LEN];
    char *curr = input;
    char *folder = argv[0];
    char *ip = argv[1];
    char *port = argv[2];
    printf("%s, %s, %s\n", folder, ip, port);

    while (true) {
        //Get input
        std::cin.getline(input, MAX_INPUT_LEN);
        input[strcspn(input, "\n")] = '\0';
        strsep(&curr, " ");
        if (!strcmp(input, "exit")) {
            break;
        } else if (!strcmp(input, "GET")) {
            printf("%s\n", curr);
            // download
        } else {
            printf("Invalid command\n");
        }
        curr = input;
    }
}

//recebe os dados diretos do main
//encaminha para fs_tracker ou fs_node
int main_cmdParser(int argc, char *argv[]) {
    if (argc < 1) {
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