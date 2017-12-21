
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "packet.h"

#define CON_OK  0
#define CON_AUTH_FAILED  4
#define CON_ACC_EXISTED  5

int sockfd = 0;

char *username;
char *password;

int key = 1;

void* receiveMsg(void *ptr){
    int connfd = *(int *)ptr;

    while (1){
        char *buffer = malloc(1024);
        read(connfd, buffer, 1024);

        // printPacket(buffer, len);
        // printf("===============\n");
        int type = (*buffer >> 4) & 15;

        if(type == LISTTPACK ){
            struct topic *top = decode_Listtpack(buffer);
            printTopic(top);
            key = 1;
        } else if( type == LISTUSRACK) {
            struct user *first = decode_Listusrack(buffer);
            printUser(first);
            key = 1;
        } else if (type == CREATPACK){
            struct CrTopicAck *ack = decode_Creatpack(buffer);
            printCreateTopicAck(ack);
            key = 1;
        } else if (type == SUBACK ){
            struct code *first = decode_Suback(buffer);
            printCode(first);
            key = 1;
        } else if (type == UNSUBACK){
            struct code *first = decode_Unsuback(buffer);
            printCode(first);
            key = 1;
        } else if(type == PUBACK){
            int ret = decode_Puback(buffer);
            if(ret == 0){
                printf("Noone receive message!\n");
            } else {
                printf("Some people received your message!\n");
            }
            key = 1;
        } else if (type == PUBLISH){
            printf("\r");
            struct Publish *pub = decode_Publish(buffer);
            printPublish(pub);
            printf(GREEN ">>> " R);
            fflush(stdout);
           
        }
        
    }   
}

void
get_list_topics(int opt){
    int len;
    char *pp = encode_Listtp(opt, &len);
    write(sockfd, pp, len);
}

void
get_list_users(char *topic){
    int len;
    char *pp = encode_Listusr(topic, &len);
    write(sockfd, pp, len);
}

void
create_topic(char *tpname, struct user *first){
    struct CrTopicPkt *cr = malloc(sizeof(struct CrTopicPkt));
    cr->name = tpname;
    cr->first = first;
    printUser(cr->first);
   
    int len;
    char *pp = encode_Creatp(cr, &len);
    write(sockfd, pp, len);
}

void
chatting(int option, char *target, char *message){
    struct Publish *pub = malloc(sizeof(struct Publish));
    pub->topic_or_user = option;
    pub->sender_name = username;
    pub->target = target;
    pub->message = message;
    int len;
    char *pp = encode_Publish(pub, &len);
    write(sockfd, pp, len);
   
}

void
subscribe(struct topic *tp){
    int len;
    char *pp = encode_Subscribe(tp, &len);
    write(sockfd, pp, len);
}

void
unsubscribe(struct topic *tp){
    int len;
    char *pp = encode_Unsubscribe(tp, &len);
    write(sockfd, pp, len);
}


void
disconnect(){
    int len;
    char *pp = encode_Disconnect(&len);
    write(sockfd, pp, len);
}

int 
main(int argc, char *argv[]){
    struct sockaddr_in serv_addr;
    // char *IP_ADDR = "127.0.0.1";

    if (argc != 2) {
       fprintf(stderr, "Error: Missing IP address!\n");
       exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: Create socket failed!\n");
        exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1900);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: IP failed!\n");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stdout, "Error: Connection failed!\n");
        exit(1);
    }

    username = malloc(1024);
    password = malloc(1024);

    printf("Username:\n");
    printf(GREEN ">>> " R);
    fgets(username, 1024, stdin);
    *(username + strcspn(username, "\n")) = 0;
  
    printf("Password:\n");
    printf(GREEN ">>> " R);
    fgets(password, 1024, stdin);
    *(password + strcspn(password, "\n")) = 0;

    struct Connect *con = malloc(sizeof(struct Connect));
    con->username = username;
    con->password = password;

   
    int len;
    char *code = encode_Connect(con, &len);
    write(sockfd, code, len);

    char *buff = malloc(1024);
    read(sockfd, buff, 1024);
    int retCode = decode_Connack(buff);

    if (retCode == CON_AUTH_FAILED){
        printf("Wrong username or password!\n");
        return 0;
    }

    //==========================================AUTH=====================================================


    pthread_t tid;
    pthread_create(&tid, NULL, &receiveMsg, &sockfd);


    const char *ch = ".";
    printf(CYAN);
    printf(" .---------------------------------------------------.\n");
    printf(" |1. get.topics.[option]                             |\n");
    printf(" |               [0] All topics                      |\n");
    printf(" |               [1] Contain me                      |\n");
    printf(" |               [2] Not contain me                  |\n");
    printf(" |2. get.users.[topic name]                          |\n");
    printf(" |3. create.[topic name].[user_1]. .[user_n]         |\n");
    printf(" |4. chat.[option].[target].[message]                |\n");
    printf(" |            [0] send to topic                      |\n");
    printf(" |            [1] send to user                       |\n");
    printf(" |6. subs.[topic_1]. .[topic_n]                      |\n");
    printf(" |7. unsub.[topic_1]. .[topic_n]                     |\n");
    printf(" |8. exit                                            |\n");
    printf(" *---------------------------------------------------*\n");
    printf(R);

    while (1){
        printf(GREEN ">>> " R);
        char *input = malloc(1024);
        fgets(input, 1024, stdin);
        *(input + strcspn(input, "\n")) = 0;
        fflush(stdin);
        char *tok = strtok(input, ch);

        key = 0;

        if (strcmp(tok, "get") == 0){
            tok = strtok(NULL, ch);
            if (strcmp(tok, "topics") == 0){
                int opt = atoi(strtok(NULL, ch));
                get_list_topics(opt);
            } else if (strcmp(tok, "users") == 0){
                tok = strtok(NULL, ch);
                get_list_users(tok);
            }
            

        } else if (strcmp(tok, "create") == 0){
            char *name = strtok(NULL, ch);
            struct user *u1 = NULL;
            while(1){
                tok = strtok(NULL, ch);
                if(tok == NULL){
                    break;
                } else {
                    u1 = addUser(u1, tok, 0);
                }
            }
            create_topic(name, u1);

        } else if (strcmp(tok, "chat") == 0){

            int opt = atoi(strtok(NULL, ch));
            char *target = strtok(NULL, ch);
            char *message = strtok(NULL, ch);
            chatting(opt, target, message);

        } else if (strcmp(tok, "subs") == 0){
            struct topic *tp = NULL;
            while(1){
                tok = strtok(NULL, ch);
                if(tok == NULL){
                    break;
                } else {
                    tp = addTopic(tp, tok);
                }
            }
            subscribe(tp);
        } else if (strcmp(tok, "unsub") == 0){
            struct topic *tp = NULL;
            while(1){
                tok = strtok(NULL, ch);
                if(tok == NULL){
                    break;
                } else {
                    tp = addTopic(tp, tok);
                }
            }
            unsubscribe(tp);
        } else if (strcmp(tok, "exit") == 0){
            disconnect();
            printf("Bye bye!\n");
            key = 1;
            break;
        } else {
            printf("Unkown!\n");
            key = 1;
            continue;
        }

        while(!key);
        free(input);

    }
    
    return 0;
}
