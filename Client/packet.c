
#include "packet.h"

struct code *addCode(struct code *first, char c){

    struct code *cd = malloc(sizeof(struct code));
    cd->retCode = c;
    cd->next = NULL;

    if (first == NULL){   
        return cd;
    }

    struct code *q = first;
    while(q->next != NULL){
        q = q->next;
    }
    q->next = cd;

    return first;
}

void printCode(struct code *q){
    struct code *pt = q;
    int i = 1;
    while (pt != NULL){
        printf("%d. %s\n", i, pt->retCode ? YELLOW "Ok" R : RED "Failed" R);
        pt = pt->next;
        i++;
    }
}

struct topic *addTopic(struct topic *first, char *name){
   
    struct topic *new = malloc(sizeof(struct topic));
    new->name = name;
    new->next = NULL;
    if (first == NULL){   
        return new;
    }

    struct topic *top = first;
    while(top->next != NULL){
        top = top->next;
    }
    top->next = new;
    return first;
}

void printTopic(struct topic *top){
    if(top == NULL){
        printf("There is no topic for your order!\n");
        return;
    }
    struct topic *pt = top;
    while (pt != NULL){
        printf("Topic name: %s\n", pt->name);
        pt = pt->next;
    }
}

struct user *addUser(struct user *first, char *name, char status){
    struct user *u = malloc(sizeof(struct user));
    u->name = name;
    u->status = status;
    u->next = NULL;

    if (first == NULL){   
        return u;
    }

    struct user *q = first;
    while(q->next != NULL){
        q = q->next;
    }
    q->next = u;

    return first;
}

void printUser(struct user *first){
    if (first == NULL){
        printf("There is no user for your order!\n");
    }
    struct user *u = first;
    int i = 1;
    while (u != NULL){
        printf("%d. %s %s\n", i, u->status ? YELLOW "Online" R : RED "Off   " R, u->name);
        u = u->next;
        i++;
    }
}


//====================================================================================================================================
static char* encodeList(struct topic *top, int *len){
    int count = 0;
    int ploadLen = 0;
    struct topic *pt = top;
    while (pt != NULL){
        count++;
        ploadLen = ploadLen + 2 + strlen(pt->name);
        pt = pt->next;
    }
    char *packet = malloc(2 + ploadLen);
    char *pp = packet;
    /* pp[0] use of headeer */
    pp++;
    *pp = count;
    pp++;
    pt = top;
    while(pt != NULL){
        *pp = MSB(strlen(pt->name));
        pp++;
        *pp = LSB(strlen(pt->name));
        pp++;
        strncpy(pp, pt->name, strlen(pt->name));
        pp = pp + strlen(pt->name);
        pt = pt->next;
    }
    *len = 2 + ploadLen;
    return packet;
}

static struct topic *decodeList(char* packet){
    struct topic *first = NULL;
    char *pp = packet;
    pp++;
    int count = *pp;
    pp++;
    int len = 0;
    int i = 0;
    while(i < count){
        char *tpname = field(pp, &len);
        first = addTopic(first, tpname);
        pp = pp + len;
        i++;
    }
    return first;
}

static char *encodeListCode(struct code *first, int *len){
    struct code *c = first;
    int count = 0;

    while(c != NULL){
        count++;
        c = c->next;
    }
    char *packet = malloc(count + HEADER_LEN + 1);
    char *pp = packet;
    pp++;
    *pp = count;
    pp++;
    c = first;
    while(c != NULL){
        *pp = c->retCode;
        pp++;
        c = c->next;
    }
    *len = HEADER_LEN + count + 1;
    return packet;
}

static struct code *decodeListCode(char *packet){
    char *pp = packet;
    pp++;
    struct code *first = NULL;
    int count = *pp;
    int i = 0;
    while (i < count){
        pp++;
        first = addCode(first, *pp);
        i++;
    }

    return first;
}


//======================================================================================================

char *field(char *pm, int *len){
    *len = (pm[0] << 8) + pm[1] + 2;
    char *data = malloc(*len - 2);
    strncpy(data, pm + 2, *len - 2);
    *(data + *len - 2) = 0;
    return data;
}

void printPacket(char *packet, int len){
    char *ptr = packet;
    for(int i = 0; i < len; i++){
        printf("char[%d]: %c \tvalue of char: %d\n", i, *(ptr + i), *(ptr + i));
    }
}

void printConnect(struct Connect *con){
    printf("=================================\n");
    printf("register: %s\n", con->regist ? "True" : "False");
    printf("username: %s\n", con->username);
    printf("password: %s\n", con->password);
    printf("=================================\n");
}

void printCreateTopic(struct CrTopicPkt *cr){

    printf("=================================\n");
    printf("name: %s\n", cr->name);
    printUser(cr->first);
    printf("=================================\n");
}


