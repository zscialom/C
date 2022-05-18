#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"


int quit(char *buf){
	char end[] = "/quit";
	int n = 0;

	while (buf[n] != '\0'){
		n++;
	}
	n--;
	buf[n] = '\0';
	if (strcmp(buf,end) == 0) {
			printf("Close connection\n");
			return 0;
		}
	return 1;
}

void echo_client(int sock){
    printf("User input : \n"); // EN VUE DE LA CORRECTION: Ce message ne s'affiche qu'une fois au début, mais plusieurs messages peuvent être envoyés
    // N'hésitez pas à écrire les messages même s'il n'y est pas précisé User input
    struct pollfd pfds[2];
    char buf[MSG_LEN];
    char buf_reception[MSG_LEN];
    int ret_poll;
    memset(pfds, 0, sizeof(pfds));
    int success = 1;

    pfds[0].fd = sock;
    pfds[0].events = POLLIN;
    pfds[1].fd = fileno(stdin);
    pfds[1].events = POLLIN;

    while (success)
    {
        memset(buf, 0, MSG_LEN);
        memset(buf_reception, 0, MSG_LEN);
        ret_poll = poll(pfds, 2, -1);

        if (ret_poll < 0)
        {
            perror("Poll Problem\n");
            close(sock);
            break;
        }

        if (pfds[1].revents == POLLIN) // --> fileno activity
        {
        // Read input from user
        //     int ret = 0;
            
        //     if( -1 == (ret = read(0,(void *)buf, MSG_LEN))){
        //         perror("Error while reading from user");
        //         exit(EXIT_FAILURE);
        //     }
        //     printf("User input is : %s\n", buf);

        //     int ret_bis=0;
        // // Sending message to server
        //     if ( (ret_bis = write(sock, (void *)buf, strlen(buf))) == -1 ){
        //         perror("Error while writing");
        //         exit(EXIT_FAILURE);
        //     }
        // }

        if (pfds[0].revents == POLLIN) 
        {
        // Receiving message
            if (recv(sock, buf, MSG_LEN, 0) <= 0) {
                printf("Nothing Received\n");
                break;
            }
            printf("Received: %s", buf);
            success = quit(buf);
        }   
    }
}

int connexion_client(const char *ip_addr, int port)
{

  int sock_client;
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr)); 

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port); 
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);
  // Socket creation
  if ((sock_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Problem Socket"); 
    exit(EXIT_FAILURE);
  }
  else
  {
    fprintf(stdout,"%s and port: %i\n", ip_addr, port);
  }

  //Connecting to server
  if (connect(sock_client, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Connection Problem");
    close(sock_client);
    exit(EXIT_FAILURE);
  }
  else
  {
    fprintf(stdout,"Connection OK\n");
  }
  return sock_client;
}

int main(int argc, char const *argv[]){
  int socket;
  socket = connexion_client(argv[1], atoi(argv[2]));
  echo_client(socket);
  close(socket);
  return EXIT_SUCCESS;
}
