#include "file.h"
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>




int sending_file(int fd, const char* filePath, const char* username){


    // Try to open the file
    FILE* input = fopen(filePath, "r");
    if (input == NULL) {
        perror("Unable to open input file");
        exit(1);
    }

    // Seek to the end of the file so we can get its size
    if (fseek(input, 0, SEEK_END) != 0) {
        perror("Unable to seek to end of file");
        exit(2);
    }

    // Get the size of the file
    size_t size = ftell(input);

    // Seek back to the beginning of the file
    if (fseek(input, 0, SEEK_SET) != 0) {
        perror("Unable to seek to beginning of file");
        exit(2);
    }

    // Allocate a buffer to hold the file contents. We know the size in bytes, so
    // there's no need to multiply to get the size we pass to malloc in this case.
    uint8_t* data = malloc(size);

    // Read the file contents
    if (fread(data, 1, size, input) != size) {
        fprintf(stderr, "Failed to read entire file\n");
        exit(2);
    }


    //send the username length
    size_t usernameLen  = strlen(username);
    if(write(fd, &usernameLen, sizeof(size_t)) != sizeof(size_t)){
        return -1;
    }

    // Send the size the username first
    size_t bytes_writtenU = 0;
    while (bytes_writtenU < usernameLen) {
        // Try to write the entire remaining username
        ssize_t uc = write(fd, username + bytes_writtenU, usernameLen - bytes_writtenU);

        // Did the write fail? If so, return an error
        if (uc <= 0) return -1;

        // If there was no error, write returned the number of bytes written
        bytes_writtenU += uc;
    }

    // Now send the size of the file and then the file bytes by bytes
    // First, send the byte in a size_t

    if (write(fd, &size, sizeof(size_t)) != sizeof(size_t)) {
        // Writing failed, so return an error
        return -1;
    }
    int i =0;
    while(i< size){
        char c = data[i];
        
        //send file containts as arrays /strings so starting from data to data+size


    }



}