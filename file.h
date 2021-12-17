#include <time.h>

#pragma once
#define MAX_FILE_PATH_LENGTH 260

// Send a across a socket with a header that includes the message length. Returns non-zero value if
// an error occurs.
int sending_file(int fd, const char* filePath, const char* username);

// Receive a message from a socket and return the message string (which must be freed later).
// Returns NULL when an error occurs.
char** receive_file(int fd);


//check if file was modifiedx
int if_modified(char* filePath, time_t last_modified_in_client);