void printCreateTopicAck(struct CrTopicAck *cr){
    printf("=================================\n");
    printf("retCode: %d\n", cr->rc);
    if(cr->rc == 1){
        printCode(cr->first);
    }
    printf("=================================\n");
}


void printPublish(struct Publish *pub){
    // printf("=================================\n");
    if (pub->topic_or_user == TOPIC){
        printf("From topic %s, %s said: %s\n", pub->target, pub->sender_name, pub->message);
    } else {
        printf("%s have sent message to you: %s\n", pub->sender_name, pub->message);
    }
    // printf("=================================\n");
}



char *merge(char *ptr, char* string){
    ptr[0] = MSB(strlen(string));
    ptr[1] = LSB(strlen(string));
    strcat(ptr + 2, string);
    ptr = ptr + 2 + strlen(string);
    return ptr;
}

char *encode_Connect(struct Connect *con, int *len){
    
    int ploadLen = 0;
    ploadLen = ploadLen + 2 + strlen(con->username);
    ploadLen = ploadLen + 2 + strlen(con->password);
    int packetLen = HEADER_LEN + ploadLen;

    // | 7 | 6 | 5 | 4 |    3     | 2 | 1 |   0    |
    // |     TYPE      |    3     |   |   | regist |

    char *packet = malloc(packetLen);
    char *pp = packet;

    *pp = CONNECT << 4;
    *pp |= con->regist;
    pp = pp + HEADER_LEN;

    pp = merge(pp, con->username);
    pp = merge(pp, con->password);

    *len = packetLen;
    return packet;
}

struct Connect* decode_Connect(char *packet){
    char *pp = packet;

    struct Connect *con = malloc(sizeof(struct Connect));
    con->regist = *pp & 1;
    pp = pp + 1;

    int len = 0;
    con->username = field(pp, &len);
    pp = pp + len;

    con->password = field(pp, &len);
    pp = pp + len;
    return con;
}

char *encode_Connack(int retCode, int *len){
    char *packet = malloc(1);
    *packet = CONNACK << 4;
    *packet |= retCode;
    *len = 1;
    return packet;
}

int decode_Connack(char *packet){
    return *packet  & 15;
}

char *encode_Publish(struct Publish *pub, int *len){
    int ploadLen = 0;
    ploadLen = ploadLen + 2 + strlen(pub->sender_name);
    ploadLen = ploadLen + 2 + strlen(pub->target);
    ploadLen = ploadLen + 2 + strlen(pub->message);

    char *packet = malloc(ploadLen + HEADER_LEN);
    char *pp = packet;
    *pp = PUBLISH << 4;
    *pp |= pub->topic_or_user;
    pp++;

    *pp = MSB(strlen(pub->sender_name));
    pp++;
    *pp = LSB(strlen(pub->sender_name));
    pp++;
    strncpy(pp, pub->sender_name, strlen(pub->sender_name));
    pp = pp + strlen(pub->sender_name);

     
    *pp = MSB(strlen(pub->target));
    pp++;
    *pp = LSB(strlen(pub->target));
    pp++;
    strncpy(pp, pub->target, strlen(pub->target));
    pp = pp + strlen(pub->target);


    *pp = MSB(strlen(pub->message));
    pp++;
    *pp = LSB(strlen(pub->message));
    pp++;
    strncpy(pp, pub->message, strlen(pub->message));
    pp = pp + strlen(pub->message);


    *len = ploadLen + HEADER_LEN;
    return packet; 

}

struct Publish *decode_Publish(char *packet){
    char *pp = packet;
    struct Publish *pub = malloc(sizeof(struct Publish));
    pub->topic_or_user = *pp & 1;
    pp++;

    int len;
    pub->sender_name = field(pp, &len);
    pp = pp + len;

    pub->target = field(pp, &len);
    pp = pp + len;

    pub->message = field(pp, &len);
    pp = pp + len;

    return pub;
}

char *encode_Puback(int retCode, int *len){
    char *packet = malloc(1);
    *packet = PUBACK << 4;
    *packet |= retCode;
    *len = 1;
    return packet;
}

int decode_Puback(char *packet){
    return *packet & 15;
}

char *encode_Listtpack(struct topic *top, int *len){
    char *packet = encodeList(top, len);
    *packet = LISTTPACK << 4;
    return packet;
}

struct topic *decode_Listtpack(char *packet){
    return decodeList(packet);
}

char *encode_Listtp(int option, int *len){
    char *packet = malloc(1);
    *packet = LISTTP << 4;
    *packet |= option;
    *len = 1;
    return packet;
}

int decode_Listtp(char *packet){
    return *packet & 3;
}

