#include "imap_utility.h"
#include <stdio.h>

#include <sys/socket.h>
#include <sys/stat.h>

#define BUFFER_SIZE 8096

int imap_send(imap_client *client, const void *buffer, int size)
{
    if(*client->cfg->use_ssl)
    { // With SSL
        return SSL_write(client->ssl, buffer, size);
    } else
    { // No SSL
        return send(client->sockfd, buffer, size, 0);
    }
}

int imap_recv(imap_client *client, void *buffer, int size)
{
    if(*client->cfg->use_ssl)
    { // With SLL
        return SSL_read(client->ssl, buffer, size);
    } else
    { // No SSL
        return recv(client->sockfd, buffer, size, 0);
    }
}

int split(const char* original_string, char** new_strings, const char* delimiter)
{
    char* string_copy = malloc(strlen(original_string) + 1);  // Allocate memory for string copy
    strcpy(string_copy, original_string);  // Create a copy of the original string
    int num_strings = 0;

    char* token = strtok(string_copy, delimiter);
    while (token != NULL) {
        new_strings[num_strings] = malloc(strlen(token) + 1);  // Allocate memory for token
        strcpy(new_strings[num_strings], token);
        num_strings++;
        token = strtok(NULL, delimiter);
    }

    free(string_copy);  // Free the memory allocated for string copy

    return num_strings;
}

char* combine_strings(char** strings, int count) {
    int total_length = 0;

    // Calculate the total length of the combined string
    for (int i = 0; i < count; i++) {
        total_length += strlen(strings[i]);
    }

    // Allocate memory for the combined string
    char* combined = malloc(total_length + 1); // +1 for null terminator
    if (combined == NULL) {
        // Handle memory allocation error
        return NULL;
    }

    // Copy each string into the combined string
    combined[0] = '\0'; // Initialize the combined string as an empty string
    for (int i = 0; i < count; i++) {
        strcat(combined, strings[i]);
        if (i < count - 1) {
            strcat(combined, " "); // Add a space between strings
        }
    }

    return combined;
}

void free_tokens(char** tokens, int num_tokens) {
    if (tokens == NULL) {
        return;
    }

    // Free each individual string
    for (int i = 0; i < num_tokens; i++) {
        free(tokens[i]);
    }

    // Free the array itself
    free(tokens);
}

char* base64_encode(const char* input, int length) {
    BIO* bmem = NULL;
    BIO* b64 = NULL;
    BUF_MEM* bptr = NULL;
    char* output = NULL;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    output = (char*)malloc(bptr->length + 1);  // Add 1 for null terminator
    memcpy(output, bptr->data, bptr->length);
    output[bptr->length] = '\0';  // Null terminate the output string

    BIO_free_all(b64);

    return output;
}

int imap_read(imap_client* client, char **response, int* len, int command_number) {
    const char* CRLF = "\r\n";
    const int CRLF_LENGTH = strlen(CRLF);

    char* buffer = NULL;
    int bufferSize = BUFFER_SIZE;
    int bytesRead = 0;
    int foundCRLF = 0;

    buffer = malloc(bufferSize);
    if (buffer == NULL) {
        // Handle memory allocation error
        return -1;
    }
    memset(buffer, 0, bufferSize);

    // Convert the command number to string with formatting "A%05d"
    char expectedCommandNumber[7]; // 6 digits + null terminator
    sprintf(expectedCommandNumber, "A%05d", command_number);

    if (*response != NULL) {
        // Free *response if it was previously allocated
        free(*response);
        *response = NULL;
    }
    // Reset the size of the buffer
    *len = 0;

    while (foundCRLF == 0) {
        memset(buffer, 0, bufferSize);
        bytesRead = imap_recv(client, buffer, bufferSize - 1);

        // Resize the response buffer if needed
        int requiredSize = *len + bytesRead + 1; // Add 1 for null terminator
        char* newResponse = (char*) malloc(requiredSize * sizeof(char));
        if (newResponse == NULL) {
            // Handle memory allocation error
            free(buffer);
            return -1;
        }

        // Initialize newResponse to avoid junk values
        memset(newResponse, 0, requiredSize);

        // Copy the old data into the new buffer
        if (*response != NULL) {
            memcpy(newResponse, *response, *len);
            free(*response); // Don't forget to free the old response memory
        }
        *response = newResponse;

        // Append the received data to the response buffer
        memcpy(*response + (*len), buffer, bytesRead);
        (*len) += bytesRead;

        // Check if the CRLF sequence is present at the end of the response buffer
        if (*len >= CRLF_LENGTH && memcmp(*response + (*len) - CRLF_LENGTH, CRLF, CRLF_LENGTH) == 0) {
            // Check if the response contains the expected command number
            if ((strstr(*response, expectedCommandNumber) != NULL) || command_number == 0) {
                foundCRLF = 1;
            }
        }

        // // Debug: Print the ASCII value of each character in the received data
        // printf("Received data: ");
        // for (int i = 0; i < bytesRead; i++) {
        //     printf("%c", buffer[i]);
        // }
    }

    (*response)[*len] = '\0';

    free(buffer);

    return 0;
}

