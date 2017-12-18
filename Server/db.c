
#include "db.h"

sqlite3 *db;



//======================= CALLBACK FUNCTIONS <only visible in db.c> =================================


    /* check if username if existed */
static int
ch_exist_pr(void *data, int argc, char **argv, char **azColName){
    struct Acc *acc  = (struct Acc *)data;
    if(strcmp(acc->username, argv[1]) == 0){
        return -1;
    }
    return 0;
}



/*authorized account */
static int
auth_pr(void *data, int argc, char **argv, char **azColName){
    struct Acc *acc  = (struct Acc *)data;
    // printf("row: uname: %d, pass: %d\n", strcmp(acc->username, argv[0]), strcmp(acc->password, argv[1]));
    if(strcmp(acc->username, argv[0]) == 0 && strcmp(acc->password, argv[1]) == 0){
        return -1;
    }
    
    return 0;
}

static int 
listtp_pr(void *data, int argc, char **argv, char **azColName){
    struct topic *first = (struct topic*) data;
    struct topic *new = malloc(sizeof(struct topic));

    new->name = malloc(1024);
    strcpy(new->name, argv[0]);
    new->next = NULL;

    struct topic *top = first;
    while(top->next != NULL){
        top = top->next;
    }
    top->next = new;

    return 0;
}


static int 
listusr_pr(void *data, int argc, char **argv, char **azColName){
    struct user *first = (struct user*) data;
    struct user *new = malloc(sizeof(struct user));

    new->name = malloc(1024);
    strcpy(new->name, argv[1]);
    new->status = strcmp(argv[2], "on") == 0 ? ON : OFF;
    new->next = NULL;

    struct user *top = first;
    while(top->next != NULL){
        top = top->next;
    }
    top->next = new;

    return 0;
}

static int 
publish_pr(void *data, int argc, char **argv, char **azColName){
    struct code *first = (struct code*)data;

    struct code *cd = malloc(sizeof(struct code));
    cd->retCode = atoi(argv[0]);
    cd->next = NULL;

    struct code *q = first;
    while(q->next != NULL){
        q = q->next;
    }
    q->next = cd;

    return 0;
}

//======================= get connection to DATABASE NAMED "news", call before and after db manipulate

void
opendb(){
    int rc = sqlite3_open("news", &db);

    if (rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    
}

void
closedb(){
    sqlite3_close(db);
}

//================================================================================================================

/* db add user */
int
db_addUser(char *username, char *password, int connfd){
    printf("=====================db_addUser===========================\n");
    opendb();
    int rc;
    char *err = 0;
    char *sql1 = "select * from users;";
    struct Acc *acc = malloc(sizeof(struct Acc));
    acc->username = username;
    acc->password = password;

    rc = sqlite3_exec(db, sql1, ch_exist_pr, (void *) acc, &err);

    /* callback "ch_exist_pr" return 0 then RESULT CODE 'rc' = SQLITE_ABORT 
                                                            else continue insert user to db */
    if(rc == SQLITE_ABORT){
        return FAILED;
    }

    char sql2[1024];
    sprintf(sql2, "insert into users (uname, pass, connfd, status) values('%s', '%s', '%d', '%s');", username, password, connfd, "on");
    rc = sqlite3_exec(db, sql2, NULL, 0, &err);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    closedb();
    printf("======================db_addUser done=====================\n");
    return OK;
}


// db authorized account 
int
db_auth(char *username, char *password, int connfd){
    printf("======================db_auth=============================\n");
    opendb();
    int rc;
    char *err;
    char *sql = "select * from users;";
    struct Acc *acc = malloc(sizeof(struct Acc));
    acc->username = username;
    acc->password = password;
    rc = sqlite3_exec(db, sql, auth_pr, (void *) acc, &err);
    if(rc == SQLITE_ABORT){
        char sql2[1024];
        sprintf(sql2, "update users set connfd = '%d', status = 'on' where uname = '%s';", connfd, username);
        sqlite3_exec(db, sql2, NULL, 0, &err);
        return OK;
    }
    closedb();
    printf("=======================db_auth done=======================\n");
    return FAILED;
}



//=================================list all topic from db 
struct topic *db_listTopic(char *sql){
    printf("=======================db_listTopic=======================\n");
    opendb();
    int rc;
    char *err;

    struct topic *first = malloc(sizeof(struct topic));
    first->name = "temp";
    first->next = NULL;
    printf("%s\n", sql);

    rc = sqlite3_exec(db, sql, listtp_pr, (void*)first, &err);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return NULL;

    } else {
        // remove first topic 'temp' 
        first = first->next;
    }

    closedb();
    printf("=======================db_listTopic end===================\n");
    return first;
}

struct user *db_listUser(char *tpname){
    printf("=======================db_listUser========================\n");

    opendb();
    int rc;
    char *err;
    char sql[1024];

    // select tpname, tu.uname, status from topic_user as tu inner join users as u where tu.uname = u.uname and tpname = '%s';


