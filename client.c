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

typedef struct thread_arg
{
  char *fileName;
  time_t *lastModified;
  int *server_socket;
} thread_arg_t;

typedef struct fnode
{
  char *fileName;
  time_t *lastModified;
  struct fnode *nextf;
} fnode_t;

fnode_t *Files = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

time_t seconds;
// Stores time seconds

// Keep the username for this machine in a global so we can access it from the callback
const char *username;
void *update_file_thread(void *args)
{
  printf("update_file_thread\n");
  while (true)
  {
    thread_arg_t *arg = (thread_arg_t *)args;
    char *fileName = arg->fileName;
    time_t *lastModified = arg->lastModified;
    int *server_socket = arg->server_socket;

    char *filePath = strcat("./", fileName);

    if (if_modified(filePath, *lastModified))
    {
      printf("File %s has been modified\n", fileName);
      fnode_t *temp = Files;
      while (temp != NULL)
      {
        if (strcmp(temp->fileName, fileName) == 0)
        {
          *temp->lastModified = time(&seconds);
          break;
        }
        temp = temp->nextf;
      }
      int rc = send_message(*server_socket, "update", username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      rc = sending_file(*server_socket, filePath, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      free(filePath);
      free(fileName);
      free(arg);
    }
  }
}

void *send_message_thread(void *args)
{
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

    //split command
    if (strcmp(new_message, "send") == 0)
    {
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }

      //create spaceeeee efor the filePath
      char *filePath = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);

      printf("Enter filepath please:");
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
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      printf("Your file has been successfully sent\n");
      free(filePath);
    }

    //request file from the server
    else if (strcmp(new_message, "receive") == 0)
    {
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
        perror("Failed to send filename to server");
        exit(EXIT_FAILURE);
      }
      printf("sent file name to server\n");
      
      sleep(5);
      
      
     
    }
    else if (strcmp(new_message, "quit") == 0)
    {
      int rc = send_message(*server_socket, new_message, username);
      if (rc == -1)
      {
        perror("Failed to send message to server");
        exit(EXIT_FAILURE);
      }
      exit(0);
      //
    }
    else
    {
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

void *receive_message_thread(void *args)
{
  // get the socket for the connection to receive messages from
  int *server_socket = (int *)args;
  while (1)
  {
    
    
    
    // Read a message from the server
    char **messageA = receive_message(*server_socket);
    char *usernameServer = messageA[0];
    char *message = messageA[1];
    printf("%s : %s\n", usernameServer, message);
    if (messageA == NULL || messageA[0] == NULL || messageA[1] == NULL)
    {
      //Failed to read message from server, so remove it from the linked list
      //do something TODO
      return NULL;
    }

    if(strcmp(messageA[1], "ready?") == 0){
      char **FileUsername = receive_file(*server_socket);
      // printf("%s\n", FileUsername[0]); 
      if (FileUsername == NULL || FileUsername[0] == NULL || FileUsername[1] == NULL)
      {
        printf("Failed to receive file\n");
        printf("%s\n", FileUsername ? "true" : "false");
        printf("%s filename\n", FileUsername[0]);
        printf("%s user\n", FileUsername[1]);
        //Failed to read message from server, so remove it from the linked list
        //do something TODO
        return NULL;
      }

      // printf("received file\n");
      fnode_t *new_file = (fnode_t *)malloc(sizeof(fnode_t));
      // new_file->fileName = FileUsername[0];
      // printf("%s\n", FileUsername[0]);
      new_file->fileName = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);
      strcpy(new_file->fileName, FileUsername[0]);
      // memcpy(new_file->fileName, FileUsername[0], strlen(FileUsername[0])-1);
      // printf("%s\n", new_file->fileName);
      time_t t = time(&seconds);
      new_file->lastModified = malloc(sizeof(time_t));
      memcpy(new_file->lastModified, &t, sizeof(time_t));
      // *new_file->lastModified = time(&seconds);

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


      pthread_t threads3;

      printf("Creating thread to update file\n");

      thread_arg_t *args = (thread_arg_t *)malloc(sizeof(thread_arg_t));
      // args->fileName = FileUsername[0];
      args->fileName = (char *)malloc(sizeof(char) * MAX_FILE_PATH_LENGTH);
      args->lastModified = malloc(sizeof(time_t));
      memcpy(args->fileName, FileUsername[0], strlen(FileUsername[0]));
      memcpy(args->lastModified, new_file->lastModified, sizeof(time_t));
      args->server_socket = server_socket;
      
      

      // printf("Created thread to update file moving on\n");

      // if (pthread_create(&threads3, NULL, update_file_thread, &args))
      // {
      //   perror("failed to create thread for client");
      //   exit(EXIT_FAILURE);
      // }
    }
    

    

    

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

    pthread_t threads[2];

    for (int i = 0; i < 2; i++)
    {
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
