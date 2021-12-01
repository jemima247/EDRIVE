#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"
#include "message.h"
#include "file.h"

#include <assert.h>
#include <signal.h>

#define MAX_MESSAGE_LENGTH 2048
#define MAX_FILE_PATH_LENGTH 260



// Keep the username for this machine in a global so we can access it from the callback
const char* username;

void* send_message_thread(void* args){
  int* server_socket = (int*)args;
  while(1){
    char* message = (char*)malloc(sizeof(char*));
    if (fgets(message, MAX_MESSAGE_LENGTH, stdin) == NULL){
      perror("fgets\n");
      exit(EXIT_FAILURE);
    }
    char* new_message = strtok(message, "\n"); //removing trailing new line char

    //split command
    if(strcmp(new_message, "send") == 0){
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1){
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      char* filePath = (char*)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);

      

      if (fgets(filePath, MAX_FILE_PATH_LENGTH, stdin) == NULL){
        perror("fgets for filepath\n");
        exit(EXIT_FAILURE);
      }
      
      char* new_filePath = strtok(filePath, "\n"); //removing trailing new line char
      


      rc = sending_file(*server_socket, new_filePath, username);
      if (rc == -1){
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      free(filePath);
      

    }
    else{
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1){
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
    }
    
    free(message);
  }


}

void* receive_message_thread(void* args){
  // get the socket for the connection to receive messages from
  int* server_socket = (int*)args;
  while(1){
    // Read a message from the server
    char** messageA = receive_message(*server_socket);
 
    char* usernameServer = messageA[0];
    char* message = messageA[1];
    printf("%s : %s", usernameServer, message );
    if(strcmp(message, "send") == 0){
      char** FileUsername = receive_file(*server_socket);
      
      
      printf("%s", FileUsername[1]);


      free(FileUsername);
    }
    // ui_display(usernameClient, message);
    // free malloc'ed memory
    free(usernameServer);
    free(message);
    free(messageA);
  }

}


int main(int argc, char** argv) {
  // Make sure the arguments include a username
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
    exit(1);
    }

  // Save the username in a global
  username = argv[1];

  // ignore SIGPIPE because that's what happens with bad reads sometimes instead of errors
  signal(SIGPIPE, SIG_IGN);

  

  // Did the user specify a peer we should connect to?
  if (argc == 4) {
    // Unpack arguments
    char* peer_hostname = argv[2];
    unsigned short peer_port = atoi(argv[3]);

    // Connect to the specified peer in the chat network
    int socket_fd = socket_connect(peer_hostname, peer_port);
    if (socket_fd == -1) {
      perror("Failed to connect");
      exit(EXIT_FAILURE);
    }

    pthread_t threads[2];
    

    for (int i=0; i<2; i++){
      if(i == 1){
        if(pthread_create(&threads[i], NULL, send_message_thread, &socket_fd)){
          perror("failed to create thread for client");
          exit(EXIT_FAILURE);
        }
      }
      else{
        if(pthread_create(&threads[i], NULL, receive_message_thread, &socket_fd)){
          perror("failed to create thread for client");
          exit(EXIT_FAILURE);
        }

      }
    }
    
    
    for (int i = 0; i<2; i++){
      if(pthread_join(threads[i], NULL)){
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
      }
    }
    
    
  }

  return 0;
}
