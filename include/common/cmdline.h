#ifndef CMDLINE_H
#define CMDLINE_H

#define MAX_FILENAME_LEN 256
#define MAX_INPUT_LEN  MAX_FILENAME_LEN + 5

void fs_tracker_cmdParser(char * port);
void fs_node_cmdParser(char * argv[]);
int main_cmdParser(int argc, char * argv[]);

#endif