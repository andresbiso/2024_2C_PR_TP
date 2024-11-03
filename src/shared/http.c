// Standard library headers
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Networking headers
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

// System headers
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Shared headers
#include "common.h"

// Project header
#include "http.h"

Header *create_headers(int initial_count)
{
    Header *headers = (Header *)malloc(initial_count * sizeof(Header));
    if (headers == NULL)
    {
        fprintf(stderr, "Error allocating memory for headers\n");
        return NULL;
    }
    return headers;
}

void free_headers(Header *headers, int header_count)
{
    for (int i = 0; i < header_count; i++)
    {
        free(headers[i].key);
        free(headers[i].value);
    }
    free(headers);
}

int add_header(Header **headers, int *header_count, const char *key, const char *value)
{
    *headers = (Header *)realloc(*headers, (*header_count + 1) * sizeof(Header));
    if (*headers == NULL)
    {
        fprintf(stderr, "Error reallocating memory\n");
        return -1;
    }

    (*headers)[*header_count].key = (char *)malloc(strlen(key) + 1);
    if ((*headers)[*header_count].key == NULL)
    {
        fprintf(stderr, "Error allocating memory for key\n");
        return -1;
    }
    strcpy((*headers)[*header_count].key, key);

    (*headers)[*header_count].value = (char *)malloc(strlen(value) + 1);
    if ((*headers)[*header_count].value == NULL)
    {
        fprintf(stderr, "Error allocating memory for value\n");
        free((*headers)[*header_count].key);
        return -1;
    }
    strcpy((*headers)[*header_count].value, value);

    (*header_count)++;
    return 0;
}

int remove_header(Header **headers, int *header_count, const char *key)
{
    for (int i = 0; i < *header_count; i++)
    {
        if (strcmp((*headers)[i].key, key) == 0)
        {
            free((*headers)[i].key);
            free((*headers)[i].value);

            // Shift remaining headers left
            for (int j = i; j < *header_count - 1; j++)
            {
                (*headers)[j] = (*headers)[j + 1];
            }

            (*header_count)--;
            *headers = (Header *)realloc(*headers, (*header_count) * sizeof(Header));
            if (*headers == NULL && *header_count > 0)
            {
                fprintf(stderr, "Error reallocating memory\n");
                return -1;
            }

            return 0; // Success
        }
    }
    return -1; // Header not found
}

int serialize_headers(const Header *headers, int header_count, char **buffer)
{
    int size = 0;

    // Calculate the total size needed for the buffer
    for (int i = 0; i < header_count; i++)
    {
        size += strlen(headers[i].key) + strlen(headers[i].value) + 4; // For ': ' and '\r\n'
    }

    *buffer = (char *)malloc((size + 1) * sizeof(char)); // +1 for the null terminator
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error allocating memory for buffer\n");
        return -1;
    }

    (*buffer)[0] = '\0'; // Initialize the buffer

    // Construct the headers string
    for (int i = 0; i < header_count; i++)
    {
        strcat(*buffer, headers[i].key);
        strcat(*buffer, ": ");
        strcat(*buffer, headers[i].value);
        strcat(*buffer, "\r\n");
    }

    return size; // Return the size of the serialized headers
}

Header *deserialize_headers(const char *headers_str, int *header_count)
{
    int count = 0;
    char *headers_copy = (char *)malloc(strlen(headers_str) + 1);
    strcpy(headers_copy, headers_str);

    char *line = strtok(headers_copy, "\r\n");

    while (line != NULL)
    {
        count++;
        line = strtok(NULL, "\r\n");
    }

    free(headers_copy);
    headers_copy = (char *)malloc(strlen(headers_str) + 1);
    strcpy(headers_copy, headers_str);

    Header *headers = (Header *)malloc(count * sizeof(Header));
    if (headers == NULL)
    {
        fprintf(stderr, "Error allocating memory\n");
        free(headers_copy);
        return NULL;
    }

    line = strtok(headers_copy, "\r\n");
    for (int i = 0; i < count; i++)
    {
        char *colon = strchr(line, ':');
        if (colon != NULL)
        {
            *colon = '\0';
            headers[i].key = (char *)malloc(strlen(line) + 1);
            strcpy(headers[i].key, line);
            headers[i].value = (char *)malloc(strlen(colon + 2) + 1);
            strcpy(headers[i].value, colon + 2);
        }
        line = strtok(NULL, "\r\n");
    }

    free(headers_copy);
    *header_count = count;
    return headers;
}

