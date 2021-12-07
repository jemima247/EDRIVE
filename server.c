#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "socket.h"
#include "file.h"
#include "message.h"

#include <assert.h>
#include <signal.h>

#define MAX_MESSAGE_LENGTH 2048
#define EDITED 1
#define NOT_EDITED 0


// Keep the username for this machine in a global so we can access it from the callback

//Notice that everytime we change the machine or relogin, the Files are going to be empty
//SO we need a function such that every time the server is started up we have that the Files 
// variable is updated with the namees of Files otherwise it will always be null

//We also have to figure out how to send larger files in time

// Nodes for a linked list of connections
typedef struct node{
  int socket;
  pthread_t receive_thread;
  struct node* next;
} node_t;

const char* username = "Server";
// struct to be passed in (by reference) to the receive_message_thread function for threads
typedef struct server_thread_arg{
  int server_socket_fd;
} server_thread_arg_t;

// lock for the linked list of connections
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// linkd list of connections
node_t* sockets = NULL;



typedef struct fnode{
    char* fileName;
    int state;
    struct fnode* nextf;
} fnode_t;

fnode_t* Files = NULL;


int remove_node(int client_remove){
  // first node_t pointer to be used to iterate through the linked list
  node_t* previous = sockets;

  // no node can be removed if the linked list is empty
  if(previous == NULL){
    return -1;
  }

  // deal with the special case where the first node is to be removed
  if(previous->socket == client_remove){
    sockets = previous->next;
    free(previous);
    return 0;
  }

  // a node_t pointer that will iterate through the linked list, one node ahead of previous
  node_t* current = sockets->next;

  // search the linked list for the specified socket
  while(current != NULL){
    if(current->socket == client_remove){
      previous->next = current->next;
      free(current);
     return 0;
    }
    previous = current;
    current = previous->next;
  }
  // if the node with the specified socket was not found, return a value indicating that
  return -1;
} 



