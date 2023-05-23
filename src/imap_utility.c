#include "imap_utility.h"

#include <sys/socket.h>

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