void parse_part(char *part, document **documents, int *documents_count) {
    char *boundary;
    char *subpart;
    char *next_subpart;
    char *content_type_start;
    char *content_type_end;

    // Find the boundary string in the Content-Type header
    boundary = strstr(part, "boundary=");
    if (boundary != NULL) {
        boundary += 9;  // Skip past "boundary="
        boundary = strtok(boundary, "\r\n");  // Get the boundary string

        // Find the first subpart
        subpart = strstr(part, boundary);
        if (subpart != NULL) {
            subpart += strlen(boundary) + 2;  // Skip past boundary and newline

            // Loop over subparts
            while ((next_subpart = strstr(subpart, boundary)) != NULL) {
                // Allocate memory for the document
                *documents = realloc(*documents, (*documents_count + 1) * sizeof(document));
                (*documents)[*documents_count].type = NULL;
                (*documents)[*documents_count].name = NULL;
                (*documents)[*documents_count].body = NULL;

                // Find the Content-Type header
                content_type_start = strstr(subpart, "Content-Type: ");
                if (content_type_start != NULL) {
                    content_type_start += 14;  // Skip past "Content-Type: "
                    content_type_end = strchr(content_type_start, '\n');
                    if (content_type_end != NULL) {
                        // Allocate memory for the type and copy the Content-Type into it
                        (*documents)[*documents_count].type = malloc(content_type_end - content_type_start + 1);
                        strncpy((*documents)[*documents_count].type, content_type_start, content_type_end - content_type_start);
                        (*documents)[*documents_count].type[content_type_end - content_type_start] = '\0';
                    }
                }

                // Allocate memory for the body and copy the subpart into it
                (*documents)[*documents_count].body = malloc(next_subpart - subpart + 1);
                strncpy((*documents)[*documents_count].body, subpart, next_subpart - subpart);
                (*documents)[*documents_count].body[next_subpart - subpart] = '\0';

                (*documents_count)++;

                subpart = next_subpart + strlen(boundary) + 2;  // Skip past boundary and newline
            }
        }
    } else {
        // This part is not multipart, so just add it as a document
        // Allocate memory for the document
        *documents = realloc(*documents, (*documents_count + 1) * sizeof(document));
        (*documents)[*documents_count].type = NULL;
        (*documents)[*documents_count].name = NULL;
        (*documents)[*documents_count].body = strdup(part);

        (*documents_count)++;
    }
}

int extract_attachments(char* email, char* name)
{
    // Calling pyhton tool to extract attachments from save email

    struct stat st = {0};
    if (stat("./emails", &st) == -1) {
        mkdir("./emails", 0700);
    }

    char buffer[1024];
    char nameb[128];
    sprintf(nameb, "./emails/%s.email.raw", name);
    sprintf(buffer, "python parse_email.py %s > output.txt", nameb);
    FILE *fd = fopen(nameb, "w");
    fprintf(fd, "%s", email);
    fclose(fd);

    system(buffer);
    
    char output[1024];
    FILE *file = fopen("output.txt", "r");
    if (file != NULL) {
        while (fgets(output, sizeof(output), file) != NULL) {
            printf("%s", output);
        }
        fclose(file);
    }

    return 0;
}

void remove_substring(char* str, const char* unwanted) {
    int unwanted_len = strlen(unwanted);
    char* occurrence = strstr(str, unwanted);

    while (occurrence != NULL) {
        memmove(occurrence, occurrence + unwanted_len, strlen(occurrence + unwanted_len) + 1);
        occurrence = strstr(str, unwanted);
    }
}


