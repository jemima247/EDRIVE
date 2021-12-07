#include "file.h"
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>



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

    char * filePathC =  (char*)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);
    strncpy(filePathC, filePath, MAX_FILE_PATH_LENGTH);
    char * fileName = strtok(filePathC, "/");
    


   // loop through the string to extract all other tokens
   char* temp = fileName;
   while( temp != NULL ) {
     fileName = temp;
      temp = strtok(NULL, "/");
   }
   printf("%s here\n", fileName);

   size_t fileNameLen = strlen(fileName);
   if(write(fd, &fileNameLen, sizeof(size_t)) != sizeof(size_t)){
     return -1;
   }

   //send file name now
   size_t bytes_writtenFN = 0;
   while(bytes_writtenFN < fileNameLen){
     //Try to write the filename
     ssize_t uc = write(fd, fileName + bytes_writtenFN, fileNameLen - bytes_writtenFN);

     // Did the write fail? If so, return an error
        if (uc <= 0) return -1;

        // If there was no error, write returned the number of bytes written
        bytes_writtenFN += uc;

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
   
    
    // }

    printf("got here5\n");

    size_t bytes_written = 0;
    while (bytes_written < size) {
      printf("in bytes\n");
        // Try to write the entire file
        ssize_t rc = write(fd, data + bytes_written, size - bytes_written);

        // Did the write fail? If so, return an error
        if (rc <= 0) return -1;

        // If there was no error, write returned the number of bytes written
        bytes_written += rc;
    }

    return 0;

}

char** receive_file(int fd) {
  // First try to read in the username length
  char** username_and_file = malloc(sizeof(char*) * 3);

  // The first iteration of the for loop reads in the username, and the second reads in the file array They are stored to 
  //  username_and_file[0] and username_and_file[1], respectively.
  for(int i = 0; i < 3; i++){
    size_t len;
    if (read(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
      // Reading failed. Return an error
      return NULL;
    }

    // Now make sure the string length is reasonable
    if (len > MAX_FILE_PATH_LENGTH) {
      errno = EINVAL;
      return NULL;
    }

    // Allocate space for the character array of string
    char* result = malloc(len + 1);

    // Try to read the string. Loop until the entire string has been read.
    size_t bytes_read = 0;
    while (bytes_read < len) {
      // Try to read the entire remaining string
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
    username_and_file[i] = result;
  }
  printf("178\n");

  FILE *fp;
  fp  = fopen (username_and_file[0], "w");

  fputs(username_and_file[2], fp);
  fclose(fp);
  //update the array to hold the name of the file instead of the string of the whole file
  printf("FIle created\n");



  return username_and_file;
}

//constructe the if modified function that takes the path of the file in the client since its onle
//if the client modifies the file it should send it back to the server
int if_modified(char* filePath, time_t last_modified_in_client){
  struct stat sb;
  stat(filePath, &sb);
  time_t last_modified = sb.st_mtime;
  if(last_modified_in_client == last_modified){
    return 0;
  }
  else{
    return 1;
  }
}


//also I think for the server to update the file it does the samem receive file method it just has to find to already saved files and give 
//it the state of edited.