void* receive_file_path_thread(void* args){
  // get the socket for the connection to receive messages from
  int* client_socket = (int*)args;
  while(1){
    // printf("waiting for message\n");
    // Read a message from the server
    char** messageA = receive_message(*client_socket);
    if (messageA == NULL || messageA[0] == NULL  ||messageA[1] == NULL ) {
      //Failed to read message from server, so remove it from the linked list
      if(pthread_mutex_lock(&lock)){
        perror("Lock to loop through list failed");
        exit(EXIT_FAILURE);
      }

      remove_node(*client_socket);

      if(pthread_mutex_unlock(&lock)){
        perror("Unlock to loop through list failed");
        exit(EXIT_FAILURE);
      }
      return NULL;     
    }
    // Otherwise there is a message in messageA, so display it    
    char* usernameClient = messageA[0]; 
    char* message = messageA[1];
    
    printf("%s : '%s'\n", usernameClient, message);
    if(strcmp(message, "send") == 0){
      printf("Preparing to receive file\n");
      char** FileUsername = receive_file(*client_socket);
      if (FileUsername == NULL || FileUsername[0] == NULL  ||FileUsername[1] == NULL ) {
      //Failed to read message from server, so remove it from the linked list
        if(pthread_mutex_lock(&lock)){
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        remove_node(*client_socket);

        if(pthread_mutex_unlock(&lock)){
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }
        return NULL;     
      }

      printf("received %s from %s\n", FileUsername[0], FileUsername[1]);
      usernameClient= FileUsername[1];
      char* fileName = FileUsername[0];
      fnode_t* new_file = (fnode_t*) malloc(sizeof(fnode_t));
      new_file->fileName = fileName;
      new_file->state = NOT_EDITED;

        // acquire the lock before iterating through the linked list
      if(pthread_mutex_lock(&lock)){
        perror("Lock to loop through list failed");
        exit(EXIT_FAILURE);
      }

      //save new file in list of files in server
      
      new_file->nextf = Files;
      Files = new_file;

      // release the lock
      if(pthread_mutex_unlock(&lock)){
        perror("Unlock for linked list failed");
        exit(EXIT_FAILURE);
      }
   
      // char* file = FileU[1];
      printf("%s has sent file %s\n", usernameClient, fileName);
      

      // free(FileUsername);
      // free(fileName);
    }
    else if(strcmp(message, "receive") == 0){
      printf("we got the command\n");
      char** messageFile = receive_message(*client_socket);
      if (messageFile == NULL || messageFile[0] == NULL  ||messageFile[1] == NULL ) {
        printf("weird\n");
        //Failed to read message from server, so remove it from the linked list
        if(pthread_mutex_lock(&lock)){
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        remove_node(*client_socket); //-- Why are we removing the node oooh never mind

        if(pthread_mutex_unlock(&lock)){
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }
        return NULL;     
      }
      // Otherwise there is a message in messageA, so display it    
      usernameClient = messageFile[0];  
      const char* fileName = messageFile[1];


      //loop through list of files to confirm that file exist then send it
      if(pthread_mutex_lock(&lock)){
        perror("Look to loop list");
        exit(EXIT_FAILURE);
      }
      printf("%s\n", usernameClient);
      
      fnode_t* temp = Files;
      printf("%s\n", fileName);
      // printf("the temp val, %s\n", temp->fileName);//was giving the segfault to fix tomorrow
      while (temp != NULL){
        printf("in\n");
        printf("tempL: %s\n", temp->fileName);
        printf("%s filename\n",fileName);
        if (strcmp(temp->fileName,fileName)==0){
          printf("WTF!!!\n");
          //use function to send to client
          char beginingFilePath[3] = "./";
          // strcpy(beginingFilePath, "./");

          //get length of the requessted fileName
          
          printf("213\n");
          //now create the space for the filePath
          char* filePath = (char*) malloc(sizeof(char)* MAX_FILE_PATH_LENGTH);
          strcpy(filePath, beginingFilePath);
          printf("217\n");
          strcat(filePath, fileName);
          printf("TF\n");
          printf("%s\n", filePath);
          int rc = sending_file(*client_socket, filePath, username);
          

          if (rc == -1) {
            //not sure what to do yet
            perror("failed to send error file message");
            exit(EXIT_FAILURE);
          }
          printf("%s has benn sent to %s\n", fileName, usernameClient);
          free(filePath);

          break;
        }
        else{
          temp = temp->nextf;
        }
      }

      if(temp == NULL){
        int rc = send_message(*client_socket, "file was not found in server", "server");
        if (rc == -1) {
          //not sure what to do yet
          perror("failed to send error file message");
          exit(EXIT_FAILURE);
        }
      }
      free(messageFile);
      
      


    }
    else if(strcmp(message, "update") == 0){
      char** FileUsername = receive_file(*client_socket);
      if (FileUsername == NULL || FileUsername[0] == NULL  ||FileUsername[1] == NULL ) {
        //Failed to read message from server, so remove it from the linked list
        if(pthread_mutex_lock(&lock)){
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        remove_node(*client_socket);

        if(pthread_mutex_unlock(&lock)){
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }
        return NULL;     
      }



      printf("update %s from %s\n", FileUsername[0], FileUsername[1]);
      usernameClient= FileUsername[1];
      char* fileName = FileUsername[0];

      fnode_t* temp = Files;

      while (temp != NULL){
        if (temp->fileName == fileName){
          //use function to send to client
          temp->state = EDITED;
          int rc = send_message(*client_socket, "file has been updated", "server");
          if (rc == -1) {
            //not sure what to do yet
            perror("failed to send error file message");
            exit(EXIT_FAILURE);
          }
          
          break;
        }
        else{
          temp = temp->nextf;
        }
      }

      if(temp == NULL){
        int rc = send_message(*client_socket, "file was not found in server, hence a new file was made", "server");
        if (rc == -1) {
          //not sure what to do yet
          perror("failed to send error file message");
          exit(EXIT_FAILURE);
        }
      }
      
      

      free(FileUsername);
      free(fileName);
    }
    else{
      printf("%s : '%s'\n", usernameClient, message);
      printf("not a valid command\n");
    }

    free(usernameClient);
    free(message);
    free(messageA);
  }

}




void* server_thread(void* args){
  server_thread_arg_t* server_args = (server_thread_arg_t*) args;
  int server_socket_fd = server_args->server_socket_fd;
  while(1){
    printf("hi\n");
    // Wait for a client to connect
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if (client_socket_fd == -1) {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }

    printf("someone connects\n");
    // add a node for the new connection to the linked list
    node_t* new_node = (node_t*) malloc(sizeof(node_t));
    assert(new_node != NULL);
    new_node->socket = client_socket_fd;
    // lock the lock associated with the linked list
    if(pthread_mutex_lock(&lock)){
      perror("pthread_mutex_lock failed");
      exit(EXIT_FAILURE);
    }
    new_node->next = sockets;
    sockets = new_node;

    // make a thread to receive messages from the new connection
    if(pthread_create(&new_node->receive_thread, NULL, receive_file_path_thread, &new_node->socket)){
      perror("failed to create thread for client");
      exit(EXIT_FAILURE);
    }
    // unlock the lock associated with the linked list
    if(pthread_mutex_unlock(&lock)){
      perror("pthread_mutex_unlock failed");
      exit(EXIT_FAILURE);
    }
  }
}



int main(int argc, char** argv) {
  // // Make sure the arguments include a username
  // if (argc != 2 ) {
  //   fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
  //   exit(1);
  // }

  // Save the username in a global
  // username = argv[1];

  // ignore SIGPIPE because that's what happens with bad reads sometimes instead of errors
  signal(SIGPIPE, SIG_IGN);

  // Set up a server socket to accept incoming connections
  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }
  // char portNum[30+sizeof(unsigned short)]; 
  // printf(portNum, "Server listening on port %u\n", port);

  printf("Server listening on port %u\n", port);


  // start a thread that keeps accepting clients
  // set up arguments for the thread's function
  pthread_t thread_listen;
  server_thread_arg_t arg;
  arg.server_socket_fd = server_socket_fd;
  
  // create the thread
  if(pthread_create(&thread_listen, NULL, server_thread, &arg)){
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }

  // Did the user specify a peer we should connect to?
  

  // Set up the user interface. The input_callback function will be called
  // each time the user hits enter to send a message.
  // ui_init(input_callback);

  // // Once the UI is running, you can use it to display log messages
  // ui_display("INFO", "This is a handy log message.");
  // ui_display(username, portNum);

  // // Run the UI loop. This function only returns once we call ui_stop() somewhere in the program.
  // ui_run();

  if(pthread_join(thread_listen, NULL)){
    perror("pthread_join failed");
    exit(EXIT_FAILURE);
  }

  return 0;
}
