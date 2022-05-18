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
#include <time.h>

#include "msg_struct.h"
#include "common.h"

//Structure du client
typedef struct client{
  int socket;
  //char nickname[NICK_LEN]; //pseudo
  char *nickname;
  struct sockaddr_in client_addr;//contient toutes les infos du client: port, fd et adresse
  struct client *next_client;
  char time_registration[MSG_LEN];
  char salon[MSG_LEN]; //Way to know in which channel is the client
  char file_name[MSG_LEN];
}client;

//Liste chaînée répertoriant tous les clients
struct all_clients
{
  client *start_client;
};

//Liste chaînée des salons de discussion
struct salon
{
  struct client *connected[128];
  struct salon *next_salon;
  char salon_nickname[MSG_LEN];
  int number; //Number of clients
};

struct all_salons
{
  struct salon *start_salon;
};

void get_time_connection(struct client *client){
  time_t *clock = malloc(sizeof(time_t));
  *clock = time(NULL);
  struct tm *tm = localtime(clock);
  char *time_start_connexion = asctime(tm);
  strcpy(client->time_registration, time_start_connexion);
}

struct client *client_added(struct sockaddr_in client_addr, struct client *next_client,int client_sock, char *nickname){
  char *nickname_bis = malloc(NICK_LEN*sizeof(char));
  strcpy(nickname_bis, nickname);
  struct client *new = malloc(sizeof(*new));
  get_time_connection(new);
  new->socket = client_sock;
  new->client_addr = client_addr;
  new->next_client = next_client;
  new->nickname = nickname_bis;
  return new;
}

void client_deleted(struct all_clients *clients, struct client *client){
  struct client *client_done = clients->start_client;
  while (client_done != NULL){
    if (client_done == client){
      //First client is now the second on the list
      clients->start_client = client->next_client;
      break;
    }
    if (client_done->next_client == client){
      client_done->next_client = client->next_client;
      break;
    }
    client_done = client_done->next_client;
  }
}

int nickname_already_exists(char *nickname, struct all_clients *clients){
  struct client *client = clients->start_client;
  while (client != NULL){
    if (strcmp(client->nickname, nickname) == 0){
      return 1;
    }
    client = client->next_client;
  }
  return 0;
}

struct client *get_client(int sock, struct all_clients *clients){
  struct client *client = clients->start_client;
  while (client != NULL){ //On parcourt la liste chaînée
    if (client->socket == sock){// Jusqu'à obtenir la socket voulue
      return client;
    }
    client = client->next_client;
  }
  return NULL;
}

char *get_all_clients(struct all_clients *clients){
  char *list = malloc(MSG_LEN * 128);//List returned
  char client_enum[MSG_LEN];
  struct client *client = clients->start_client;
  memset(client_enum, 0, sizeof(client_enum));
  strcpy(list, "Here is the list of all the clients: \n");
  while (client != NULL){
    sprintf(client_enum, "%s\n", client->nickname);
    strcat(list, client_enum);
    client = client->next_client;
  }
  return list;
}

char *get_client_infos(struct all_clients *clients, char *nickname){
  struct client *client = clients->start_client;
  //struct sockaddr_in client_addr = client->client_addr;

  char *buf = malloc(MSG_LEN * 128);
  char ip_adress[128];
  memset(ip_adress, 0, sizeof(ip_adress));

  while (client != NULL){
    if (strcmp(client->nickname, nickname) == 0){
      inet_ntop(AF_INET, &(client->client_addr.sin_addr), ip_adress, INET_ADDRSTRLEN);
      sprintf(buf, "%s connected, at %s, IP Adress %s & port number %i", client->nickname,client->time_registration ,ip_adress, client->client_addr.sin_port);
      return buf;
    }
    client = client->next_client;
  }
  return NULL;
}

struct client *get_nickname(char *surname,struct all_clients *clients){
  struct client *client = clients->start_client;
  while (client != NULL){ //Parcourt la liste
    if (strcmp(client->nickname, surname)==0){
      return client;
    }
    client = client->next_client;
  }
  return NULL;
}