char *encode_Listusrack(struct user *first, int *len){
    int count = 0;
    int ploadLen = 0;
    struct user *pu = first;
    while (pu != NULL){
        count++;
        ploadLen = ploadLen + 3 + strlen(pu->name);
        pu = pu->next;
    }
    char *packet = malloc(2 + ploadLen);
    char *pp = packet;
    *pp = LISTUSRACK << 4;
    pp++;
    *pp = count;
    pp++;
    pu = first;
    while(pu != NULL){
        *pp = MSB(strlen(pu->name));
        pp++;
        *pp = LSB(strlen(pu->name));
        pp++;
        strncpy(pp, pu->name, strlen(pu->name));
        pp = pp + strlen(pu->name);
        *pp = pu->status;
        pp++;
        pu = pu->next;
    }
    *len = 2 + ploadLen;
    return packet;
}

struct user *decode_Listusrack(char *packet){
    char *pp = packet;
    pp++;

    // number of users
    struct user *first = NULL;
    int count = *pp;
    pp++;
    int i = 0;
    while(i < count){
        int len;
        char *name = field(pp, &len);
        pp = pp + len;
        char status = *pp;
        first = addUser(first, name, status);
        pp++;
        i++;
    }
    return first;
}

char *encode_Listusr(char *topic, int *len){
    int ploadLen = HEADER_LEN + 2 + strlen(topic);
    char *packet = malloc(ploadLen);
    char *pp = packet;
    *pp |= LISTUSR << 4;
    pp++;
    *pp = MSB(strlen(topic));
    pp++;
    *pp = LSB(strlen(topic));
    pp++;
    strcpy(pp, topic);
    *len = ploadLen;
    return packet;
}

char *decode_Listusr(char *packet){
    char *pp = packet;
    pp++;
    int len;
    return field(pp, &len);
}


char *encode_Creatp(struct CrTopicPkt *ctp, int *len){
    int ploadLen = 0;
    ploadLen = ploadLen + 2 + strlen(ctp->name);
    struct user *f = ctp->first;
    int count = 0;
    while (f != NULL){
        ploadLen = ploadLen + 2 + strlen(f->name);
        f = f->next;
        count++;
    }
    char *packet = malloc(2 + ploadLen);
    char *pp = packet;
    *pp = CREATP << 4;
    pp++;
    *pp = MSB(strlen(ctp->name));
    pp++;
    *pp = LSB(strlen(ctp->name));
    pp++;
    strcpy(pp, ctp->name);
    pp = pp + strlen(ctp->name);
    *pp = count;
    pp++;
    f = ctp->first;
    while (f != NULL){
        *pp = MSB(strlen(f->name));
        pp++;
        *pp = LSB(strlen(f->name));
        pp++;
        strcpy(pp, f->name);
        pp = pp + strlen(f->name);
        f = f->next;
    }
    *len = 2 + ploadLen;
    return packet;
}

struct CrTopicPkt *decode_Creatp(char *packet){
    struct CrTopicPkt *cr = malloc(sizeof(struct CrTopicPkt));
    char *pp = packet;
    pp++;
    int len;
    cr->name = field(pp, &len);
    pp = pp + len;

    struct user *u1 = NULL;
    int count = *pp;
    pp++;
    int i = 0;
    while (i < count){
        char *name = field(pp, &len);
        pp = pp + len;
        u1 = addUser(u1, name, 0);
        i++;
    }
    cr->first = u1;
    return cr;
}

char *encode_Creatpack(struct CrTopicAck* cr, int *len){
    char *packet = encodeListCode(cr->first, len);
    *packet = CREATPACK << 4;
    *packet |= cr->rc;
    return packet;
}


struct CrTopicAck *decode_Creatpack(char *packet){
    struct CrTopicAck *cr = malloc(sizeof(struct CrTopicAck));
    char *pp = packet;
    cr->rc = *pp & 1;
    if(cr->rc == 1){
        cr->first = decodeListCode(packet);
    }
    return cr;
}

char *encode_Subscribe(struct topic *first, int *len){
    char *packet = encodeList(first, len);
    *packet = SUBSCRIBE << 4;
    return packet;
}

struct topic *decode_Subscribe(char *packet){
    return decodeList(packet);
}

char *encode_Suback(struct code *first, int *len){
    char *packet = encodeListCode(first, len);
    *packet = SUBACK << 4;
    return packet;
}

struct code *decode_Suback(char *packet){
    return decodeListCode(packet);
}

char *encode_Unsubscribe(struct topic *first, int *len){
    char *packet = encodeList(first, len);
    *packet = UNSUBSCRIBE << 4;
    return packet;
}

struct topic *decode_Unsubscribe(char *packet){
    return decodeList(packet);
}

char *encode_Unsuback(struct code *first, int *len){
    char *packet = encodeListCode(first, len);
    *packet = UNSUBACK << 4;
    return packet;
}

struct code *decode_Unsuback(char *packet){
    return decodeListCode(packet);
}

char *encode_Disconnect(int *len){
    char *packet = malloc(2);
    packet[0] = DISCONECT << 4;
    *len = 1;
    return packet;
}

