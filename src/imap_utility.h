#ifndef IMAP_UTILITY_H
#define IMAP_UTILITY_H

#include "imap_client.h"

int imap_send(imap_client *client, const void *buffer, int size);
int imap_recv(imap_client *client, void *buffer, int size);
int split(const char* original_string, char** new_strings, const char* delimiter);
char* combine_strings(char** strings, int count);
void free_tokens(char** tokens, int num_tokens);
char* base64_encode(const char* input, int length);
int imap_read(imap_client* client, char **response, int* len, int command_number);


#endif