    sprintf(sql, "select tpname, tu.uname, status from topic_user as tu inner join users as u where tu.uname = u.uname and tpname = '%s';", tpname);

    struct user *first = malloc(sizeof(struct user));
    first->name = "temp";
    first->status = 0;
    first->next = NULL;

    rc = sqlite3_exec(db, sql, listusr_pr, (void*)first, &err);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return NULL;

    } else {
        // remove first topic 'temp' 
        first = first->next;
    }

    closedb();
    printf("=======================db_listUser end====================\n");
    return first;
}



void db_exec_stm(char *sql){
    printf("=======================db_exec_stm========================\n");
    opendb();
    char *err;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return;

    }

    closedb();
    printf("=======================db_exec_stm end====================\n");
}


struct CrTopicAck *db_createTopic(struct CrTopicPkt *cr, char *user){
    printf("=======================db_createTopic=====================\n");
    opendb();
    
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    
    struct CrTopicAck *ack = malloc(sizeof(struct CrTopicAck));
    int rc;
    char sql1[1024];
    sprintf(sql1, "insert into topics (tpname, usrcreated) values ('%s', '%s');", cr->name, user);
    rc = sqlite3_exec(db, sql1, NULL, NULL, NULL);
    if( rc != SQLITE_OK ){
        ack->rc = RC_FAILED;
        return ack;
    }
    ack->rc = RC_SUCCESS;

    char sql2[1024];
    sprintf(sql2, "insert into topic_user (tpname, uname) values ('%s', '%s');", cr->name, user);
    printf("%s\n", sql2);
    rc = sqlite3_exec(db, sql2, NULL, NULL, NULL);

    struct code *cd = NULL;
    struct user *u1 = cr->first;
    while(u1 != NULL){
        char sql[1024];
        printf("day %d\n", strcmp(cr->name, user));
        if(strcmp(u1->name, user) == 0){
            cd = addCode(cd, RC_SUCCESS);
            u1 = u1->next;
            continue;
        }
        sprintf(sql, "insert into topic_user (tpname, uname) values ('%s', '%s');", cr->name, u1->name);
        printf("%s\n", sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if( rc == SQLITE_OK ){
            cd = addCode(cd, RC_SUCCESS);
        } else {
            cd = addCode(cd, RC_FAILED);
        }
        u1 = u1->next;
    }
    ack->first = cd;
    printCode(cd);

    closedb();
    printf("=======================db_createTopic end=================\n");
    return ack;
}

struct code *
db_subscribe(struct topic *top, char *user){
    printf("=======================db_subscribe=======================\n");
    opendb();
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    int rc;
    struct code *first = NULL;
    struct topic *tp = top;
    while(tp != NULL){
        char sql[1024];
        sprintf(sql, "insert into topic_user (tpname, uname) values ('%s', '%s');", tp->name, user);
        printf("%s\n", sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if( rc == SQLITE_OK ){
            first = addCode(first, RC_SUCCESS);
        } else {
            first = addCode(first, RC_FAILED);
        }
        tp = tp->next;
    }
    closedb();
    printf("=======================db_subscribe end===================\n");
    return first;
}

struct code *
db_unsubscribe(struct topic *top, char *user){
    printf("=======================db_unsubscribe=====================\n");
    opendb();
    int rc;
    struct code *first = NULL;
    struct topic *tp = top;
    while(tp != NULL){
        char sql[1024];
        sprintf(sql, "delete from topic_user where tpname = '%s' and uname = '%s';", tp->name, user);
        printf("%s\n", sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if( rc == SQLITE_OK ){
            first = addCode(first, RC_SUCCESS);
        } else {
            first = addCode(first, RC_FAILED);
        }
        tp = tp->next;
    }
    closedb();
    printf("=======================db_unsubscribe end=================\n");
    return first;
}

int
db_sendMessage(struct Publish *pub){
    printf("=======================db_sendMessage=====================\n");
    opendb();
    char sql[1024];
    if (pub->topic_or_user == TOPIC){
        sprintf(sql, "select u.connfd, u.uname from topic_user tu inner join users u where tu.uname = u.uname and u.status = 'on' and tu.tpname = '%s' and u.uname != '%s';", pub->target, pub->sender_name);
    } else {
        sprintf(sql, "select connfd from users where uname = '%s' and status = 'on';", pub->target);
    }

    printf("sql: %s\n", sql);

    struct code *first = malloc(sizeof(struct code));
    first->retCode = 0;
    first->next = NULL;

    sqlite3_exec(db, sql, publish_pr, (void*) first, NULL);

    first = first->next;
    if (first == NULL){
        return 0;
    }
    int len;
    char *pp = encode_Publish(pub, &len);

    struct code *pc = first;
    while(pc != NULL){
        write(pc->retCode, pp, len);
        pc = pc->next;
    }

    closedb();
    printf("=======================db_sendMessage end=================\n");
    return 1;
}


