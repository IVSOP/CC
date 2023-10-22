#ifndef TP2_FS_TRACK_TEST_H
#define TP2_FS_TRACK_TEST_H

void set_RegData(FS_Track* data);
void read_RegData(FS_Track* data);
void testRegData();

void set_IDAssignment(FS_Track* data);
void read_IDAssignment(FS_Track* data);
void testIDAssignment();

void set_UpdateFileBlocks(FS_Track* data);
void read_UpdateFileBlocks(FS_Track* data);
void testUpdateFileBlocks();

void set_PostFileBlocks(FS_Track* data);
void read_PostFileBlocks(FS_Track* data);
void testPostFileBlocks();

void set_ErrorMessage(FS_Track* data);
void read_ErrorMessage(FS_Track* data);
void testErrorMessage();

void testCommunication (void set(FS_Track*), void get(FS_Track*));

#endif //TP2_FS_TRACK_TEST_H
