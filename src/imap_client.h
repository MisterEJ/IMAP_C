/*
https://www.rfc-editor.org/rfc/rfc3501
*/

#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "yaml_conf.h"

typedef struct imap_client
{
    struct imap_config *cfg;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    int sockfd;
    char** capability;
    int capability_length;
} imap_client;

typedef struct document
{
    char* type;
    char* name;
    char* body;
} document;

imap_client* imap_construct(const char *config_location);


/*
Connect to the IMAP Server
Errors:
0  : Success
-1 : Failed to create socker
-2 : Failed to connect
-3 : Server not found
-4 : SSL Error
*/
int  imap_connect(imap_client* client);
void imap_disconnnect(imap_client* client);

// Authentication
int  imap_login(imap_client* client);
void imap_logout(imap_client* client);

//
void imap_noop(imap_client* client);
void imap_capability(imap_client* client);

void imap_list(imap_client* client, const char* query);
void imap_lsub(imap_client* client, const char* query);
int  imap_select(imap_client* client, const char* mailboxt);
void imap_examine(imap_client* client, const char* query);
void imap_create(imap_client* client, const char* query);
void imap_delete(imap_client* client, const char* query);
void imap_rename(imap_client* client, const char* query);
void imap_subscribe(imap_client* client, const char* query);
void imap_unsubscribe(imap_client* client, const char* query);
void imap_status(imap_client* client, const char* query);
void imap_append(imap_client* client, const char* query);

void imap_search(imap_client* client, const char* query);
void imap_fetch(imap_client* client, const char* query, int flag);
void imap_check(imap_client* client);
void imap_close(imap_client* client);
void imap_expunge(imap_client* client);
void imap_store(imap_client* client, const char* query);
void imap_copy(imap_client* client, const char* query);
void imap_uid(imap_client* client, const char* query);



#endif