int broadcast_message(char *payload, struct all_clients *clients, struct client *client){
  //printf("Broadcast Debug");
  char payload_send[MSG_LEN * 4];
  char *broadcast_text = "Broadcast message sent";
  struct client *user = clients->start_client;
  sprintf(payload_send, "[%s] : %s", client->nickname, payload);
  while (user != NULL){
    //send it to all the other clients connected
    if (user != client){
      if (send(user->socket, payload_send, strlen(payload_send), 0)<=0){
        perror("Cannot send server");
        return -1;
      }
    }
    else{ 
      //Send it back to user to confirm him everything is OK
      if (send(user->socket, broadcast_text, strlen(broadcast_text), 0)<=0){
        perror("Cannot send broadcast message");
        return -1;
      }
    }
    user=user->next_client;
  }
  return 1;
}

void initialisation_salon(struct salon *salon, struct salon *next_salon, struct client *client, char *salon_nickname){
  memset(salon->connected, 0, sizeof(salon->connected));
  strcpy(salon->salon_nickname, salon_nickname);
  salon->connected[0] = client;
  salon->number = 1;
  salon->next_salon = next_salon;
}

void salon_destroyed(struct all_salons *salons, struct salon *salon_destroyed){
  struct salon *salon = salons->start_salon;
  while (salon != NULL){
    if (salon == salon_destroyed){
      salons->start_salon = salon_destroyed->next_salon;
      break;
    }
    if (salon->next_salon == salon_destroyed){
      salon->next_salon = salon_destroyed->next_salon;
      break;
    }
    //implémentation pour parcourir toute la liste
    salon = salon->next_salon;
  }
}

int salon_already_exists(char *salon_name, struct all_salons *salons){
  struct salon *start_salon = salons->start_salon;
  while (start_salon != NULL){
    if (strcmp(start_salon->salon_nickname, salon_name) == 0){
      return -1;
    }
    start_salon = start_salon->next_salon;
  }
  return 1;
}

char *get_all_salons(struct all_salons *salons){
  struct salon *salon = salons->start_salon;
  char *buf = malloc(MSG_LEN*128);
  char new[2*MSG_LEN];
  strcpy(buf, "Here is the names of all the channels:\n");

  while (salon != NULL){ //on parcout toute la liste chaînée
    sprintf(new, "%s\n", salon->salon_nickname);
    strcat(buf, new);
    salon = salon->next_salon;
  }
  return buf;
}

void client_added_salon(struct salon *salon, struct client *client){
  for (int i = 0; i < 128; i++){
    if (salon->connected[i] == NULL){
      salon->connected[i] = client;
      strcpy(client->salon, salon->salon_nickname);
      salon->number++;
      break;
    }
  }
}

struct salon *salon_nickname(struct all_salons *salons, char *nickname){
  struct salon *salon_request = salons->start_salon;
  while (salon_request != NULL){
    if (strcmp(salon_request->salon_nickname, nickname)==0){
      return salon_request;
    }
    salon_request = salon_request->next_salon;
  }
  return NULL;
}

int client_deleted_salon(struct salon *salon, struct client *client){
  strcpy(client->salon, "");
  for (int i = 0; i < 128; i++){
    if (salon->connected[i] != NULL){
      //on parcourt la liste des clients connectés 
      if (strcmp(salon->connected[i]->nickname, client->nickname) == 0){
        //si dans le salon le client apparait dans les noms des clients connectés
        salon->connected[i] = NULL;
        salon->number--;
        return 1;
      }
    }
  }
  return -1;
}

void send_salon_multicast(struct salon *salon, char *message, struct all_clients *clients, struct client *client_sender){
  char multicast[] = "Multicast message sent in the channel";
  for (int i = 0; i < 128; i++){
    if (salon->connected[i] != NULL){
      struct client *client = get_nickname(salon->connected[i]->nickname, clients);
      if (strcmp(client->nickname, client_sender->nickname) != 0){
        //Si client connecté dans le channel n'est pas le sender
        if (send(client->socket, (const char *)message, strlen(message), 0)<=0){
          perror("Cannot send the message");
        }
      }
      else{
        if (send(client->socket, (const char *)multicast, strlen(multicast), 0) <= 0)
        {
          perror("Cannot send the message");
        }
      }
    }
  }
}


