#ifndef TP2_FS_TRACK_TEST_H
#define TP2_FS_TRACK_TEST_H

/**
 * Test function to sed RegUpdateData
 * @param data FS_Track Message
 */
void set_RegUpdateData(FS_Track *data);

/**
 * Test function to read RegUpdateData
 * @param data FS_Track Message
 */
void read_RegUpdateData(FS_Track *data);

/**
 * Function that tests RegUpdateData communication
 */
void testRegUpdateData();

/**
 * Test function to sed PostFileBlocks
 * @param data FS_Track Message
 */
void set_PostFileBlocks(FS_Track *data);

/**
 * Test function to read PostFileBlocks
 * @param data FS_Track Message
 */
void read_PostFileBlocks(FS_Track *data);

/**
 * Function that tests PostFileBlocks communication
 */
void testPostFileBlocks();

/**
 * Test function to sed ErrorMessage
 * @param data FS_Track Message
 */
void set_ErrorMessage(FS_Track *data);

/**
 * Test function to read ErrorMessage
 * @param data FS_Track Message
 */
void read_ErrorMessage(FS_Track *data);

/**
 * Function that tests ErrorMessage communication
 */
void testErrorMessage();

/**
 * Function that tests the communication between protocols
 * @param set Function that sets protocol's data
 * @param get Function that reads protocol's data
 */
void testCommunication(void set(FS_Track *), void get(FS_Track *));

#endif //TP2_FS_TRACK_TEST_H