const char *find_header_value(Header *headers, int header_count, const char *key)
{
    for (int i = 0; i < header_count; i++)
    {
        if (strcmp(headers[i].key, key) == 0)
        {
            return headers[i].value;
        }
    }
    return NULL; // Return NULL if the header is not found
}

void log_headers(Header *headers, int header_count)
{
    for (int i = 0; i < header_count; ++i)
    {
        printf("%s: %s\n", headers[i].key, headers[i].value);
    }
}

HTTP_Request *create_http_request(const char *method, const char *uri, const char *version, const Header *headers, int header_count, const char *body)
{
    HTTP_Request *packet = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (packet == NULL)
    {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        return NULL;
    }

    packet->request_line.method = (char *)malloc(strlen(method) + 1);
    strcpy(packet->request_line.method, method);

    packet->request_line.uri = (char *)malloc(strlen(uri) + 1);
    strcpy(packet->request_line.uri, uri);

    packet->request_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(packet->request_line.version, version);

    packet->headers = (Header *)malloc(header_count * sizeof(Header));
    if (packet->headers == NULL)
    {
        fprintf(stderr, "Error allocating memory for headers: %s\n", strerror(errno));
        free(packet->request_line.method);
        free(packet->request_line.uri);
        free(packet->request_line.version);
        free(packet);
        return NULL;
    }

    for (int i = 0; i < header_count; i++)
    {
        packet->headers[i].key = (char *)malloc(strlen(headers[i].key) + 1);
        strcpy(packet->headers[i].key, headers[i].key);

        packet->headers[i].value = (char *)malloc(strlen(headers[i].value) + 1);
        strcpy(packet->headers[i].value, headers[i].value);
    }
    packet->header_count = header_count;

    packet->body = (char *)malloc(strlen(body) + 1);
    strcpy(packet->body, body);

    return packet;
}

void free_http_request(HTTP_Request *packet)
{
    if (packet != NULL)
    {
        free(packet->request_line.method);
        free(packet->request_line.uri);
        free(packet->request_line.version);
        free_headers(packet->headers, packet->header_count);
        free(packet->body);
        free(packet);
    }
}

int serialize_http_request(HTTP_Request *packet, char **buffer)
{
    int size = strlen(packet->request_line.method) + strlen(packet->request_line.uri) + strlen(packet->request_line.version) + 4; // For spaces and \r\n
    for (int i = 0; i < packet->header_count; i++)
    {
        size += strlen(packet->headers[i].key) + strlen(packet->headers[i].value) + 4; // For ': ' and \r\n
    }
    size += 4; // For \r\n\r\n after headers

    *buffer = (char *)malloc(size * sizeof(char));
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error allocating memory\n");
        return -1;
    }

    sprintf(*buffer, "%s %s %s\r\n", packet->request_line.method, packet->request_line.uri, packet->request_line.version);
    for (int i = 0; i < packet->header_count; i++)
    {
        strcat(*buffer, packet->headers[i].key);
        strcat(*buffer, ": ");
        strcat(*buffer, packet->headers[i].value);
        strcat(*buffer, "\r\n");
    }
    strcat(*buffer, "\r\n");

    return size;
}

HTTP_Request *deserialize_http_request(const char *buffer)
{
    HTTP_Request *packet = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (packet == NULL)
    {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }

    char *temp_buffer = strdup(buffer);
    char *line = strtok(temp_buffer, "\r\n");
    char method[16], uri[256], version[16];
    sscanf(line, "%s %s %s", method, uri, version);

    packet->request_line.method = (char *)malloc(strlen(method) + 1);
    strcpy(packet->request_line.method, method);

    packet->request_line.uri = (char *)malloc(strlen(uri) + 1);
    strcpy(packet->request_line.uri, uri);

    packet->request_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(packet->request_line.version, version);

    packet->header_count = 0;
    packet->headers = NULL;
    line = strtok(NULL, "\r\n");
    while (line != NULL && line[0] != '\0')
    {
        add_header(&(packet->headers), &(packet->header_count), strtok(line, ": "), strtok(NULL, ""));
        line = strtok(NULL, "\r\n");
    }

    packet->body = strdup(strtok(NULL, ""));

    free(temp_buffer);
    return packet;
}