void echo_server(struct pollfd pfd, struct sockaddr_in client_addr, struct all_clients *clients, struct all_salons *salons){
    int client_sock = pfd.fd;
    char buf[MSG_LEN];//////
    //char *buf_bis = malloc(MSG_LEN*sizeof(char));
    char buf_not_payload[MSG_LEN];
    char payload[MSG_LEN];
    char buf_unicast[MSG_LEN];
    char buf_infos[MSG_LEN];
    memset(buf, '\0', MSG_LEN);
    memset(buf_not_payload, '\0', MSG_LEN);
    memset(payload, '\0', MSG_LEN);
    struct message *message = malloc(sizeof(struct message)); //utilisation structure sujet

    //printf("Start echo1\n");
    // Receiving message from client
    if (recv(client_sock, message, sizeof(struct message), 0)<0) {
      perror("Error while receiving the message");
    }
    else{
      //printf("Start echo2\n");
      /* Gestion Message type NICKNAME_NEW */
      if (strcmp(msg_type_str[message->type], "NICKNAME_NEW") == 0){
        //printf("Start echo3\n");
        if (strcmp(message->infos, "") != 0){
          //printf("Start echo4\n");
          if (nickname_already_exists(message->infos,clients) == 1){ // Nickname chosen is already taken --> 1
            char *nickname_exist = "You cannot use this nickname, It is already taken";
            if (send(client_sock, nickname_exist, strlen(nickname_exist), 0)<0){
              perror("Cannot send the nickname");
            }
            // else{
            //   printf("Client Socket closed because the nickname already exists\n");
            //   close(client_sock);
            // }
          }

          else{ 
            //Create a new client

            //printf("Start echo13\n");
            struct client *new_client = client_added(client_addr,clients->start_client,client_sock,message->infos);
            //printf("Start echo14\n");
            clients->start_client = new_client;
            //printf("Start echo15\n");
            sprintf(buf_not_payload, "Welcome on the chat %s\n", new_client->nickname);
            //printf("Start echo16\n");
            if (send(client_sock, (const char *)buf_not_payload, strlen(buf_not_payload), 0)<=0){
              perror("Cannot respond to the client");
            }
          //Changing the nickname
            if (strcmp(message->nick_sender, "") != 0){
              //printf("Start echo8\n");
              struct client *client = get_client(client_sock, clients);
              //printf("%lu | %s\n", strlen(message->infos), message->infos);
              if (strcmp(client->nickname, message->nick_sender)){
                //printf("Start echo9\n");
                //printf("%lu | %s\n", strlen(message->infos), message->infos);
                strcpy(client->nickname, message->infos);//Changing the nickname//
                //printf("Start echo10\n");
                sprintf(buf_not_payload, "Your new nickname is %s", client->nickname);
                //printf("Start echo11\n");
                if (send(client_sock, (const char *)buf_not_payload, strlen(buf_not_payload), 0)<0){
                  //printf("Start echo12\n");
                  perror("Cannot respond to the client");
                }
              }
            }
          }
          
        }
      }  
      /* Gestion Message Type NICKNAME_LIST */
      else if(strcmp(msg_type_str[message->type], "NICKNAME_LIST")==0){
        char list_clients_copy[MSG_LEN];
        char *list_clients = get_all_clients(clients);
        strcpy(list_clients_copy, list_clients);
        if (send(client_sock, (const char *)list_clients_copy, strlen(list_clients_copy), 0)<=0){
          perror("Cannot send the list");
        }
        free(list_clients);
        }
      /* Gestion Message Type NICKNAME_INFOS */  
      else if(strcmp(msg_type_str[message->type], "NICKNAME_INFOS")==0){
        if (get_nickname(message->infos, clients) == NULL){
          //Nickname does not exist in the list
          memset(buf_infos, 0, sizeof(buf_infos));
          strcpy(buf_infos,"No clients correspond with your request");
          printf("%s\n",buf_infos);
          if (send(client_sock,(const char *)buf_infos,strlen(buf_infos), 0)<=0){
            perror("Cannot send to the client");
          }
          // else{
          //   close(client_sock);
          // }
          
        }
        else{
        char list_infos[MSG_LEN];
        char *client_infos = get_client_infos(clients, message->infos);
        strcpy(list_infos, client_infos);
        if (send(client_sock, (const char *)list_infos, strlen(list_infos), 0)<=0){
          perror("Cannot send the infos");
        }
        free(client_infos);
        }
        }
      /* Gestion Message Type BROADCAST_SEND */
      else if(strcmp(msg_type_str[message->type], "BROADCAST_SEND")==0){
        //printf("debug1");
        memset(payload, 0, sizeof(payload));
        if (recv(client_sock, payload, message->pld_len, 0)<0){ //stocke in payload
          perror("Cannot receive the message");
        }
        else{
          if (broadcast_message(payload, clients, get_client(client_sock,clients)) == -1){ //PB corrigé
            perror("Cannot send Broadcast message");
          }
          else{ //Requirement 2.0
            fprintf(stdout, "Payload: %s to be sent in broadcast\n", payload);
            }
          printf("Broadcast message is = %s\n", payload);
          }
        }
      /* Gestion Message Type UNICAST_SEND */    
      else if(strcmp(msg_type_str[message->type], "UNICAST_SEND")==0){
        //printf("debug2");
        if (get_nickname(message->infos, clients) != NULL){ // If the nickname exists in the list
          //Easiest way to send a unicast is to get the right client through the nickname
          struct client *client = get_nickname(message->infos, clients);
          char payload_send[MSG_LEN];
          memset(payload, 0, sizeof(payload));
          if (recv(client_sock, payload, message->pld_len, 0)<0){
            //Server receives first from the user asking -->client_sock
            perror("Cannot send the payload in unicast");
          }
          else{
            sprintf(payload_send, "[ %s ] :  %s", message->nick_sender, payload);
          }
          //Then sends the message to the right client -->    client->sock
          if (send(client->socket,(const char *)payload_send,strlen(payload_send), 0)<=0){
            perror("Cannot send Unicast");
          }
          else{
            printf("Unicast message\n");
            char *message_unicast = "Unicast message sent\n";
            if (send(client_sock, message_unicast, strlen(message_unicast), 0)<=0){
              perror("Cannot send unicast message");
            }
          }
        }
        else{
          //Nickname does not exist in the list
          memset(buf_unicast, 0, sizeof(buf_unicast));
          strcpy(buf_unicast,"No clients correspond with your request");
          if (send(client_sock,(const char *)buf_unicast,strlen(buf_unicast), 0)<=0){
            perror("Cannot send to the client");
          }
        }
      }
      /* Gestion Message Type ECHO_SEND */    
      else if(strcmp(msg_type_str[message->type], "ECHO_SEND")==0){
        //printf("Un echo send detecté\n");
        //struct client *client = get_client(client_sock,clients);
        if (recv(client_sock, payload, message->pld_len, 0) <= 0){
          perror("Cannot receive payload");
        }

        if (strcmp(payload, "/quit\n") == 0){
          //printf("DEBUG QUIT\n");
          struct client *client = get_client(client_sock,clients);
          fprintf(stdout, "The client %s is now disconnected\n", client->nickname);
          client_deleted(clients, get_client(client_sock,clients));
          close(client_sock);
        }
        else{
          printf("Received: %s\n", payload);
          //Message sent back to the client//
          if (send(client_sock, (const char *)payload, strlen(payload), 0)<= 0){
            perror("Cannot send echo send");
          }
          printf("Message sent back\n");
        }
      }
      /* Gestion Message Type MULTICAST_CREATE */  
      else if(strcmp(msg_type_str[message->type], "MULTICAST_CREATE")==0){
        char created[2*MSG_LEN];
        if (salon_already_exists(message->infos, salons) == -1){
          //printf("zac\n");
          strcpy(created, "This channel already exists\n");
        }
        else{
          struct salon *start_salon = salons->start_salon;
          struct salon *new = malloc(sizeof(struct salon));
          //Client automatically inserted into the channel
          struct client *client = get_nickname(message->nick_sender, clients);

          strcpy(client->salon, message->infos);
          printf("Creation of a new channel\n");
          initialisation_salon(new,start_salon,client,message->infos);
          salons->start_salon = new;
          sprintf(created, "New chanel has been created: %s\n", new->salon_nickname);
        }
        if (send(client_sock, (const char *)created, strlen(created), 0)<=0){
          perror("Cannot create channel");
        }
      }
      /* Gestion Message Type MULTICAST_LIST */  
      else if (strcmp(msg_type_str[message->type], "MULTICAST_LIST") == 0){
        printf("List of all the channels required\n");
        char list_channels[MSG_LEN];
        char *salon_copy = get_all_salons(salons);
        strcpy(list_channels, salon_copy);
        if (send(client_sock, (const char *)list_channels, strlen(list_channels), 0) <= 0){
          perror("Cannot send the message");
        }
        free(salon_copy);
      }
      /* Gestion Message Type MULTICAST_JOIN */
      else if (strcmp(msg_type_str[message->type], "MULTICAST_JOIN") == 0){
        printf("Join request\n");
        char join[2*MSG_LEN];
        char quit_join[2*MSG_LEN];
        char join_success[MSG_LEN];
        struct salon *salon = salon_nickname(salons, message->infos);
        struct client *client = get_nickname(message->nick_sender, clients);

        if (strcmp(client->salon, "") != 0){
          //If client was already on a channel
          struct salon *current_salon = salon_nickname(salons, client->salon); //strcopy client->salon avec salon->salon_nickname permet le bon fonctionnement
                                                                              //dans la fct d'ajout
          if (salon_already_exists(message->infos,salons) == 1){
            strcpy(join_success, "This Channel cannot be found");
            if (send(client_sock, (const char *)join_success, strlen(join_success), 0)<=0){
              perror("Cannot send the message");
            }
          }
          else{
            client_deleted_salon(current_salon, client);
            client_added_salon(salon, client);
            //Delete Channel if there's nobody inside
            if (current_salon->number == 0){
              sprintf(quit_join, "There's 0 client in this channel. %s has been destroyed\n", current_salon->salon_nickname);
              if (send(client_sock, (const char *)quit_join, strlen(quit_join), 0)<=0){
                perror("Cannot destroy the channel");
              }
              salon_destroyed(salons, current_salon);
            }
            strcpy(join_success, "You are now on the new channel you selected\n");
            if (send(client_sock, (const char *)join_success, strlen(join_success), 0)<=0){
              perror("Cannot send the message");
            }
            sprintf(join, "You joined: %s\n", salon->salon_nickname);
            if (send(client_sock, (const char *)join, strlen(join), 0)<=0){
              perror("Cannot join the channel");
            }
          }
        }
        else{
          //If client is not on a channel yet
          if (salon_already_exists(message->infos,salons) == -1){
            //Has to join an existant channel
            client_added_salon(salon, get_client(client_sock, clients));
            sprintf(join_success, "You joined the following channel: %s\n", salon->salon_nickname);
          }
          else{
            //printf("test2");
            strcpy(join_success, "This Channel cannot be found");
            //strcpy(client->salon, "");
            //printf("test1");
            //printf("%s\n",client->salon);
          }
          if (send(client_sock, (const char *)join_success, strlen(join_success), 0) <= 0){
            perror("Cannot send the message");
          }
        }
      }
      /* Gestion Message Type MULTICAST_QUIT */
      else if (strcmp(msg_type_str[message->type], "MULTICAST_QUIT") == 0){
        printf("Quit request\n");
        struct client *client = get_nickname(message->nick_sender, clients);
        //struct salon *salon = salon_nickname(salons, message->infos);
        struct salon *salon = salon_nickname(salons, client->salon);
        char quit[2*MSG_LEN];
        char quit_destroy[2*MSG_LEN];
        //char quit_impossible[2*MSG_LEN];
        char disconnected[] = "You are disconnected from the channel\n";
        // if (strcmp(client->salon,message->infos)!=0){
        //   //printf("TEST DEBUG");
        //   strcpy(quit_impossible, "You must quit the right channel");
        //   if (send(client_sock, (const char *)quit_impossible, strlen(quit_impossible), 0)<=0){
        //       perror("Cannot destroy the channel");
        //   }
        // }
        // else{
          if (client_deleted_salon(salon, client)==1){
            sprintf(quit, "You were in: %s\n", salon->salon_nickname);
            //si le client choisi existe dans le salon, on le supprime du salon
            if (send(client_sock, (const char *)quit, strlen(quit), 0)<=0){
              perror("Cannot join the channel");
            }
            if (send(client_sock, (const char *)disconnected, strlen(disconnected), 0)<=0){
              perror("Cannot send the message");
            }
            if (salon->number == 0){
              //If there's no more clients in the channel, we destroy it
              sprintf(quit_destroy, "There's 0 client in this channel. %s has been destroyed\n", salon->salon_nickname);
              if (send(client_sock, (const char *)quit_destroy, strlen(quit_destroy), 0)<=0){
                perror("Cannot destroy the channel");
              }
              salon_destroyed(salons, salon);
            }
          }
          else{
            strcpy(disconnected, "Client cannot be disconnected");
            if (send(client_sock, (const char *)disconnected, strlen(disconnected), 0) <= 0){
              perror("Cannot send the message");
            }
          }
        //}
      }
      /* Gestion Message MULTICAST_SEND */
      else if (strcmp(msg_type_str[message->type], "MULTICAST_SEND") == 0){
        printf("Multicast send message requested\n");
        struct client *client = get_client(client_sock, clients);
        if (recv(client_sock, payload, message->pld_len, 0) < 0){
          perror("Cannot send the message");
        }
        char multicast_send[MSG_LEN*2];
        sprintf(multicast_send, "[%s] : %s", client->nickname, payload);
        send_salon_multicast(salon_nickname(salons, client->salon), multicast_send, clients, get_nickname(message->nick_sender, clients));
      }
      /* Gestion FILE_REQUEST */ 
      else if (strcmp(msg_type_str[message->type], "FILE_REQUEST") == 0){
        struct client *client_receiver;
        struct client *client_sender;
        char request[MSG_LEN*3];
        char request_bis[] = "Do you accept? [Y/N]\n";
        char request_name[MSG_LEN];
        char nickname_receiver[MSG_LEN];
        char file_name[MSG_LEN];
        char request_send[] = "The request has been sent to the client\n";
        char request_error[] = "This client does not exist, you cannot send him a file\n";

        strcpy(nickname_receiver, message->infos);
        client_receiver = get_nickname(nickname_receiver,clients);
        client_sender = get_client( client_sock,clients);

        if (recv(client_sock, file_name, message->pld_len, 0)<=0){
          perror("Cannot send the message");
        }
        else{
          if (get_nickname(message->infos, clients) == NULL){
            if (send(client_sock, (const char *)request_error, strlen(request_error), 0)<=0){
              perror("Cannot send the message");
            }
          }
          else{
            strcpy(client_sender->file_name, file_name);
            sprintf(request, "%s wants to transfer you the file %s\n", message->nick_sender, file_name);
            sprintf(request_name, "%s is the name of the file\n", file_name);
            if (send(client_receiver->socket, (const char *)request, strlen(request), 0)<=0){
              perror("Cannot send the request");
            }
            if (send(client_receiver->socket, (const char *)request_bis, strlen(request_bis), 0)<=0){
              perror("Cannot send the message");
            }
            if (send(client_receiver->socket, (const char *)request_name, strlen(request_name), 0)<=0){
              perror("Cannot send the request");
            }
            if (send(client_sock, (const char *)request_send, strlen(request_send), 0)<=0){
              perror("Cannot send the message");
            }
          }
        }
      }
      /* Gestion FILE_ACCEPT */
      else if (strcmp(msg_type_str[message->type], "FILE_ACCEPT")==0){
        //printf("DEBUG\n");
        struct client *client_sender = get_nickname(message->infos, clients);
        struct client *client_receive = get_nickname(message->nick_sender, clients);
        char message_to_sender[] = "The client has accepted to receive the file\n";
        char message_to_receiver[] = "Now, you have to wait for the file\n";
        char infos_for_connection[MSG_LEN*3];
        char ip_adress[128];
        memset(ip_adress, 0, sizeof(ip_adress));
        inet_ntop(AF_INET, &(client_receive->client_addr.sin_addr), ip_adress, INET_ADDRSTRLEN);
        //printf("DEBUG2\n");
        sprintf(infos_for_connection, "%i : %s > the information to connect to send the file \n", client_receive->client_addr.sin_port, ip_adress);
        //printf("DEBUG3\n");
        if (send(client_receive->socket, (const char *)message_to_receiver, strlen(message_to_receiver), 0)<=0){
          perror("Cannot send the message");
        }
        if (send(client_sender->socket, (const char *)infos_for_connection, strlen(infos_for_connection), 0)<=0){
          perror("Cannot send the message");
        }
        if (send(client_sender->socket, (const char *)message_to_sender, strlen(message_to_sender), 0)<=0){
          perror("Cannot send the message");
        }
      }

      /* Gestion FILE_REJECT */
      else if (strcmp(msg_type_str[message->type], "FILE_REJECT")==0){
        char message_to_sender[] = "The client has rejected the file\n";
        char message_to_receiver[] = "Okay, therefore no transfers will occur\n";

        struct client *client_sender = get_nickname(message->infos, clients);
        struct client *client_receive = get_nickname(message->nick_sender, clients);
        if (send(client_receive->socket, (const char *)message_to_receiver, strlen(message_to_receiver), 0)<=0){
          perror("Cannot send the message");
        }
        if (send(client_sender->socket, (const char *)message_to_sender, strlen(message_to_sender), 0)<=0){
          perror("Cannot send the message");
        }
      }
    }   
}

