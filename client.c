#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "socket.h"
#include "message.h"
#include "file.h"

#include <assert.h>
#include <signal.h>

#define MAX_MESSAGE_LENGTH 2048
#define MAX_FILE_PATH_LENGTH 260

//struct to hold the file information for updating
typedef struct thread_arg
{
  char *fileName;
  time_t *lastModified;
  int *server_socket;
} thread_arg_t;

//the file struct so the client can have a list of the files theyhold
typedef struct fnode
{
  char *fileName;
  time_t *lastModified;
  struct fnode *nextf;
} fnode_t;

//the list of all files the client has access to
fnode_t *Files = NULL;

//lock for the file list
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

time_t seconds;
// Stores time seconds

// Keep the username for this machine in a global so we can access it from the callback
const char *username;

/**
 * This thread is always running for every file that the client has.
 * It will check if the file has been modified since the last time it checked.
 * If it has, it will send a message to the server to update the file.
 * @param arg - a pointer to the file name and last modified time
 * @return NULL
 * @see if_modified
 */

void *update_file_thread(void *args)
{

  //constantly running
  while (true)
  {
    thread_arg_t *arg = (thread_arg_t *)args;
    //get the file information
    char *fileName = arg->fileName;
    time_t *lastModified = arg->lastModified;
    int *server_socket = arg->server_socket;

    //construct the file path to open it
    char beginingFilePath[] = "./";
    

    //now create the space for the filePath
    char *filePath = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);

    strcpy(filePath, beginingFilePath);
    strcat(filePath, fileName);

    //check if the file was modified
    if (if_modified(filePath, *lastModified))
    {
      //let client and server know that the file has been modified
      printf("File %s has been modified\n", fileName);
      fnode_t *temp = Files;
      while (temp != NULL)
      {
        //find the file in the list and update its time
        if (strcmp(temp->fileName, fileName) == 0)
        {
          *temp->lastModified = time(&seconds);
          break;
        }
        temp = temp->nextf;
      }
      //send the message to the server
      int rc = send_message(*server_socket, "update", username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      //send the updated file back to the server
      rc = sending_file(*server_socket, filePath, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      //update the file information for the time
      time_t t = time(&seconds);
      // arg->lastModified = &t;
      memcpy(arg->lastModified, &t, sizeof(time_t));
    }
    //free the file path
    free(filePath);
    
    
  }
}

/**
 * This function/thread is always running to receive commands 
 * from the client and and send to the server.
 * so the server can perform based on the command.
 * @param arg - a pointer to the server socket
 * @return NULL
 */
void *send_message_thread(void *args)
{
  //server socket
  int *server_socket = (int *)args;
  while (1)
  {
    //allocate space for the commands
    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (fgets(message, MAX_MESSAGE_LENGTH, stdin) == NULL)
    {
      perror("fgets\n");
      exit(EXIT_FAILURE);
    }
    char *new_message = strtok(message, "\n"); //removing trailing new line char

    //split command
    if (strcmp(new_message, "send") == 0)
    {
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      //create space for the filePath
      char *filePath = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);

      //ask for the file path
      printf("Enter filepath please: \n");
      //get the filePath
      if (fgets(filePath, MAX_FILE_PATH_LENGTH, stdin) == NULL)
      {
        perror("fgets for filepath\n");
        exit(EXIT_FAILURE);
      }

      char *new_filePath = strtok(filePath, "\n"); //removing trailing new line char

      //send the file to the server
      rc = sending_file(*server_socket, new_filePath, username);
      if (rc == -1)
      {
        //error checking
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      printf("Your file has been successfully sent\n");
      free(filePath);
    }

    //request file from the server
    else if (strcmp(new_message, "receive") == 0)
    {
      //send the command to the server
      int rc = send_message(*server_socket, new_message, username);
      //send the recieve command to the server
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      //create space for the filename
      char *filename = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
      printf("Enter file name please:\n");

      //get the file name that the client wants to access then send taht to the server
      if (fgets(filename, MAX_MESSAGE_LENGTH, stdin) == NULL)
      {
        perror("fgets for file name\n");
        exit(EXIT_FAILURE);
      }
      filename = strtok(filename, "\n"); //removing trailing new line char

      //send filename to server
      rc = send_message(*server_socket, filename, username);
      if (rc == -1)
      {
        //error checking
        perror("Failed to send filename to server");
        exit(EXIT_FAILURE);
      }
      printf("sent file name to server\n");
      
      sleep(5);
      //receive the file from the server
    }
    //quit connection
    else if (strcmp(new_message, "quit") == 0)
    {
      //send the message to the server
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      while (Files != NULL)
      {
        //free and clear all files client holds
        fnode_t *temp = Files;
        Files = Files->nextf;
        
        int r = remove(temp->fileName);
        free(temp->fileName);
        free(temp);
      }
      //close the socket connection
      close(*server_socket);
      exit(EXIT_SUCCESS);
      //exit the program
    }
    else
    {
      //sned random message to server that is not a command
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
    }

    free(message);
  }
}

/**
 * This function/thread is always running to receive command from the server
 * and then gets ready to receive the file from the server.
 * @param arg - a pointer to the server socket
 * @return NULL
 */
void *receive_message_thread(void *args)
{
  // get the socket for the connection to receive messages from
  int *server_socket = (int *)args;
  while (1)
  {
    // Read a message from the server
    char **messageA = receive_message(*server_socket);
    //error check the meesage
    char *usernameServer = messageA[0];
    char *message = messageA[1];
    //show what the srever sent
    printf("%s : %s\n", usernameServer, message);
    if (messageA == NULL || messageA[0] == NULL || messageA[1] == NULL)
    {
      //Failed to read message from server, so remove it from the linked list
      //do something TODO
      return NULL;
    }

    //if its the right command exppect file
    if(strcmp(messageA[1], "ready?") == 0){
      char **FileUsername = receive_file(*server_socket);
      //error check the file
      if (FileUsername == NULL || FileUsername[0] == NULL || FileUsername[1] == NULL)
      {
        perror("Failed to receive file");
        //Failed to read message from server, so remove it from the linked list
        //do something TODO
        return NULL;
      }

      
      fnode_t *new_file = (fnode_t *)malloc(sizeof(fnode_t));
      //save new file to linked list
      new_file->fileName = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);
      strcpy(new_file->fileName, FileUsername[0]);
      //save time of arrival
      time_t t = time(&seconds);
      new_file->lastModified = malloc(sizeof(time_t));
      memcpy(new_file->lastModified, &t, sizeof(time_t));
      
      //add to linked list

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
      printf("File %s has been received from %s\n", FileUsername[0], FileUsername[1]);

      //create thread for file to constantly check for modifications
      pthread_t threads3;

      printf("Creating thread to update file\n");

      thread_arg_t *args = (thread_arg_t *)malloc(sizeof(thread_arg_t));
      
      args->fileName = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);
      args->lastModified = malloc(sizeof(time_t));
      strcpy(args->fileName, FileUsername[0]);
      //copy all necessary data to thread
      memcpy(args->lastModified, new_file->lastModified, sizeof(time_t));
      args->server_socket = server_socket;
      
      printf("Created thread to update file moving on\n");
      //create thread to constantly check for modifications

      if (pthread_create(&threads3, NULL, update_file_thread, args))
      {
        perror("failed to create thread for client");
        exit(EXIT_FAILURE);
      }
    }
    //free the message
    free(usernameServer);
    free(message);
    free(messageA);
  }
}


