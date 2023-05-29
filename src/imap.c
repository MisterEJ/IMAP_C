#include "imap.h"

#include <stdio.h>
#include <ctype.h>

#include "imap_utility.h"
#include "imap_client.h"

#define COMMAND_LENGTH 4000

int main(int argc, const char **argv)
{
    // Loading and creating the imap client
    if (argc < 2)
    {
        printf("USAGE: imap <config-file-name>");
        return EXIT_FAILURE;
    }

    imap_client *client = imap_construct(argv[1]);
    if(client == NULL)
    {
        printf("ERROR WHILE LOADING CONFIG FILE");
        return EXIT_FAILURE;
    }

    // Logging in
    message("Connecting to the server...");
    if (imap_connect(client) == 0)
    {
        char msg[1024];
        sprintf(msg, "Connected to %s", client->cfg->imap_server);
        message(msg);
    }
    else
    {
        message("Failed to connect to server");
        return EXIT_FAILURE;
    }

    message("Logging in");
    if (imap_login(client) == 0)
    {
        char msg[1024];
        sprintf(msg, "Logged In as %s", client->cfg->imap_username);
        message(msg);
    }
    else
    {
        message("Failed to Login");
        return EXIT_FAILURE;
    }

    // Enter main loop
    char command[COMMAND_LENGTH];
    int run = 1;
    while(run)
    {
        printf("> ");

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            printf("Error while reading command, exiting...");
            break;
        }

        command[strcspn(command, "\n")] = 0;

        if (strlen(command) != 0)
            execute_command(command, &run, client);
    }
    
    message("Disconnecting");
    imap_logout(client);

    return 0;
}

void message(const char message[])
{
    printf("[*] %s\n", message);
}

void execute_command(char* command, int* run, imap_client* client)
{
    char delimeter[] = " ";
    char *cmd, *extension;

    cmd = strtok(command, delimeter);
    extension = strtok(NULL, "");

    printf("EXTENSION: %s\n", extension);

    for(size_t i = 0; i < strlen(cmd); i++)
    {
        cmd[i] = tolower(cmd[i]);
    }

    if(strcmp(cmd, "quit") == 0)
    {
        *run = 0;
        message("Exiting...");
    } else
    if(strcmp(cmd, "noop") == 0)
    {
        imap_noop(client);
    } else
    if(strcmp(cmd, "capability") == 0)
    {
        imap_capability(client);
    } else
    if(strcmp(cmd, "list") == 0)
    {
        imap_list(client, extension);
    } else
    if(strcmp(cmd, "select") == 0)
    {
        imap_select(client, extension);
    } else
    if(strcmp(cmd, "search") == 0)
    {
        imap_search(client, extension);
    } else
    if(strcmp(cmd, "fetch") == 0)
    {
        if (strstr(extension, "-se") != NULL)
        {
            remove_substring(extension, " -se");
            printf("[*] Saving and extracting attachments of the email.\n");
            imap_fetch(client, extension, 1);
        }
        else
        {
            imap_fetch(client, extension, 0);
        }
    } else
    if(strcmp(cmd, "check") == 0)
    {
        imap_check(client);
    } else
    if(strcmp(cmd, "close") == 0)
    {
        imap_close(client);
    } else
    if(strcmp(cmd, "expunge") == 0)
    {
        imap_close(client);
    } else
    if(strcmp(cmd, "store") == 0)
    {
        imap_store(client, extension);
    } else
    if(strcmp(cmd, "copy") == 0)
    {
        imap_copy(client, extension);
    } else
    if(strcmp(cmd, "uid") == 0)
    {
        imap_uid(client, extension);
    } else
    if(strcmp(cmd, "examine") == 0)
    {
        imap_examine(client, extension);
    } else
    if(strcmp(cmd, "create") == 0)
    {
        imap_create(client, extension);
    } else
    if(strcmp(cmd, "delete") == 0)
    {
        imap_delete(client, extension);
    } else
    if(strcmp(cmd, "rename") == 0)
    {
        imap_rename(client, extension);
    } else
    if(strcmp(cmd, "subscribe") == 0)
    {
        imap_subscribe(client, extension);
    } else
    if(strcmp(cmd, "unsubscribe") == 0)
    {
        imap_unsubscribe(client, extension);
    } else
    if(strcmp(cmd, "status") == 0)
    {
        imap_status(client, extension);
    } else
    if(strcmp(cmd, "append") == 0)
    {
        imap_append(client, extension);
    } else
    if(strcmp(cmd, "lsub") == 0)
    {
        imap_lsub(client, extension);
    }
    else
    {
        message("Unknown command.");
    }
}