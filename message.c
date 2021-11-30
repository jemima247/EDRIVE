#include "message.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// Send a across a socket with a header that includes the message length.
int send_message(int fd, const char* message, const char* username) {
  // If the message is NULL, set errno to EINVAL and return an error
  if (message == NULL) {
    errno = EINVAL;
    return -1;
  }


  size_t usernameLen  = strlen(username);
  if(write(fd, &usernameLen, sizeof(size_t)) != sizeof(size_t)){
    return -1;
  }

  // Send the size of the username and then the username first
  size_t bytes_writtenU = 0;
  while (bytes_writtenU < usernameLen) {
    // Try to write the entire remaining username
    ssize_t uc = write(fd, username + bytes_writtenU, usernameLen - bytes_writtenU);

    // Did the write fail? If so, return an error
    if (uc <= 0) return -1;

    // If there was no error, write returned the number of bytes written
    bytes_writtenU += uc;
  }

  // Now send the size of the message and then the message
  // First, send the length of the message in a size_t
  size_t len = strlen(message);
  if (write(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
    // Writing failed, so return an error
    return -1;
  }

  

  // Now we can send the message. Loop until the entire message has been written.
  size_t bytes_written = 0;
  while (bytes_written < len) {
    // Try to write the entire remaining message
    ssize_t rc = write(fd, message + bytes_written, len - bytes_written);

    // Did the write fail? If so, return an error
    if (rc <= 0) return -1;

    // If there was no error, write returned the number of bytes written
    bytes_written += rc;
  }

  return 0;
}

// Receive a message from a socket and return a 2-element array with the username and message strings (which must be freed later)
char** receive_message(int fd) {
  // First try to read in the username length
  char** usernamemessage = malloc(sizeof(char*) * 2);

  // The first iteration of the for loop reads in the username, and the second reads in the message. They are stored to 
  //  usernameMessage[0] and usernameMessage[1], respectively.
  for(int i = 0; i < 2; i++){
    size_t len;
    if (read(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
      // Reading failed. Return an error
      return NULL;
    }

    // Now make sure the message length is reasonable
    if (len > MAX_MESSAGE_LENGTH) {
      errno = EINVAL;
      return NULL;
    }

    // Allocate space for the message
    char* result = malloc(len + 1);

    // Try to read the message. Loop until the entire message has been read.
    size_t bytes_read = 0;
    while (bytes_read < len) {
      // Try to read the entire remaining message
      ssize_t rc = read(fd, result + bytes_read, len - bytes_read);

      // Did the read fail? If so, return an error
      if (rc <= 0) {
        free(result);
        return NULL;
      }

      // Update the number of bytes read
      bytes_read += rc;
    }

    result[len] = '\0';
    usernamemessage[i] = result;
  }
  return usernamemessage;
}
