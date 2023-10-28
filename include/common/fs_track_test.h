#ifndef TP2_FS_TRACK_TEST_H
#define TP2_FS_TRACK_TEST_H


void set_RegUpdateData(FS_Track *data);

void read_RegUpdateData(FS_Track *data);

void testRegUpdateData();

void set_PostFileBlocks(FS_Track *data);

void read_PostFileBlocks(FS_Track *data);

void testPostFileBlocks();

void set_ErrorMessage(FS_Track *data);

void read_ErrorMessage(FS_Track *data);

void testErrorMessage();

void testCommunication(void set(FS_Track *), void get(FS_Track *));

#endif //TP2_FS_TRACK_TEST_H