int send_http_request(int sockfd, HTTP_Request *packet)
{
    char *buffer;
    int size = serialize_http_request(packet, &buffer);
    if (size < 0)
    {
        return -1;
    }
    if (send(sockfd, buffer, size, 0) < 0)
    {
        perror("send headers");
        free(buffer);
        return -1;
    }
    free(buffer);

    if (packet->body != NULL)
    {
        if (send(sockfd, packet->body, strlen(packet->body), 0) < 0)
        {
            perror("send body");
            return -1;
        }
    }
    return 0;
}

HTTP_Request *receive_http_request(int sockfd)
{
    char buffer[1024];
    int size;
    HTTP_Request *packet;

    size = recv(sockfd, buffer, sizeof(buffer), 0);
    if (size < 0)
    {
        perror("recv headers");
        return NULL;
    }
    buffer[size] = '\0'; // Null-terminate the received data
    packet = deserialize_http_request(buffer);

    packet->body = (char *)malloc(1024 * sizeof(char));
    if (packet->body == NULL)
    {
        fprintf(stderr, "Error allocating memory for body\n");
        free_http_request(packet);
        return NULL;
    }
    size = recv(sockfd, packet->body, 1024, 0); // Adjust size as needed
    if (size < 0)
    {
        perror("recv body");
        free_http_request(packet);
        return NULL;
    }
    packet->body[size] = '\0';

    return packet;
}

HTTP_Response *create_http_response(const char *version, int status_code, const char *reason_phrase, const Header *headers, int header_count, const char *body)
{
    HTTP_Response *response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        return NULL;
    }

    response->response_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;
    response->response_line.reason_phrase = (char *)malloc(strlen(reason_phrase) + 1);
    strcpy(response->response_line.reason_phrase, reason_phrase);

    response->headers = (Header *)malloc(header_count * sizeof(Header));
    if (response->headers == NULL)
    {
        fprintf(stderr, "Error allocating memory for headers: %s\n", strerror(errno));
        free(response->response_line.version);
        free(response->response_line.reason_phrase);
        free(response);
        return NULL;
    }

    for (int i = 0; i < header_count; i++)
    {
        response->headers[i].key = (char *)malloc(strlen(headers[i].key) + 1);
        strcpy(response->headers[i].key, headers[i].key);

        response->headers[i].value = (char *)malloc(strlen(headers[i].value) + 1);
        strcpy(response->headers[i].value, headers[i].value);
    }
    response->header_count = header_count;

    response->body = (char *)malloc(strlen(body) + 1);
    strcpy(response->body, body);

    return response;
}

void free_http_response(HTTP_Response *response)
{
    if (response != NULL)
    {
        free(response->response_line.version);
        free(response->response_line.reason_phrase);
        free_headers(response->headers, response->header_count);
        free(response->body);
        free(response);
    }
}

int serialize_http_response(HTTP_Response *response, char **buffer)
{
    int size = strlen(response->response_line.version) + strlen(response->response_line.reason_phrase) + 4 + 12; // For separators and status code
    for (int i = 0; i < response->header_count; i++)
    {
        size += strlen(response->headers[i].key) + strlen(response->headers[i].value) + 4; // For ': ' and '\r\n'
    }
    size += 4; // For '\r\n\r\n' after headers

    *buffer = (char *)malloc(size * sizeof(char));
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error allocating memory\n");
        return -1;
    }
    sprintf(*buffer, "%s %d %s\r\n", response->response_line.version, response->response_line.status_code, response->response_line.reason_phrase);
    for (int i = 0; i < response->header_count; i++)
    {
        strcat(*buffer, response->headers[i].key);
        strcat(*buffer, ": ");
        strcat(*buffer, response->headers[i].value);
        strcat(*buffer, "\r\n");
    }
    strcat(*buffer, "\r\n");

    return size;
}