int main(int argc, char **argv)
{
  // Make sure the arguments include a username
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
    exit(1);
  }

  // Save the username in a global
  username = argv[1];

  // ignore SIGPIPE because that's what happens with bad reads sometimes instead of errors
  signal(SIGPIPE, SIG_IGN);

  // Did the user specify a peer we should connect to?
  if (argc == 4)
  {
    // Unpack arguments
    char *peer_hostname = argv[2];
    unsigned short peer_port = atoi(argv[3]);

    // Connect to the specified peer in the chat network
    int socket_fd = socket_connect(peer_hostname, peer_port);
    if (socket_fd == -1)
    {
      perror("Failed to connect");
      exit(EXIT_FAILURE);
    }
    //create thread to receive messages and send messages to and from server
    pthread_t threads[2];

    for (int i = 0; i < 2; i++)
    {
      //create both threads with arguments as the socket
      if (i == 1)
      {
        if (pthread_create(&threads[i], NULL, send_message_thread, &socket_fd))
        {
          perror("failed to create thread for client");
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        if (pthread_create(&threads[i], NULL, receive_message_thread, &socket_fd))
        {
          perror("failed to create thread for client");
          exit(EXIT_FAILURE);
        }
      }
    }


    //wait for threads to finish
    for (int i = 0; i < 2; i++)
    {
      if (pthread_join(threads[0], NULL))
      {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
      }
    }
  }

  return 0;
}
