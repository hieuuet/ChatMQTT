
#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h> 

#include "packet.h"

#define FAILED 3
#define OK 4

#define CON_OK  0
#define CON_AUTH_FAILED  4
#define CON_ACC_EXISTED  5

extern sqlite3 *db;

struct Acc{
	char *username;
	char *password;
};

void
opendb();

void 
closedb();

int
db_addUser(char *username, char *password, int connfd);

int
db_auth(char *username, char *password, int connfd);

/// return a linked list topic 
struct topic *
db_listTopic(char *sql);

/* list all user that subscribing topic 'tpname' */
struct user *
db_listUser(char *tpname);

struct CrTopicAck *
db_createTopic(struct CrTopicPkt *cr, char *user);

void 
db_exec_stm(char *sql);

struct code *
db_subscribe(struct topic *first, char *user);

struct code *
db_unsubscribe(struct topic *first, char *user);

int
db_sendMessage(struct Publish *pub);




#endif
