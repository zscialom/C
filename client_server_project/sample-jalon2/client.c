#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <xlocale.h>
#include <fcntl.h>

#include "msg_struct.h"
#include "common.h"
char salon_client[NICK_LEN];
char nick_reception_file[NICK_LEN];
char file_name[MSG_LEN];
int file_success = 0;
//A ete deplace sinon impossible de communiquer nick_name client lors d'un FILE_ACCEPT
char nickname_client[MSG_LEN];
char port_peer_to_peer[MSG_LEN];
char ip_peer_to_peer[MSG_LEN];


int connexion_peer_to_peer(const char *ip_addr, int port){
  int sock_client;
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr)); 

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port); 
  server_addr.sin_addr.s_addr = inet_addr(ip_addr);
  // Socket creation
  if ((sock_client = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Problem Socket"); 
    exit(EXIT_FAILURE);
  }
  else{
    fprintf(stdout,"%s and port: %i\n", ip_addr, port);
  }

  //Connecting to server
  if (connect(sock_client, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
    perror("Connection Problem");
    close(sock_client);
    exit(EXIT_FAILURE);
  }
  else{
    fprintf(stdout,"Connecting to server...done\n");
  }
  return sock_client;
}


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

struct message *initialisation(struct message *message, int pld_len, char *nick_sender, enum msg_type type, char *infos){
  //printf("DEBUG INITIALISATION\n");
  strcpy(message->infos, infos);
  //printf("%s\n",message->infos);
  strcpy(message->nick_sender, nick_sender);
  //printf("%s\n",message->nick_sender);
  message->pld_len = pld_len;
  //printf("%i\n",message->pld_len);
  strcpy(message->infos, infos);
  message->type = type;
  //printf("%u\n",message->type);
  return message;
}

int message_type(int sock, char *command){
  //Tableaux
  char *buf = malloc(MSG_LEN*sizeof(char));
  char payload_send[MSG_LEN];
  char nickname_receiver[NICK_LEN];
  //char nickname_client[MSG_LEN];
  char command_copy[MSG_LEN];
  char filename[MSG_LEN];
  //char salon_client[NICK_LEN];
  //Structures
  struct message *message = malloc(sizeof(struct message));
  strcpy(command_copy, command);
  //Initialisation
  memset(buf, 0, MSG_LEN*sizeof(char));
  memset(payload_send, 0, sizeof(payload_send));
  memset(nickname_receiver, 0, sizeof(nickname_receiver));
  //COMMANDE IMPORTANTE : The strtok() function is used to isolate sequential tokens in a null-terminated string, str. 
  strtok(command, " ");

  
  
  
  /* Gestion FILE_ACCEPT & FILE REJECT */
  if (file_success==1){
      if (strcmp(command, "Y\n")==0){
        //printf("DEBUG1");
        if (strcmp(nickname_client, "") != 0){
          //printf("DEBUG2");
          //printf("%s\n", nick_reception_file);
          //printf("%s\n", nickname_client);
          initialisation(message, 0, nickname_client, FILE_ACCEPT, nick_reception_file);
          //printf("DEBUG3");
          if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
            perror("Cannot send to server");
            free(message);
            return -1;
          }
        }
      }
      else{
        if (strcmp(nickname_client, "") != 0){
          initialisation(message, 0, nickname_client, FILE_REJECT, nick_reception_file);
          if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
            perror("Cannot send to server");
            free(message);
            return -1;
          }
        }
      }
    }



