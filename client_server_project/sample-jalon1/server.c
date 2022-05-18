#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

//Structure du client
typedef struct client{
  int socket;
  struct sockaddr_in client_addr;//contient toutes les infos du client: port, fd et adresse
  struct client *next_client;
}client;

//Liste chaînée répertoriant tous les clients
struct all_clients
{
  client *start_client;
};


struct client *client_added(struct sockaddr_in client_addr, struct client *next_client,int client_sock){
  struct client *new = malloc(sizeof(*new));
  new->socket = client_sock;
  new->client_addr = client_addr;
  new->next_client = next_client;
  return new;
}



void echo_server(struct pollfd pfd, struct sockaddr_in client_addr, struct all_clients *clients){
    int client_sock = pfd.fd;
    char buf[MSG_LEN];
    memset(buf, '\0', MSG_LEN);
    printf("Start echo\n");
    // Receiving message from client
    if (recv(client_sock, buf, MSG_LEN, 0) <= 0) {
      return;
    }
    printf("Received: %s\n", buf);
    
    //Sending message back to client
    if (send(client_sock, buf, strlen(buf), 0) <= 0) {
      return;
    }
    printf("Message sent\n");
  }


void socket_closure(int server_sock, struct pollfd *pollfds, int fds_num){
  close(server_sock);
  for (int i = 0; i < fds_num; i++){
    close(pollfds[i].fd);
  }
}

// int accept_each_client(struct all_clients *clients, int socket, struct sockaddr *client_addr)
// {
//   int len_addr;
//   int sock_client;

//   len_addr = sizeof(client_addr);
//   sock_client = accept(socket, client_addr, (socklen_t *)&len_addr); //New socket for new client

//   if (sock_client < 0)
//   {
//     perror("Problem Accept");
//   }
//   printf("Accepting OK");
//   return sock_client;
// }


void connection(int port, struct all_clients *clients)
{
  fprintf(stdout,"IRC server on\n");

    int server_sock;
    int ret = 1;
    int client_sock;
    int fds_num = 1;
    int ret_poll;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct pollfd pollfds[SOMAXCONN];
    socklen_t size_addr = sizeof(struct sockaddr_in);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("Problem Socket");
      ret = 0;
    }
    else
    {
      fprintf(stdout,"Socket OK\n");
    }

    memset(pollfds, 0, SOMAXCONN * sizeof(struct pollfd)); 
    memset(&server_addr, 0, sizeof(server_addr));                
    memset(&client_addr, 0, sizeof(client_addr));                  
    //Server information
    server_addr.sin_family = AF_INET;//v4
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    //Init the poll tab
    pollfds[0].fd = server_sock;
    pollfds[0].events = POLLIN;
    //POLLIN: Data other than high priority data may be read without blocking


    clients->start_client = NULL;
    

    // bind to server addr
    printf("Binding...\n");
    if (bind(server_sock, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error while binding");
        ret = 0;
    }
    else
        fprintf(stdout,"Bind OK\n");


    // listen
    printf("Listening...\n");
    if (listen(server_sock, SOMAXCONN) == -1)
    {
        perror("Error listening...");
        ret = 0;
    }
    else
        fprintf(stdout,"Listening OK\n");

  
    while (ret)
    //while incoming connection
    //Poll method to accept clients
    //poll() examines a set of file descriptors to see if some of them are ready
    {
        ret_poll = poll(pollfds, fds_num, -1);
        // printf("%i\n", ret_poll);
        if (ret_poll < -1)
        {
          perror("Problem: poll not succeed");
          ret = 0;
        }
        int i;
        for (i = 0; i < fds_num; i++)
        { 
          if (pollfds[i].revents == POLLIN) 
          {
            //There's an activity
            if (pollfds[i].fd == server_sock)
            {
              //Socket listening for clients
              printf("Accepting\n");
              if ((client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &size_addr)) < 0) {
                perror("accept()\n");
                exit(EXIT_FAILURE);
              }
              // client_sock = accept_each_client(clients, server_sock, (struct sockaddr *)&client_addr);

              //Liste chainée Ajout//
              struct client *new = client_added(client_addr,clients->start_client,client_sock); //first client becomes next
              clients->start_client = new; // new client is first
              printf("Le numero de port du client est: %d\n",new->client_addr.sin_port);
              printf("L'adresse du client est: %s\n",inet_ntoa(new->client_addr.sin_addr));
              printf("Test : %d\n",client_sock);
              printf(" Numéro de Socket du nouveau client: %d\n",new->socket);

              pollfds[fds_num].fd = client_sock;
              pollfds[fds_num].events = POLLIN;
              fds_num++;
            }
          else
          {
              echo_server(pollfds[i], client_addr, clients); // Haven't put :echo_server(client_sock, client_addr, clients); -->connexion lost every time 
                                                          // -->connexion lost every time the client changes
          }
        }
      }
    }
    socket_closure(server_sock, pollfds,fds_num);
}


int main(int argc, char const *argv[])
{
  struct all_clients *clients = malloc(sizeof(struct all_clients));
  connection(atoi(argv[1]), clients); //port: first arg
  free(clients); //free malloc
  return EXIT_SUCCESS;
}