void socket_closure(int server_sock, struct pollfd *pollfds, int fds_num){
  close(server_sock);
  for (int i = 0; i < fds_num; i++){
    close(pollfds[i].fd);
  }
}

int accept_each_client(struct all_clients *clients, int socket, struct sockaddr *client_addr)
{
  int len_addr;
  int sock_client;
  char buf[] = "Please login with /nick <your pseudo>";
  len_addr = sizeof(client_addr);
  sock_client = accept(socket, client_addr, (socklen_t *)&len_addr); //New socket for new client
  if (sock_client < 0){
    perror("Problem Accept");
    close(sock_client);
  }
  else{
    if (send(sock_client, (const char *)buf, strlen(buf), 0)<0){
      perror("Cannot send buffer");
      close(sock_client);
      client_deleted(clients, get_client(sock_client,clients));
    }
  }
  printf("Socket number of the new client is %i\n",sock_client);
  return sock_client;
}


void connection(int port, struct all_clients *clients, struct all_salons *salons)
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
    //socklen_t size_addr = sizeof(struct sockaddr_in);

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
    salons->start_salon = NULL;
    

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
              client_sock = accept_each_client(clients, server_sock, (struct sockaddr *)&client_addr);

            //JALON 1
              // //Liste chainée Ajout//
              // struct client *new = client_added(client_addr,clients->start_client,client_sock); //first client becomes next
              // clients->start_client = new; // new client is first
              // printf("Le numero de port du client est: %d\n",new->client_addr.sin_port);
              // printf("L'adresse du client est: %s\n",inet_ntoa(new->client_addr.sin_addr));
              // printf("Test : %d\n",client_sock);
              // printf(" Numéro de Socket du nouveau client: %d\n",new->socket);

              pollfds[fds_num].fd = client_sock;
              pollfds[fds_num].events = POLLIN;
              fds_num++;
            }
            else
            {
              echo_server(pollfds[i], client_addr, clients, salons); // Haven't put :echo_server(client_sock, client_addr, clients); -->connexion lost every time 
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
  struct all_salons *salons = malloc(sizeof(struct all_salons));
  connection(atoi(argv[1]), clients, salons); //port: first arg
  free(clients);//free malloc
  free(salons); //free malloc
  return EXIT_SUCCESS;
}