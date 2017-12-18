
#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define RED         "\x1B[31m"
#define GREEN       "\x1b[32m"
#define YELLOW      "\x1b[33m"
#define CYAN        "\x1B[36m"
#define R           "\x1B[0m"

#define LSB(A) (A & 0x00FF)
#define MSB(A) ((A & 0xFF00) >> 8)

#define CONNECT     1 /**/
#define CONNACK     2 /**/

#define PUBLISH     3

#define PUBACK      4


#define SUBSCRIBE   8
#define SUBACK      9

#define UNSUBSCRIBE 10
#define UNSUBACK    11
 
#define CREATP      12 /**/
#define CREATPACK   5 /**/


// list users and list users ACK
#define LISTUSR     13 /**/
#define LISTUSRACK  7 /**/


//List topics and list topics ACK
#define LISTTP      0 /**/
#define LISTTPACK   6 /**/

#define DISCONECT   14 /**/ 


 /* max byte use for encode message */
#define MAX_RM_LEN  4
#define HEADER_LEN  1

 /* option for list topic, list user packet */

// All for list all topic OR list all user that belong to topic
#define ALL         0

// list all topic that user belong to <LISTTP> or list all user belong to topic <LISTUSR>
#define CONT_ME     1 /* CONTAIN 'ME' */

//list all topic that USER DOES NOT BELONG TO
#define N_CONT_ME   2 /* NOT CONTAIN 'ME' */


/* status for user */
#define OFF         0 // offline
#define ON          1 // online


// RESULT CODE FOR CREATE TOPIC < 0 IF USER DOES NOT EXIST, 1 IF SUCCESS>
#define RC_FAILED   0 
#define RC_SUCCESS  1


#define TOPIC       0 /* Send to TOPIC */
#define USER        1 /* Send to USER */


//=============================== MORE 
struct topic {
    char *name;
    struct topic *next;
};

struct code { //// list of RETURN CODE 1 for SUCCESS and 2 for FAILED
    char retCode;
    struct code *next;
};

struct user{
    char *name;
    char status;
    struct user *next;
};


// | 7 | 6 | 5 | 4 |    3     | 2 | 1 |   0    |
// |     TYPE      |    3     |   |   | regist |
struct Connect {
    bool regist;

    char *username;
    char *password;
};

struct CrTopicPkt{
    char *name;
    struct user *first;
};

struct CrTopicAck{
    char rc; /* RESULT CODE */
    struct code *first;
};

struct Publish {
    char topic_or_user;

    char *sender_name;
    char *target;
    char *message;
};



//==============================================================================

struct code *addCode(struct code *first, char retCode);

void printCode(struct code *cd);

struct user *addUser(struct user *first, char *name, char status);

void printUser(struct user *first);

struct topic *addTopic(struct topic *first, char *name);

void printTopic(struct topic *top);

//============================================================================================
char *field(char *pm, int *len);

void printPacket(char *packet, int len);

void printConnect(struct Connect *con);


//============================================

void printCreateTopic(struct CrTopicPkt *cr);

void printCreateTopicAck(struct CrTopicAck *cr);

//============================================

char *merge(char *ptr, char* string);

void printPublish(struct Publish *pub);



///==========================================PACKET SENT BY SOCKET=====================================

/* 1 */
char *encode_Connect(struct Connect *conn, int *len);

struct Connect* decode_Connect(char *packet);

/* 2 */
char *encode_Connack(int retCode, int *len);

int decode_Connack(char *packet);


/* 3 */
char *encode_Listtpack(struct topic *first, int *len);

struct topic *decode_Listtpack(char *packet);


/* 4 */

// | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0|
// |      TYPE     | 3 | 2 |  OPT |   


/* 5 */
char *encode_Listtp(int option, int *len);

/* return OPTION */
int decode_Listtp(char *packet);


/* 6 */
char *encode_Listusr(char *tpname, int *len);

char *decode_Listusr(char *packet);


/* 7 */
char *encode_Listusrack(struct user *first, int *len);

struct user *decode_Listusrack(char *packet);

// 8

char *encode_Creatp(struct CrTopicPkt *ctp, int *len);

struct CrTopicPkt *decode_Creatp(char *packet);

char *encode_Creatpack(struct CrTopicAck* cr, int *len);

struct CrTopicAck *decode_Creatpack(char *packet);


//===========================================================================================



char *encode_Publish(struct Publish *pub, int *len);

struct Publish *decode_Publish(char *packet);

char *encode_Puback(int retCode, int *len);

int decode_Puback(char *packet);



char *encode_Subscribe(struct topic *first, int *len);

struct topic *decode_Subscribe(char *packet);

char *encode_Suback(struct code *first, int *len);

struct code *decode_Suback(char *packet);


char *encode_Unsubscribe(struct topic *un, int *len);

struct topic *decode_Unsubscribe(char *packet);

char *encode_Unsuback(struct code *first, int *len);

struct code *decode_Unsuback(char *packet);



char *encode_Disconnect(int *len);


#endif
