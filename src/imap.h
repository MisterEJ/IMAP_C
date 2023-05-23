#ifndef IMAP_H
#define IMAP_H

#include "imap_client.h"

void execute_command(char* command, int* run, imap_client* client);
void message(const char message[]);

#endif