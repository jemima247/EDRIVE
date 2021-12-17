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

// Nodes for a linked list of connections
typedef struct node
{
  int socket;
  pthread_t receive_thread;
  pthread_t send_thread;
  struct node *next;
} node_t;

const char *username = "Server";
// struct to be passed in (by reference) to the receive_message_thread function for threads
typedef struct server_thread_arg
{
  int server_socket_fd;
} server_thread_arg_t;

// lock for the linked list of connections
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// linked list of connections
node_t *sockets = NULL;

//struct for fileNames
typedef struct fnode
{
  char *fileName;
  int state;
  struct fnode *nextf;
} fnode_t;

//GLobal variable for File names
fnode_t *Files = NULL;

//is called to remove a node from the linked list
int remove_node(int client_remove)
{
  // first node_t pointer to be used to iterate through the linked list
  node_t *previous = sockets;

  // no node can be removed if the linked list is empty
  if (previous == NULL)
  {
    return -1;
  }

  // deal with the special case where the first node is to be removed
  if (previous->socket == client_remove)
  {
    sockets = previous->next;
    free(previous);
    return 0;
  }

  // a node_t pointer that will iterate through the linked list, one node ahead of previous
  node_t *current = sockets->next;

  // search the linked list for the specified socket
  while (current != NULL)
  {
    if (current->socket == client_remove)
    {
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

void *send_messages_thread(void* args){
   int *server_socket = (int *)args;
  while (1)
  {
    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (fgets(message, MAX_MESSAGE_LENGTH, stdin) == NULL)
    {
      perror("fgets\n");
      exit(EXIT_FAILURE);
    }
    char *new_message = strtok(message, "\n"); //removing trailing new line char
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      printf("senst\n");
      free(message);
  }
}

void *receive_file_path_thread(void *args)
{
  // get the socket for the connection to receive messages from
  int *client_socket = (int *)args;
  while (1)
  {
    // Read a message from the server
    char **messageA = receive_message(*client_socket);

    //check if we received message from the server
    if (messageA == NULL || messageA[0] == NULL || messageA[1] == NULL)
    {
      
      //lock linked list
      if (pthread_mutex_lock(&lock))
      {
        perror("Lock to loop through list failed");
        exit(EXIT_FAILURE);
      }

      //Failed to read message from server, so remove it from the linked list
      remove_node(*client_socket);

      //unlock lock
      if (pthread_mutex_unlock(&lock))
      {
        perror("Unlock to loop through list failed");
        exit(EXIT_FAILURE);
      }

      return NULL;
    }

    // Otherwise there is a message in messageA
    char *usernameClient = messageA[0];
    char *message = messageA[1];
    
    //clients wants to send something
    if (strcmp(message, "send") == 0)
    {
      printf("%s : '%s'\n", usernameClient, message);
      printf("Preparing to receive file\n");

      //receive the file and username from client
      char **FileUsername = receive_file(*client_socket);

      if (FileUsername == NULL || FileUsername[0] == NULL || FileUsername[1] == NULL)
      {
        // lock the linked list
        if (pthread_mutex_lock(&lock))
        {
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        //Failed to read message from server, so remove it from the linked list
        remove_node(*client_socket);

        //unlock the lock
        if (pthread_mutex_unlock(&lock))
        {
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        return NULL;
      }

      printf("received %s from %s\n", FileUsername[0], FileUsername[1]);

      //store client userName and file name
      usernameClient = FileUsername[1];
      char *fileName = FileUsername[0];

      //saving the fileName into linked list
      fnode_t *new_file = (fnode_t *)malloc(sizeof(fnode_t));
      new_file->fileName = fileName;

      //intiate fileState to NOT_EDITED
      new_file->state = NOT_EDITED;

      // acquire the lock before iterating through the linked list
      if (pthread_mutex_lock(&lock))
      {
        perror("Lock to loop through list failed");
        exit(EXIT_FAILURE);
      }

      //save new file in list of files in server
      new_file->nextf = Files;
      Files = new_file;

      // release the lock
      if (pthread_mutex_unlock(&lock))
      {
        perror("Unlock for linked list failed");
        exit(EXIT_FAILURE);
      }

      printf("%s has sent file %s\n", usernameClient, fileName);
    }

    //client want to receive a file
    else if (strcmp(message, "receive") == 0)
    {
      printf("%s : '%s'\n", usernameClient, message);
      
      char **messageFile = receive_message(*client_socket);
      if (messageFile == NULL || messageFile[0] == NULL || messageFile[1] == NULL)
      {
        
       //lock the linked list first
        if (pthread_mutex_lock(&lock))
        {
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }
         //Failed to read message from server, so remove it from the linked list
        remove_node(*client_socket); //-- Why are we removing the node oooh never mind

        //unlock the linked list
        if (pthread_mutex_unlock(&lock))
        {
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }
        return NULL;
      }

      // Otherwise there is a message in messageA, so display it
      usernameClient = messageFile[0];
      const char *fileName = messageFile[1];

      //lock the linked list 
      if (pthread_mutex_lock(&lock))
      {
        perror("Look to loop list");
        exit(EXIT_FAILURE);
      }
      
      //loop through list of files to confirm that file exist then send it by creating a temp node
      fnode_t *temp = Files;
      
      while (temp != NULL)
      {

        //check if fileName is existing in the list 
        if (strcmp(temp->fileName, fileName) == 0)
        {
          
          char beginingFilePath[] = "./";
          //now create the space for the filePath
          char *filePath = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);

          //copy beginingFilePath into filePath
          strcpy(filePath, beginingFilePath);

          //concatenate filePath and fileName
          strcat(filePath, fileName);
          
          //tell the client we are ready
          int rc = send_message(*client_socket, "ready?", username);
          
          if (rc == -1)
          {
            perror("failed to send message");
            exit(EXIT_FAILURE);
          }

          //send file to client
          int rcq = sending_file(*client_socket, filePath, username);
          if (rcq == -1)
          {
            perror("failed to send file message");
            exit(EXIT_FAILURE);
          }

          printf("%s has benn sent to %s\n", fileName, usernameClient);

          //free malloced space
          free(filePath);

          //unlock the linked list
          if (pthread_mutex_unlock(&lock))
          {
            perror("Look to loop list");
            exit(EXIT_FAILURE);
          }
          break;
        }
        //we haven't found the fileName
        else
        {
          temp = temp->nextf;
        }
      }

      //file is non existant
      if (temp == NULL)
      {
        int rc = send_message(*client_socket, "file was not found in server", "server");
        if (rc == -1)
        {
          perror("failed to send message");
          exit(EXIT_FAILURE);
        }
      }
      //free malloced space
      free(messageFile);
    }

    //client is updating file
    else if (strcmp(message, "update") == 0)
    {

      //receive file from the client
      char **FileUsername = receive_file(*client_socket);
      if (FileUsername == NULL || FileUsername[0] == NULL || FileUsername[1] == NULL)
      {
        
        //lock the linked list
        if (pthread_mutex_lock(&lock))
        {
          perror("Lock to loop through list failed");
          exit(EXIT_FAILURE);
        }

        //Failed to read message from server, so remove it from the linked list
        remove_node(*client_socket);

        //unlock the linked list
        if (pthread_mutex_unlock(&lock))
        {
          perror("Unlock to loop through list failed");
          exit(EXIT_FAILURE);
        }
        return NULL;
      }

      printf("update %s from %s\n", FileUsername[0], FileUsername[1]);

      //store usename for client and fileName
      usernameClient = FileUsername[1];
      char *fileName = FileUsername[0];

      //create a temp to iterate the file names linkeed list
      fnode_t *temp = Files;

      while (temp != NULL)
      {
        //found the fileName
        if (strcmp(temp->fileName, fileName) == 0)
        {
          //update fileState
          temp->state = EDITED;

          //tell client file successfuly updated
          int rc = send_message(*client_socket, "file has been updated", "server");
          if (rc == -1)
          {
            perror("failed to send error file message");
            exit(EXIT_FAILURE);
          }

          break;
        }
        else
        {
          //file not yet found
          temp = temp->nextf;
        }
      }

      //file is non-existant
      if (temp == NULL)
      {
        int rc = send_message(*client_socket, "file was not found in server, hence a new file was made", "server");
        if (rc == -1)
        {
          perror("failed to send message");
          exit(EXIT_FAILURE);
        }
      }

      //free malloced spaces
      free(FileUsername);
      free(fileName);
    }
    else
    {
      //non-valid command
      printf("%s : '%s'\n", usernameClient, message);
      printf("not a valid command\n");
    }

    //free malloced spaces
    free(usernameClient);
    free(message);
    free(messageA);
  }
}

void *server_thread(void *args)
{
  server_thread_arg_t *server_args = (server_thread_arg_t *)args;
  int server_socket_fd = server_args->server_socket_fd;
  while (1)
  {
    // Wait for a client to connect
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if (client_socket_fd == -1)
    {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }

    printf("someone connects\n");

    // create a node to add for the new client connection to the sockets linked list
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    assert(new_node != NULL);
    new_node->socket = client_socket_fd;

    // lock the lock associated with the linked list
    if (pthread_mutex_lock(&lock))
    {
      perror("pthread_mutex_lock failed");
      exit(EXIT_FAILURE);
    }

    //update the sockets with client connection
    new_node->next = sockets;
    sockets = new_node;

    // make a thread to receive messages from the new connection
    if (pthread_create(&new_node->receive_thread, NULL, receive_file_path_thread, &new_node->socket))
    {
      perror("failed to create thread for client");
      exit(EXIT_FAILURE);
    }

    // make a thread to receive messages from the new connection
    if (pthread_create(&new_node->send_thread, NULL, send_messages_thread, &new_node->socket))
    {
      perror("failed to create thread for client");
      exit(EXIT_FAILURE);
    }

    // unlock the lock associated with the linked list
    if (pthread_mutex_unlock(&lock))
    {
      perror("pthread_mutex_unlock failed");
      exit(EXIT_FAILURE);
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  
  // ignore SIGPIPE because that's what happens with bad reads sometimes instead of errors
  signal(SIGPIPE, SIG_IGN);

  // Set up a server socket to accept incoming connections
  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1)
  {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_socket_fd, 1))
  {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %u\n", port);

  // set up arguments for the thread_listen function
  pthread_t thread_listen;
  server_thread_arg_t arg;
  arg.server_socket_fd = server_socket_fd;

  // create the thread to listen for incoming client connection
  if (pthread_create(&thread_listen, NULL, server_thread, &arg))
  {
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }

  //join the thread
  if (pthread_join(thread_listen, NULL))
  {
    perror("pthread_join failed");
    exit(EXIT_FAILURE);
  }

  return 0;
}