/* Gestion /nick */
  if (strcmp(command, "/nick")==0 && strcmp(salon_client,"")== 0){
    //printf("debug0\n");
    char *nickname = strtok(NULL, " ");
    strtok(nickname, "\n");
    // for (int i = 0; i < strlen(nickname); i++){
    //   if (strcmp(&nickname[i],"")==0){
    //     printf("You cannot use this nickname\n");
    //     printf("No spaces are allowed in your nickname\n");
    //     printf("Socket closed\n");
    //     free(message);
    //     return -1;
    //   }
    // }
    
    if (strcmp(command, "/nick\n")==0){
      printf("You cannot use this nickname\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else if (strcmp(nickname, "\n") == 0 || nickname == NULL){
      printf("You cannot use this nickname\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else if (strlen(nickname)>15){
      printf("You cannot use this nickname, it is too long\n");
      printf("It cannot exceed a length of 15\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else{
      //printf("%s\n",nickname);
      initialisation(message, 0, nickname_client, NICKNAME_NEW, nickname);
      strcpy(nickname_client, nickname);
      if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
        perror("Cannot send to the server");
        free(message);
        return -1;
      }    
    }
  }
    
/* Gestion /who */
  else if (strcmp(command, "/who\n") == 0 && strcmp(salon_client,"")== 0){
    //printf("debug1");
    if (strcmp(nickname_client, "") != 0){
      initialisation(message, 0, nickname_client, NICKNAME_LIST, "");
      if (send(sock, (const struct message *)message, sizeof(struct message), 0) <= 0){
        perror("Cannot send to server");
        free(message);
        return -1;
      }
    }
    else{
      perror("Nothing found");
      free(message);
      return -1;
    }
  }    
/* Gestion /whois */
  else if (strcmp(command, "/whois") == 0 && strcmp(salon_client,"")== 0){
    //printf("debug2");

    // Separate command and what is written
    char *nickname = strtok(NULL, " ");
    strtok(nickname, "\n");
    ///////////////////////////////////////
    if (strcmp(nickname_client, "") != 0){
      initialisation(message, 0, nickname_client, NICKNAME_INFOS, nickname);
      if (send(sock, (const struct message *)message, sizeof(struct message), 0) <= 0){
        perror("Cannot send to the server");
        free(message);
        return -1;
      }
    }
    // if (strcmp(command, "/nick\n")==0){
    //   printf("No clients connected found\n");
    //   free(message);
    //   return -1;
    // }
    if (strcmp(nickname, "\n") == 0 || nickname == NULL){
      printf("No clients connected entered\n");
      free(message);
      return 1;
    }
    // else{
    //   perror("No nickname input with the command");
    //   free(message);
    //   return -1;
    // }
  }  

/* Gestion /msgall */
  else if (strcmp(command, "/msgall") == 0 && strcmp(salon_client,"")== 0){
    //printf("debug3");
    if (strcmp(nickname_client, "") != 0){
      strcpy(payload_send, strtok(NULL, "\n"));
      strtok(payload_send, "\n");
      initialisation(message, strlen(payload_send), nickname_client, BROADCAST_SEND, "");
      //if (send(sock, (const char *)payload_send, strlen(payload_send), 0) <= 0){
        if (send(sock, (const struct message *)message, sizeof(struct message), 0) <= 0){
          perror("Cannot send Payload");
          free(message);
          return -1;
      }
      else{
        printf("First message to handle structure\n");
        printf("Broadcast Message sent to server.\n");
        printf("Broadcast message confirmed:\n");
        if (send(sock, payload_send, strlen(payload_send),0)<= 0){
         perror("Cannot send echo message");
         return -1;
        } 
        free(message);
        return 1;
      }
    }
    else{
      fprintf(stdout, "You have not signed up yet\n");
      free(message);
      return -1;
    }
  }  
  /* Gestion /msg */
  else if (strcmp(command,"/msg")==0 && strcmp(salon_client,"")== 0){
    //printf("debug4");
    strcpy(nickname_receiver, strtok(NULL, " "));
    strcpy(payload_send, strtok(NULL, "\n"));
    if (strcmp(nickname_client, "") != 0){
      initialisation(message, strlen(payload_send), nickname_client, UNICAST_SEND, nickname_receiver);
      //if (send(sock, (const char *)payload_send, strlen(payload_send), 0) <= 0){
      if (send(sock, (const struct message *)message, sizeof(struct message), 0) <= 0){
        perror("Cannot send Payload");
        free(message);
        return -1;
      }
      else{
        printf("First message to handle structure\n");
        printf("Unicast Message sent to server.\n");
        printf("Unicast message confirmed:\n");
        if (send(sock, payload_send, strlen(payload_send),0)<= 0){
         perror("Cannot send echo message");
         return -1;
        } 
        free(message);
        return 1;
      }
    }
    else{
      perror("Nothing found");
      free(message);
      return -1;
    }
  }
  /* Gestion MULTICAST_CREATE */
  else if (strcmp(command, "/create") == 0 && strcmp(salon_client,"")== 0){
    printf("Is it possible to create a new channel?\n");
    char *salon_name = strtok(NULL, " ");
    strtok(salon_name, "\n");
    if (strcmp(command, "/create\n")==0){
      printf("You cannot use this name for a channel\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else if (strcmp(salon_name, "\n") == 0 || salon_name == NULL){
      printf("You cannot use this name for a channel\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else if (strlen(salon_name)>15){
      printf("You cannot use this name for a channel, it is too long\n");
      printf("It cannot exceed a length of 15\n");
      printf("Socket closed\n");
      free(message);
      return -1;
    }
    else{
      if (strcmp(nickname_client, "") != 0){
        initialisation(message, 0, nickname_client, MULTICAST_CREATE, salon_name);
        strcpy(salon_client, salon_name);
        if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
          perror("Cannot send to the server");
          free(message);
          return -1;
        }    
      }
      else{
        perror("Nothing found");
        free(message);
        return -1;
      }
    }  
  }
  /* Gestion MULTICAST_LIST */
  else if (strcmp(command, "/channel_list\n") == 0){
    printf("Is the list of all the channels available?\n");
    if (strcmp(nickname_client, "") != 0){
      initialisation(message, 0, nickname_client, MULTICAST_LIST, "");
      if (send(sock, (const struct message *)message, sizeof(struct message), 0) <= 0){
        perror("Cannot send to server");
        free(message);
        return -1;
      }
    }
    else{
      perror("Nothing found");
      free(message);
      return -1;
    }
  }
  /* Gestion MULTICAST_JOIN */
  else if (strcmp(command, "/join") == 0){
    printf("Is it possible to join this new channel?\n");
    char *salon_name = strtok(NULL, " ");
    strtok(salon_name, "\n");

    if (strcmp(nickname_client, "") != 0) {                                     
      initialisation(message, 0, nickname_client, MULTICAST_JOIN, salon_name);
      strcpy(salon_client, salon_name);
      if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
        perror("Cannot send to the server");
        free(message);
        return -1;
      }
    }
    else{
      perror("Nothing found");
      free(message);
      return -1;
    }
  }
  /* Gestion MULTICAST_QUIT */
  else if (strcmp(command, "/quit") == 0 && strcmp(salon_client, "")!= 0){
    printf("Is it possible to quit the channel?\n");
    char *salon_name = strtok(NULL, " ");
    strtok(salon_name, "\n");
    if (strcmp(salon_name, salon_client) == 0){
      initialisation(message, 0, nickname_client, MULTICAST_QUIT, salon_client);
      strcpy(salon_client, "");
      if (send(sock, (const struct message *)message, sizeof(struct message), 0)<=0){
        perror("Cannot send to the server");
        free(message);
        return -1;
      }
    }
    else{
      printf("You are not in this channel, write the right channel's name\n");
    }
    
  }
  /* Gestion MULTICAST_SEND */
  else if (strcmp(salon_client, "") != 0){
    strcpy(buf, command_copy);
    initialisation(message, strlen(buf), nickname_client, MULTICAST_SEND, "");
     if (send(sock, (const struct message *)message, sizeof(struct message),0)<= 0){
         perror("Cannot send echo message");
         return -1;
     }
    if (send(sock, buf, strlen(buf),0)<= 0){
         perror("Cannot send echo message");
         return -1;
     }
    return 1;  
  }
  /* Gestion FILE_REQUEST */
  else if (strcmp(command, "/send") == 0 && strcmp(salon_client, "") == 0){
    strcpy(nickname_receiver, strtok(NULL, " "));
    strcpy(filename, strtok(NULL, "\n"));
    if (strcmp(nickname_client, "") != 0){
      initialisation(message, strlen(filename), nickname_client, FILE_REQUEST, nickname_receiver);
      if (send(sock, (const struct message *)message, sizeof(struct message),0)<= 0){
        perror("Cannot send the file");
        free(message);
        return -1;
      }
      else{
        printf("File request worked\n");
        if (send(sock, filename, strlen(filename),0)<= 0){
         perror("Cannot send echo message");
         return -1;
        } 
        free(message);
        return 1;
      }
    }
    else{
      printf("You have to register first\n");
      free(message);
      return -1;
    }
  }  
  /* Gestion /echo */
  else{
    if (file_success != 1){
      strcpy(buf, command_copy);
      //printf("%lu\n", strlen(buf));
      //printf("%s\n", buf);
      initialisation(message, strlen(buf), nickname_client, ECHO_SEND, "");
      //printf("%i\n",message->pld_len);
      if (send(sock, (const struct message *)message, sizeof(struct message),0)<= 0){
          perror("Cannot send echo message");
          return -1;
      }
      if (send(sock, buf, strlen(buf),0)<= 0){
          perror("Cannot send echo message");
          return -1;
      } 
      if (strcmp(command, "/quit\n") == 0){
        printf("Socket Closing, Quitting\n");
        return -1;
      }
      return 1;
    }
    file_success = 0;
  }
  free(message);
  return 1;
}



void echo_client(int sock){
    struct pollfd pfds[2];
    char buf[MSG_LEN];
    char buf_reception[MSG_LEN];
    //char salon_client[NICK_LEN];
    int ret_poll;
    memset(pfds, 0, sizeof(pfds));
    int success = 1;

    pfds[0].fd = sock;
    pfds[0].events = POLLIN;
    pfds[1].fd = fileno(stdin);
    pfds[1].events = POLLIN;

    while (success){
        memset(buf, 0, MSG_LEN);
        memset(buf_reception, 0, MSG_LEN);
        ret_poll = poll(pfds, 2, -1);

        if (ret_poll < 0){
            perror("Poll Problem\n");
            close(sock);
            break;
        }

        if (pfds[0].revents == POLLIN) {
        // Receiving message
            if (recv(sock, buf_reception, MSG_LEN, 0) <= 0) {
                printf("Nothing Received\n");
                break;
            }

            if (strstr(buf_reception, "Do you accept? [Y/N]\n")!=NULL){ //String in string permet de récuperer le nickname du sender
              printf("%s\n",buf_reception); 
              file_success = 1;
              //printf("%i\n",file_success);
              // printf("LE BUF QUE JE RECOIS\n");
              // printf("%s\n",buf_reception);
              strtok(buf_reception, " ");
              // printf("LE BUF QUE JE RECOIS APRES STRTOK\n");
              // printf("%s\n",buf_reception);
              strcpy(nick_reception_file,buf_reception);
              // printf("FInalite\n");
              //printf("%s\n", nick_reception_file);
            }
            else if (strcmp(buf_reception, "is the name of the file\n")==0)
            {
              strtok(buf_reception, " ");
              strcpy(file_name,buf_reception);
              //printf("%s\n",file_name);
            }
            else if (strstr(buf_reception, "the information to connect to send the file \n")!=NULL)
            {
              // GESTION DU FILE_SEND //
              printf("%s\n",buf_reception);
              strcpy(port_peer_to_peer,buf_reception);
              printf("Port obtained for connexion\n");
              strtok(port_peer_to_peer, ":");
              //strtok(port_peer_to_peer, ":");
              //strtok(buf_reception, ":");
              //strtok(buf_reception, ":");
              strcpy(ip_peer_to_peer,buf_reception);
              //printf("%s\n",ip_peer_to_peer);
              printf("%s\n",port_peer_to_peer);

              int	socket_p2p = connexion_peer_to_peer("127.0.0.1",atoi(port_peer_to_peer)); //je ne parviens pas a récupérer l'adresse
              if (socket_p2p == -1){
                printf("Connexion impossible\n");
              }
              int fd = open(file_name,O_RDONLY);

              if (fd == -1){
                printf("This file does not exit\n");
              }
              else{
                char message[MSG_LEN];
                int nb = 1;
                while( nb != 0){
                  memset(message,'\0',MSG_LEN);
                  nb = read(fd,message, MSG_LEN);
                  //initialisation(message, strlen(filename), nickname_client, FILE_REQUEST, file_name);
                  
                }
                close(fd);
              }
              close(socket_p2p);
            }

            else if (strcmp(buf_reception, "Please login with /nick <your pseudo>")==0){
              fprintf(stdout, "[Server] %s\n", buf_reception);
            }
            else if (strcmp(buf_reception, "This channel already exists\n")==0){
                fprintf(stdout, "%s\n", buf_reception);
                strcpy(salon_client, "");
            }
            else if (strcmp(buf_reception, "This Channel cannot be found")==0){
                fprintf(stdout, "[Server] This Channel cannot be found\n");
                strcpy(salon_client, "");
            }
            // else if (strcmp(buf_reception, "You must quit the right channel")==0){
            //     fprintf(stdout, "[Server] You must quit the right channel\n");
            //     strcpy(salon_client, salon_client);
            // }
            else{
              fprintf(stdout, "[Server] %s\n", buf_reception);  
              fprintf(stdout, "Type your request: \n");
            }
            success = quit(buf_reception);
        }

        if (pfds[1].revents == POLLIN) // --> fileno activity
        {
          read(fileno(stdin),buf,sizeof(buf));
          //printf("%s\n",buf);
          // int result = message_type(sock,buf);
          // printf("%i\n",result);
          if (message_type(sock,buf) == -1){
            close(sock);
            break;
          }
          
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
    fprintf(stdout,"Connecting to server...done\n");
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
