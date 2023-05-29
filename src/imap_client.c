#include "imap_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "imap_utility.h"

#define BUFFER_SIZE 4096

typedef struct CommandResponsePair {
    int command_number;
    char* command;
    char* response;
    int response_length;
} CommandResponsePair;

// Free the CommandResponsePair
void freeCommandResponsePair(CommandResponsePair* pair) {
    free(pair->response);
}

CommandResponsePair create_command(char* command)
{
    static int command_number = 1;
    CommandResponsePair crp;
    crp.command = command;
    crp.response_length = 0;
    crp.response = NULL;
    crp.command_number = command_number;

    command_number += 1;
    return crp;
}

CommandResponsePair imap_send_command(imap_client* client, const char* command)
{
    char cmd_buffer[4096] = {0};
    CommandResponsePair crp = create_command(cmd_buffer);
    sprintf(cmd_buffer, "A%05d %s\r\n", crp.command_number, command);
    printf("[*] %s\n", cmd_buffer);

    imap_send(client, crp.command, strlen(crp.command));
    if (imap_read(client, &crp.response, &crp.response_length, crp.command_number) != 0)
    {
        crp.response_length = -1;
    }

    return crp;
}

void imap_send_command_(imap_client* client, CommandResponsePair* crp, int flag)
{
    char cmd_buffer[4096] = {0};
    sprintf(cmd_buffer, "A%05d %s\r\n", crp->command_number, crp->command);
    crp->command = cmd_buffer;

    imap_send(client, crp->command, strlen(crp->command));
    if (flag == 1)
    {
        if (imap_read(client, &crp->response, &crp->response_length, 0) != 0)
        {
            crp->response_length = -1;
        }
    } else
    {
        if (imap_read(client, &crp->response, &crp->response_length, crp->command_number) != 0)
        {
            crp->response_length = -1;
        }
    }
}

imap_client* imap_construct(const char *config_locaiton)
{
    imap_client *client = malloc(sizeof(imap_client));
    client->sockfd = -1;

    struct imap_config* cfg;

    cyaml_err_t err = cyaml_load_file(config_locaiton, &config, &top_schema, (cyaml_data_t **)&cfg, NULL);
    if (err != CYAML_OK){
        fprintf(stderr, "%s", cyaml_strerror(err));
        return NULL;
    }

    client->cfg = cfg;
    return client;
}

int imap_connect(imap_client* client)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Configure SSL
    if(*client->cfg->use_ssl)
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        client->ssl_ctx = SSL_CTX_new(TLS_client_method());
    }

    // Creating the socket
    server = gethostbyname(client->cfg->imap_server);
    if(server == NULL) return -3;
    struct in_addr addr = *((struct in_addr **) server->h_addr_list)[0];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sockfd == -1) return -1;

    // assign IP and PORT
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(*(client->cfg->imap_port));
    serv_addr.sin_addr = addr;

    // Connecting
    int err = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err != 0) return -2;

    // Create the SSL object and connect to socket
    if(*client->cfg->use_ssl)
    {
        client->ssl = SSL_new(client->ssl_ctx);
        SSL_set_fd(client->ssl, sockfd);
        SSL_connect(client->ssl);
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    err = imap_recv(client, buffer, BUFFER_SIZE-1);
    if (err <= 0) return -4;
    // Server greeting
    printf("%s", buffer);
    if (strncmp(buffer, "* OK", 4) != 0)
    {
        return -1;
    }


    // Get Capability
    CommandResponsePair capa = imap_send_command(client, "CAPABILITY");
    int num_tokens = 1;
    for (int i = 0; capa.response[i] != '\0'; i++) {
        if (capa.response[i] == ' ') {
            num_tokens++;
        }
    }

    char** strings = malloc(num_tokens * sizeof(char*));
    int count = split(capa.response, strings, " ");
    client->capability = strings;
    client->capability_length = count;
    freeCommandResponsePair(&capa);

    // Success
    client->sockfd = sockfd;
    return 0;
}

void imap_disconnnect(imap_client* client)
{
    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);
    SSL_CTX_free(client->ssl_ctx);

    close(client->sockfd);
    client->sockfd = -1;

    ERR_free_strings();
    EVP_cleanup();

    free_tokens(client->capability, client->capability_length);
}