HTTP_Response *deserialize_http_response(const char *buffer)
{
    HTTP_Response *response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }

    char *temp_buffer = (char *)malloc(strlen(buffer) + 1);
    strcpy(temp_buffer, buffer);

    char *line = strtok(temp_buffer, "\r\n");
    char version[16], reason_phrase[256];
    int status_code;
    sscanf(line, "%s %d %255[^\r\n]", version, &status_code, reason_phrase);

    response->response_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;

    response->response_line.reason_phrase = (char *)malloc(strlen(reason_phrase) + 1);
    strcpy(response->response_line.reason_phrase, reason_phrase);

    response->header_count = 0;
    response->headers = NULL;
    line = strtok(NULL, "\r\n");
    while (line != NULL && line[0] != '\0')
    {
        add_header(&(response->headers), &(response->header_count), strtok(line, ": "), strtok(NULL, ""));
        line = strtok(NULL, "\r\n");
    }

    char *body = strtok(NULL, "");
    response->body = (char *)malloc(strlen(body) + 1);
    strcpy(response->body, body);

    free(temp_buffer);
    return response;
}

int send_http_response(int sockfd, HTTP_Response *response)
{
    char *buffer;
    int size = serialize_http_response(response, &buffer);
    if (size < 0)
    {
        return -1;
    }
    if (send(sockfd, buffer, size, 0) < 0)
    {
        perror("send headers");
        free(buffer);
        return -1;
    }
    free(buffer);

    if (response->body != NULL)
    {
        if (send(sockfd, response->body, strlen(response->body), 0) < 0)
        {
            perror("send body");
            return -1;
        }
    }
    return 0;
}

HTTP_Response *receive_http_response_headers(int sockfd)
{
    char buffer[DEFAULT_BUFFER_SIZE];
    int size;
    HTTP_Response *response;

    size = recv(sockfd, buffer, sizeof(buffer), 0);
    if (size < 0)
    {
        perror("recv headers");
        return NULL;
    }
    buffer[size] = '\0'; // Null-terminate the received data
    response = deserialize_http_response(buffer);

    if (response == NULL)
    {
        fprintf(stderr, "Error deserializing HTTP response\n");
        return NULL;
    }

    return response;
}

int receive_http_response_body(int sockfd, HTTP_Response *response)
{
    const char *content_length_str = find_header_value(response->headers, response->header_count, "Content-Length");
    if (content_length_str == NULL)
    {
        fprintf(stderr, "Content-Length header not found\n");
        return -1;
    }

    int content_length = atoi(content_length_str);
    response->body = (char *)malloc(content_length + 1);
    if (response->body == NULL)
    {
        fprintf(stderr, "Error allocating memory for body\n");
        return -1;
    }

    int total_received = 0;
    while (total_received < content_length)
    {
        int received = recv(sockfd, response->body + total_received, content_length - total_received, 0);
        if (received < 0)
        {
            perror("recv body");
            free(response->body);
            response->body = NULL;
            return -1;
        }
        total_received += received;
    }
    response->body[content_length] = '\0';

    return 0;
}

HTTP_Response *receive_http_response(int sockfd)
{
    HTTP_Response *response = receive_http_headers(sockfd);
    if (response == NULL)
    {
        return NULL;
    }

    if (receive_http_body(sockfd, response) < 0)
    {
        free_http_response(response);
        return NULL;
    }

    return response;
}

const char *get_extension(const char *content_type)
{
    if (strstr(content_type, "image/jpeg") != NULL)
    {
        return ".jpg";
    }
    else if (strstr(content_type, "image/png") != NULL)
    {
        return ".png";
    }
    else if (strstr(content_type, "image/gif") != NULL)
    {
        return ".gif";
    }
    else if (strstr(content_type, "image/bmp") != NULL)
    {
        return ".bmp";
    }
    else if (strstr(content_type, "image/tiff") != NULL)
    {
        return ".tiff";
    }
    else if (strstr(content_type, "text/plain") != NULL)
    {
        return ".txt";
    }
    else if (strstr(content_type, "application/pdf") != NULL)
    {
        return ".pdf";
    }
    else
    {
        return NULL;
    }
}