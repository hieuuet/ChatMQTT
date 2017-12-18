#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "packet.h"
#include "db.h"

struct Connect*
_connect(char *buffer, int connfd, int *rc){
    printf("=====================function _connect====================\n");
    struct Connect *con = decode_Connect(buffer);
    int len;
    int retCode;

    *rc = db_auth(con->username, con->password, connfd);
    if (*rc == FAILED){
        retCode = CON_AUTH_FAILED;
    } else {
        retCode = CON_OK;
    }
    
    char *packet = encode_Connack(retCode, &len);
    if (retCode == CON_OK){
         char sql[1024];
        sprintf(sql, "update users set status = 'on', connfd = '%d' where uname = '%s';", connfd, con->username);
        db_exec_stm(sql);
    }
   
    write(connfd, packet, len);
    printf("=====================function _connect end================\n");

    return con;
}

void
list_topic(int connfd, char* username, int opt){
    printf("=====================function list topics=================\n");

    char sql[1024];
    if (opt == 0){
        sprintf(sql, "select * from topics;");
    } else if (opt == 1){
        sprintf(sql, "select * from topic_user where uname = '%s';", username);
    } else {
        sprintf(sql, "select tpname from topics where tpname not in (select tpname from topic_user where uname = '%s');", username);
    }

    int len;
    struct topic *first = db_listTopic((char*)sql);
    char *pp = encode_Listtpack(first, &len);
    write(connfd, pp, len);
    printf("=====================function list topics end=============\n");

}

void
list_user(int connfd, char *packet){
    printf("=====================function list users==================\n");
    
    char *uname = decode_Listusr(packet);
    int len;
    struct user *first = db_listUser(uname);
    char *pp = encode_Listusrack(first, &len);
    printPacket(pp, len);
    write(connfd, pp, len);
    printf("=====================function list users end==============\n");
    
}

void
create_topic(int connfd, char *buffer, char *user){
    printf("=====================function create_topic================\n");
    
    struct CrTopicPkt *cr = decode_Creatp(buffer);
    struct CrTopicAck *ack = db_createTopic(cr, user);
    printCreateTopicAck(ack);
    int len;
    char *pp = encode_Creatpack(ack, &len);
    write(connfd, pp, len);
    printf("=====================function create_topic end============\n");
    
}

void
subscribe(int connfd, char *buffer, char *user){
    printf("=====================function subscribe===================\n");
    
    struct topic *first = decode_Subscribe(buffer);
    struct code *cd = db_subscribe(first, user);
    int len;
    char *pp = encode_Suback(cd, &len);
    write(connfd, pp, len);
    printf("=====================function subscribe end===============\n");
    
}


void
unsubscribe(int connfd, char *buffer, char *user){
    printf("=====================function unsubscribe=================\n");
    ;
    struct topic *first = decode_Unsubscribe(buffer);
    struct code *cd = db_unsubscribe(first, user);
    int len;
    char *pp = encode_Unsuback(cd, &len);
    write(connfd, pp, len);
    printf("=====================function unsubscribe end=============\n");
    
}

void
publish(int connfd, char *buffer, char *user){
    printf("=====================function publish=====================\n");

    struct Publish *pub = decode_Publish(buffer);
    int ret = db_sendMessage(pub);
    int len;
    char *pp = encode_Puback(ret, &len);
    write(connfd, pp, len);
    printf("=====================function publish end=================\n");
}


void disconnect(char *username){
    printf("=====================function disconnect==================\n");
    
    char sql[1024];
    sprintf(sql, "update users set status = 'off' where uname = '%s';", username);
    db_exec_stm(sql);
    printf("=====================function disconnect end==============\n");
}

void*
do_response(void *ptr){
	int connfd = *(int *)ptr;
	char *buffer = malloc(1024);
    read(connfd, buffer, 1024);
    if(strlen(buffer) == 0){
        /* if client disconnect then server close connection */
        return NULL;
    }

    int rc;
    struct Connect *con = _connect(buffer, connfd, &rc);
    /* username & passoword */

    if(rc == FAILED){
        return NULL;
    }

	while(1){
		buffer = malloc(1024);
        int len = read(connfd, buffer, 1024);
        if(len == 0){
            disconnect(con->username);
            break;
        }

        int type = (*buffer >> 4) & 15;
        switch (type){
            case LISTTP:
                list_topic(connfd, con->username, *buffer & 3);
                continue;

            case LISTUSR:
                list_user(connfd, buffer);
                continue;

            case CREATP:
                create_topic(connfd, buffer, con->username);
                continue;

            case SUBSCRIBE:
                subscribe(connfd, buffer, con->username);
                continue;

            case UNSUBSCRIBE:
                unsubscribe(connfd, buffer, con->username);
                continue;

            case PUBLISH:
                publish(connfd, buffer, con->username);
                continue;

            case DISCONECT:
                disconnect(con->username);
                break;

            default:
                break;
        }
        free(buffer);
        
	}
	printf("Client has disconnected\n");
	return NULL;
}

int main(){
	int listenfd = 0;
    int *connfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len;
    char cli_addr_str[100];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        fprintf(stderr, "Error: Create listen socket failed!\n");
        exit(1);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error: Set flag SO_REUSEADDR failed!\n");
        exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1900);

    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error: Bind failed!\n");
        exit(1);
    }

    if (listen(listenfd, 20) < 0) {
        fprintf(stderr, "Error: Listening failed!\n");
        exit(1);
    }

    while (1) {
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &cli_len);
        strcpy(cli_addr_str, inet_ntoa(cli_addr.sin_addr));
        fprintf(stdout, "Client connect: %s:%d\n", cli_addr_str, cli_addr.sin_port);
        if (*connfd < 0) {
            fprintf(stderr, "Error: Accept failed!\n");
        }

        pthread_t tid;
        pthread_create(&tid, NULL, &do_response, (void*) connfd);
    }

}