int imap_login(imap_client* client)
{
    char* username = client->cfg->imap_username;
    char* password = client->cfg->imap_password;

    // Get the capability
    int canAuth = 0;
    for (int i = 0; i < client->capability_length; i++)
    {
        if (strcmp("AUTH=PLAIN", client->capability[i]))
        {
            canAuth = 1;
            break;
        }
    }

    if (canAuth == 0)
    {
        return -1;
    }

    char auth_string[BUFFER_SIZE];
    memset(auth_string, 0, BUFFER_SIZE);
    int len = sprintf(auth_string, "%s%c%s%c%s", username, '\0' , username, '\0', password);
    char* encoded = base64_encode(auth_string, len);

    CommandResponsePair auth1 = create_command("AUTHENTICATE PLAIN");
    imap_send_command_(client, &auth1, 1);
    if('+' == auth1.response[0])
    {
        free(auth1.response);
        auth1.response = NULL;
        len = sprintf(auth_string, "%s\r\n", encoded);
        imap_send(client, auth_string, len);
        if (imap_read(client, &auth1.response, &len, auth1.command_number) == 0)
        {
            printf("%s\n", auth1.response);
            if (strstr(auth1.response, " OK ") != NULL)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    freeCommandResponsePair(&auth1);

    return 0;
}

void imap_logout(imap_client* client)
{
    CommandResponsePair logout = imap_send_command(client, "LOGOUT");
    printf("%s", logout.response);
    imap_disconnnect(client);
}

void imap_list(imap_client* client, const char* query)
{
    CommandResponsePair crp;
    if (query == NULL)
    {
        crp = imap_send_command(client, "LIST \"\" \"*\"");
    } else
    {
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "LIST %s \"\" \"*\"", query);
        printf("%s\n", buffer);
        crp = imap_send_command(client, buffer);
    }

    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_capability(imap_client* client)
{
    CommandResponsePair cpr = imap_send_command(client, "CAPABILITY");
    printf("%s\n", cpr.response);
    freeCommandResponsePair(&cpr);
}

void imap_noop(imap_client* client)
{
    CommandResponsePair crp = imap_send_command(client, "NOOP");
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

int imap_select(imap_client* client, const char* mailbox)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "SELECT %s", mailbox);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
    return 0;
}

void imap_search(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "SEARCH %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_fetch(imap_client* client, const char* query, int flag)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "FETCH %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);

    if(flag == 1)
    {
        // Find the opening and closing braces
        char *start_brace = strstr(crp.response, "{") + 1;
        char *end_brace = strstr(crp.response, "}");

        if(start_brace == NULL || end_brace == NULL) return;

        // Calculate the size of the email body content
        size_t email_size = strtol(start_brace, &end_brace, 10);

        // Calculate the start position of the email body content
        char *start_content = strstr(crp.response, "\n") + 1;

        // Extract the email body content
        char email_body[email_size + 1];
        strncpy(email_body, start_content, email_size);
        email_body[email_size] = '\0'; // Null-terminate the strings

        char* len = strstr(query, " ");
        char name[32];
        strncpy(name, query, len - query);

        printf("[*] %s\n", name);
        extract_attachments(email_body, name);
    }
    else
    {
        printf("%s", crp.response);
    }

    freeCommandResponsePair(&crp);
}

void imap_check(imap_client* client)
{
    CommandResponsePair crp = imap_send_command(client, "CHECK");
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_close(imap_client* client)
{
    CommandResponsePair crp = imap_send_command(client, "CLOSE");
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_expunge(imap_client* client)
{
    CommandResponsePair crp = imap_send_command(client, "EXPUNGE");
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_store(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "STORE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_copy(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "COPY %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_uid(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "UID %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_examine(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "EXAMINE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_create(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "CREATE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_delete(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "DELETE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_rename(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "RENAME %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_subscribe(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "SUBSCRIBE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_unsubscribe(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "UNSUBSCRIBE %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_status(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "STATUS %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_append(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "APPEND %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

void imap_lsub(imap_client* client, const char* query)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "LSUB %s", query);
    CommandResponsePair crp = imap_send_command(client, buffer);
    printf("%s\n", crp.response);

    freeCommandResponsePair(&crp);
}

