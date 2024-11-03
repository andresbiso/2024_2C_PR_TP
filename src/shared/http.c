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

// Other project headers
#include "common.h"

// Project header
#include "http.h"

Header *create_headers(int initial_count)
{
    Header *headers;
    headers = (Header *)malloc(initial_count * sizeof(Header));
    if (headers == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers\n");
        return NULL;
    }
    memset(headers, 0, initial_count * sizeof(Header)); // Initialize memory to zero
    return headers;
}

void free_headers(Header **headers, int header_count)
{
    for (int i = 0; i < header_count; i++)
    {
        free_header(headers[i]);
        headers[i] = NULL; // Set each element to NULL
    }
    free(*headers);
    *headers = NULL; // Set the original pointer to NULL
}

void free_header(Header *header)
{
    if (header != NULL)
    {
        free(header->key);
        free(header->value);
    }
}

int add_header(Header **headers, int *header_index, int *header_count, const char *key, const char *value)
{
    if (*header_index >= *header_count)
    {
        *header_count *= 2;
        *headers = (Header *)realloc(*headers, (*header_count) * sizeof(Header));
        if (*headers == NULL)
        {
            fprintf(stderr, "Error reasignando memoria\n");
            return -1;
        }
    }

    (*headers)[*header_index].key = (char *)malloc(strlen(key) + 1);
    if ((*headers)[*header_index].key == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para key\n");
        return -1;
    }
    strcpy((*headers)[*header_index].key, key);

    (*headers)[*header_index].value = (char *)malloc(strlen(value) + 1);
    if ((*headers)[*header_index].value == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para value\n");
        free((*headers)[*header_index].key);
        return -1;
    }
    strcpy((*headers)[*header_index].value, value);

    (*header_index)++;
    return 0;
}

int remove_header(Header **headers, int *header_index, int *header_count, const char *key)
{
    int i, j;
    Header *temp;
    for (i = 0; i < *header_index; i++)
    {
        if (strcmp((*headers)[i].key, key) == 0)
        {
            free_header(&(*headers)[i]);

            // Shift remaining headers left
            for (j = i; j < *header_index - 1; j++)
            {
                (*headers)[j] = (*headers)[j + 1];
            }

            (*header_index)--;

            if (*header_index > 0 && *header_index < *header_count / 2)
            {
                *header_count /= 2;
                temp = (Header *)realloc(*headers, (*header_count) * sizeof(Header));
                if (temp == NULL)
                {
                    fprintf(stderr, "Error reasignando memoria\n");
                    return -1;
                }
                *headers = temp;
            }

            return 0; // Success
        }
    }
    return -1; // Header not found
}

int serialize_headers(const Header *headers, int header_count, char **buffer)
{
    const char *key_value_separator;
    const char *line_ending;
    int i, extra_char_size, size;

    key_value_separator = ": ";
    line_ending = "\r\n";
    size = 0;

    // Calculate the total size needed for the buffer
    for (i = 0; i < header_count; i++)
    {
        size += strlen(headers[i].key) + strlen(headers[i].value) + strlen(key_value_separator) + strlen(line_ending);
    }

    *buffer = (char *)malloc((size + 1) * sizeof(char)); // +1 for the null terminator
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para buffer\n");
        return -1;
    }

    (*buffer)[0] = '\0'; // Initialize the buffer

    // Construct the headers string
    for (i = 0; i < header_count; i++)
    {
        // Concat values
        strcat(*buffer, headers[i].key);
        strcat(*buffer, key_value_separator);
        strcat(*buffer, headers[i].value);
        strcat(*buffer, line_ending);
    }

    return size; // Return the size of the serialized headers
}

Header *deserialize_headers(const char *headers_str, int *header_count)
{
    int count;
    const char *temp_str;

    count = 0;
    temp_str = headers_str;

    while (*temp_str)
    {
        if (*temp_str == '\r' || *temp_str == '\n')
        {
            count++;
            while (*temp_str == '\r' || *temp_str == '\n')
                temp_str++;
        }
        else
        {
            temp_str++;
        }
    }

    Header *headers = (Header *)malloc(count * sizeof(Header));
    if (headers == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return NULL;
    }

    char *headers_copy = (char *)malloc(strlen(headers_str) + 1);
    strcpy(headers_copy, headers_str);

    char *line = strtok(headers_copy, "\r\n"); // Tokenize the headers_copy string, separating by "\r\n"
    for (int i = 0; i < count; i++)
    {
        // Finds the first occurrence of ':' in the current line
        char *colon = strchr(line, ':');
        if (colon != NULL)
        {
            *colon = '\0';                                     // Replace ':' with '\0' to terminate the key string
            headers[i].key = (char *)malloc(strlen(line) + 1); // Allocate memory for the key
            strcpy(headers[i].key, line);                      // Copy the key to headers[i].key

            // colon + 2 points directly to the start of the value string
            headers[i].value = (char *)malloc(strlen(colon + 2) + 1); // Allocate memory for the value
            strcpy(headers[i].value, colon + 2);                      // Copy the value to headers[i].value
        }
        line = strtok(NULL, "\r\n"); // Continue tokenizing the rest of the headers_copy string
    }
    free(headers_copy);
    *header_count = count;
    return headers;
}

const char *find_header_value(Header *headers, int header_count, const char *key)
{
    int i;
    for (i = 0; i < header_count; i++)
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
    int i;
    for (i = 0; i < header_count; ++i)
    {
        printf("%s: %s\n", headers[i].key, headers[i].value);
    }
}

HTTP_Request *create_http_request(const char *method, const char *uri, const char *version, const Header *headers, int header_count, const char *body)
{
    int header_index, i;
    HTTP_Request *request;

    if (method == NULL || uri == NULL || version == NULL || headers == NULL || header_count < 0 || body == NULL)
    {
        puts("Uno o más valores de creación de HTTP Request es inválido\n");
        return NULL;
    }

    request = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (request == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    request->request_line.method = (char *)malloc(strlen(method) + 1);
    strcpy(request->request_line.method, method);

    request->request_line.uri = (char *)malloc(strlen(uri) + 1);
    strcpy(request->request_line.uri, uri);

    request->request_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(request->request_line.version, version);

    request->headers = create_headers(header_count);
    if (request->headers == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers: %s\n", strerror(errno));
        free(request->request_line.method);
        free(request->request_line.uri);
        free(request->request_line.version);
        free(request);
        return NULL;
    }

    request->header_count = header_count;
    header_index = 0;

    for (i = 0; i < header_count; i++)
    {
        if (add_header(&(request->headers), &header_index, &(request->header_count), headers[i].key, headers[i].value) != 0)
        {
            fprintf(stderr, "Error al agregar header\n");
            free(request->request_line.method);
            free(request->request_line.uri);
            free(request->request_line.version);
            free_headers(&(request->headers), request->header_count);
            free(request);
            return NULL; // Return NULL to indicate failure
        }
    }

    request->body = (char *)malloc(strlen(body) + 1);
    strcpy(request->body, body);

    return request;
}

void free_http_request(HTTP_Request *request)
{
    if (request != NULL)
    {
        if (request->request_line.method != NULL)
        {
            free(request->request_line.method);
        }
        if (request->request_line.uri != NULL)
        {
            free(request->request_line.uri);
        }
        if (request->request_line.version != NULL)
        {
            free(request->request_line.version);
        }
        free_headers(request->headers, request->header_count);
        if (request->body != NULL)
        {
            free(request->body);
        }
        free(request);
    }
}

int serialize_http_request(HTTP_Request *request, char **buffer)
{
    const char *key_value_separator;
    const char *line_ending;
    const char *space;
    int i, extra_char_size, size;

    key_value_separator = ": ";
    line_ending = "\r\n";
    space = " ";

    size = strlen(request->request_line.method) + strlen(request->request_line.uri) + strlen(request->request_line.version) + strlen(space) * 2 + strlen(line_ending);
    for (i = 0; i < request->header_count; i++)
    {
        size += strlen(request->headers[i].key) + strlen(request->headers[i].value) + strlen(key_value_separator) + strlen(line_ending);
    }
    size += strlen(line_ending); // For \r\n after headers

    *buffer = (char *)malloc(size * sizeof(char));
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    sprintf(*buffer, "%s %s %s%s", request->request_line.method, request->request_line.uri, request->request_line.version, line_ending);
    for (i = 0; i < request->header_count; i++)
    {
        strcat(*buffer, request->headers[i].key);
        strcat(*buffer, key_value_separator);
        strcat(*buffer, request->headers[i].value);
        strcat(*buffer, line_ending);
    }
    strcat(*buffer, line_ending);

    return size;
}

HTTP_Request *deserialize_http_request(const char *buffer)
{
    char method[METHOD_SIZE], uri[URI_SIZE], version[VERSION_SIZE];
    char *body_start, *key, *line, *temp_buffer, *value;
    int header_index;
    HTTP_Request *request;

    request = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (request == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return NULL;
    }

    temp_buffer = (char *)malloc(strlen(buffer) + 1);
    if (temp_buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        free(request);
        return NULL;
    }
    strcpy(temp_buffer, buffer);

    line = strtok(temp_buffer, "\r\n");
    sscanf(line, "%s %s %s", method, uri, version);

    request->request_line.method = (char *)malloc(strlen(method) + 1);
    strcpy(request->request_line.method, method);

    request->request_line.uri = (char *)malloc(strlen(uri) + 1);
    strcpy(request->request_line.uri, uri);

    request->request_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(request->request_line.version, version);

    request->header_count = 0;
    request->headers = create_headers(INITIAL_HEADER_COUNT); // Initial allocation for headers

    line = strtok(NULL, "\r\n");
    header_index = 0;
    while (line != NULL && line[0] != '\0')
    {
        if (line[0] == '\0')
        {
            // Blank line indicates the end of headers
            // Blank line between headers and body
            break;
        }
        key = strtok(line, ": ");
        value = strtok(NULL, "\r\n");
        if (add_header(&(request->headers), &header_index, &(request->header_count), key, value) != 0)
        {
            fprintf(stderr, "Error al agregar header\n");
            free_headers(&(request->headers), request->header_count);
            free(request->request_line.method);
            free(request->request_line.uri);
            free(request->request_line.version);
            free(request);
            free(temp_buffer);
            return NULL;
        }
        line = strtok(NULL, "\r\n");
    }
    // Process body
    body_start = strtok(NULL, "");
    request->body = (char *)malloc(strlen(body_start) + 1);
    if (request->body == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para el body\n");
        free_headers(&(request->headers), request->header_count);
        free(request->request_line.method);
        free(request->request_line.uri);
        free(request->request_line.version);
        free(request);
        free(temp_buffer);
        return NULL;
    }
    strcpy(request->body, body_start);

    free(temp_buffer);
    return request;
}

int send_http_request(int sockfd, HTTP_Request *request)
{
    char *buffer;
    // Serialize request line and headers
    int size = serialize_http_request(request, &buffer);
    if (size < 0)
    {
        return -1;
    }

    // Send request line and headers
    if (sendall(sockfd, buffer, size) < 0)
    {
        perror("send headers");
        free(buffer);
        return -1;
    }
    free(buffer);

    // Send body if it exists
    if (request->body != NULL)
    {
        int body_len = strlen(request->body);
        if (send_all(sockfd, request->body, body_len) < 0)
        {
            perror("send body");
            return -1;
        }
    }

    return 0;
}

HTTP_Request *receive_http_request(int sockfd)
{
    char buffer[DEFAULT_BUFFER_SIZE];
    char *header_end_ptr, *headers_part;
    const char *content_length_str, *line_ending;
    int already_read, body_length, header_end, size;
    HTTP_Request *request;

    line_ending = "\r\n";

    // Read headers first
    size = read_until_double_crlf(sockfd, buffer, DEFAULT_BUFFER_SIZE);
    if (size <= 0)
    {
        return NULL;
    }

    buffer[size] = '\0'; // Null-terminate the received data

    // Find the end of the headers, marked by \r\n\r\n
    header_end_ptr = strstr(buffer, "\r\n\r\n");
    if (header_end_ptr == NULL)
    {
        fprintf(stderr, "Formato inválido de HTTP request\n");
        return NULL;
    }

    // Calculate the position of the end of the headers
    header_end = header_end_ptr - buffer + (sizeof(line_ending) * 2); // move past \r\n\r\n

    // Deserialize the headers part
    headers_part = (char *)malloc((header_end + 1) * sizeof(char));
    if (headers_part == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers_part\n");
        return NULL;
    }

    strncpy(headers_part, buffer, header_end);
    headers_part[header_end] = '\0'; // Null-terminate

    request = deserialize_http_request(headers_part);
    free(headers_part);

    // Get the Content-Length header value
    content_length_str = find_header_value(request->headers, request->header_count, "Content-Length");
    body_length = 0;
    if (content_length_str != NULL)
    {
        body_length = atoi(content_length_str); // Convert Content-Length to an integer
    }

    // Read the body if it exists
    if (body_length > 0)
    {
        request->body = (char *)malloc((body_length + 1) * sizeof(char)); // +1 for null-terminator
        if (request->body == NULL)
        {
            fprintf(stderr, "Error al asignar memoria para body\n");
            free_http_request(request);
            return NULL;
        }

        // Check if any of the body was already read in the initial read
        already_read = size - header_end;
        if (already_read < 0)
        {
            already_read = 0;
        }
        else if (already_read > 0)
        {
            strncpy(request->body, buffer + header_end, already_read);
        }

        // Read the remaining body
        if (recvall(sockfd, request->body + already_read, body_length - already_read) <= 0)
        {
            free_http_request(request);
            return NULL;
        }
        request->body[body_length] = '\0'; // Null-terminate the body
    }
    else
    {
        request->body = NULL;
    }

    return request;
}

HTTP_Response *create_http_response(const char *version, const int status_code, const char *reason_phrase, const Header *headers, int header_count, const char *body)
{
    int header_index, i;
    HTTP_Response *response;

    if (version == NULL || status_code <= 0 || reason_phrase == NULL || headers == NULL || header_count < 0 || body == NULL)
    {
        puts("Uno o más valores de creación de HTTP Response es inválido\n");
        return NULL;
    }

    response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    response->response_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;

    response->response_line.reason_phrase = (char *)malloc(strlen(version) + 1);
    strcpy(response->response_line.reason_phrase, reason_phrase);

    response->headers = create_headers(header_count);
    if (response->headers == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers: %s\n", strerror(errno));
        free(response->response_line.version);
        free(response->response_line.reason_phrase);
        free(response);
        return NULL;
    }

    response->header_count = header_count;
    header_index = 0;

    for (i = 0; i < header_count; i++)
    {
        if (add_header(&(response->headers), &header_index, &(response->header_count), headers[i].key, headers[i].value) != 0)
        {
            fprintf(stderr, "Error al agregar header\n");
            free(response->response_line.version);
            free(response->response_line.reason_phrase);
            free_headers(&(response->headers), response->header_count);
            free(response);
            return NULL; // Return NULL to indicate failure
        }
    }

    response->body = (char *)malloc(strlen(body) + 1);
    strcpy(response->body, body);

    return response;
}

void free_http_response(HTTP_Response *response)
{
    if (response != NULL)
    {
        if (response->response_line.version != NULL)
        {
            free(response->response_line.version);
        }
        if (response->response_line.reason_phrase != NULL)
        {
            free(response->response_line.reason_phrase);
        }
        free_headers(response->headers, response->header_count);
        if (response->body != NULL)
        {
            free(response->body);
        }
        free(response);
    }
}

int serialize_http_response(HTTP_Response *response, char **buffer)
{
    const char *key_value_separator;
    const char *line_ending;
    const char *space;
    int i, extra_char_size, size;

    key_value_separator = ": ";
    line_ending = "\r\n";
    space = " ";

    size = strlen(response->response_line.version) + sizeof(response->response_line.status_code) + strlen(response->response_line.reason_phrase) + strlen(space) * 2 + strlen(line_ending);
    for (i = 0; i < response->header_count; i++)
    {
        size += strlen(response->headers[i].key) + strlen(response->headers[i].value) + strlen(key_value_separator) + strlen(line_ending);
    }
    size += strlen(line_ending); // For \r\n after headers

    *buffer = (char *)malloc(size * sizeof(char));
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    sprintf(*buffer, "%s %d %s%s", response->response_line.version, response->response_line.status_code, response->response_line.reason_phrase, line_ending);
    for (i = 0; i < response->header_count; i++)
    {
        strcat(*buffer, response->headers[i].key);
        strcat(*buffer, key_value_separator);
        strcat(*buffer, response->headers[i].value);
        strcat(*buffer, line_ending);
    }
    strcat(*buffer, line_ending);

    return size;
}

HTTP_Response *deserialize_http_response(const char *buffer)
{
    char version[VERSION_SIZE], reason_phrase[REASON_PHRASE_SIZE];
    char *body_start, *key, *line, *temp_buffer, *value;
    int header_index, status_code;
    HTTP_Response *response;

    response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return NULL;
    }

    temp_buffer = (char *)malloc(strlen(buffer) + 1);
    if (temp_buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        free(response);
        return NULL;
    }
    strcpy(temp_buffer, buffer);

    line = strtok(temp_buffer, "\r\n");
    sscanf(line, "%s %d %s", version, status_code, reason_phrase);

    response->response_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;

    response->response_line.reason_phrase = (char *)malloc(strlen(reason_phrase) + 1);
    strcpy(response->response_line.reason_phrase, reason_phrase);

    response->header_count = 0;
    response->headers = create_headers(INITIAL_HEADER_COUNT); // Initial allocation for headers

    line = strtok(NULL, "\r\n");
    header_index = 0;
    while (line != NULL && line[0] != '\0')
    {
        if (line[0] == '\0')
        {
            // Blank line indicates the end of headers
            // Blank line between headers and body
            break;
        }
        key = strtok(line, ": ");
        value = strtok(NULL, "\r\n");
        if (add_header(&(response->headers), &header_index, &(response->header_count), key, value) != 0)
        {
            fprintf(stderr, "Error al agregar header\n");
            free_headers(&(response->headers), response->header_count);
            free(response->response_line.version);
            free(response->response_line.reason_phrase);
            free(response);
            free(temp_buffer);
            return NULL;
        }
        line = strtok(NULL, "\r\n");
    }
    // Process body
    body_start = strtok(NULL, "");
    response->body = (char *)malloc(strlen(body_start) + 1);
    if (response->body == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para el body\n");
        free_headers(&(response->headers), response->header_count);
        free(response->response_line.version);
        free(response->response_line.reason_phrase);
        free(response);
        free(temp_buffer);
        return NULL;
    }
    strcpy(response->body, body_start);

    free(temp_buffer);
    return response;
}

int send_http_response(int sockfd, HTTP_Response *response)
{
    char *buffer;
    // Serialize response line and headers
    int size = serialize_http_response(response, &buffer);
    if (size < 0)
    {
        return -1;
    }

    // Send response line and headers
    if (sendall(sockfd, buffer, size) < 0)
    {
        perror("send headers");
        free(buffer);
        return -1;
    }
    free(buffer);

    // Send body if it exists
    if (response->body != NULL)
    {
        int body_len = strlen(response->body);
        if (send_all(sockfd, response->body, body_len) < 0)
        {
            perror("send body");
            return -1;
        }
    }

    return 0;
}

HTTP_Response *receive_http_response(int sockfd)
{
    char buffer[DEFAULT_BUFFER_SIZE];
    char *header_end_ptr, *headers_part;
    const char *content_length_str, *line_ending;
    int already_read, body_length, header_end, size;
    HTTP_Response *response;

    line_ending = "\r\n";

    // Read headers first
    size = read_until_double_crlf(sockfd, buffer, DEFAULT_BUFFER_SIZE);
    if (size <= 0)
    {
        return NULL;
    }

    buffer[size] = '\0'; // Null-terminate the received data

    // Find the end of the headers, marked by \r\n\r\n
    header_end_ptr = strstr(buffer, "\r\n\r\n");
    if (header_end_ptr == NULL)
    {
        fprintf(stderr, "Formato inválido de HTTP request\n");
        return NULL;
    }

    // Calculate the position of the end of the headers
    header_end = header_end_ptr - buffer + (sizeof(line_ending) * 2); // move past \r\n\r\n

    // Deserialize the headers part
    headers_part = (char *)malloc((header_end + 1) * sizeof(char));
    if (headers_part == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers_part\n");
        return NULL;
    }

    strncpy(headers_part, buffer, header_end);
    headers_part[header_end] = '\0'; // Null-terminate

    response = deserialize_http_response(headers_part);
    free(headers_part);

    // Get the Content-Length header value
    content_length_str = find_header_value(response->headers, response->header_count, "Content-Length");
    body_length = 0;
    if (content_length_str != NULL)
    {
        body_length = atoi(content_length_str); // Convert Content-Length to an integer
    }

    // Read the body if it exists
    if (body_length > 0)
    {
        response->body = (char *)malloc((body_length + 1) * sizeof(char)); // +1 for null-terminator
        if (response->body == NULL)
        {
            fprintf(stderr, "Error al asignar memoria para body\n");
            free_http_request(response);
            return NULL;
        }

        // Check if any of the body was already read in the initial read
        already_read = size - header_end;
        if (already_read < 0)
        {
            already_read = 0;
        }
        else if (already_read > 0)
        {
            strncpy(response->body, buffer + header_end, already_read);
        }

        // Read the remaining body
        if (recvall(sockfd, response->body + already_read, body_length - already_read) <= 0)
        {
            free_http_request(response);
            return NULL;
        }
        response->body[body_length] = '\0'; // Null-terminate the body
    }
    else
    {
        response->body = NULL;
    }

    return response;
}

int read_until_double_end_line(int sockfd, char *buffer, int length)
{
    int total = 0; // Total bytes received
    int n;
    char *end_of_headers;

    while (total < length)
    {
        n = recv(sockfd, buffer + total, length - total, 0);
        if (n == -1)
        {
            perror("recv");
            return -1; // Error
        }
        if (n == 0)
        {
            // Connection closed
            return total;
        }
        total += n;
        buffer[total] = '\0';

        // Check if we've received two consecutive CRLF
        end_of_headers = strstr(buffer, "\r\n\r\n");
        if (end_of_headers != NULL)
        {
            // Found the end of headers
            return total;
        }
    }

    return total; // Return total bytes received
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

const char *get_content_type(const char *extension)
{
    if (strcmp(extension, ".jpg") == 0)
    {
        return "image/jpeg";
    }
    else if (strcmp(extension, ".png") == 0)
    {
        return "image/png";
    }
    else if (strcmp(extension, ".gif") == 0)
    {
        return "image/gif";
    }
    else if (strcmp(extension, ".bmp") == 0)
    {
        return "image/bmp";
    }
    else if (strcmp(extension, ".tiff") == 0)
    {
        return "image/tiff";
    }
    else if (strcmp(extension, ".txt") == 0)
    {
        return "text/plain";
    }
    else if (strcmp(extension, ".pdf") == 0)
    {
        return "application/pdf";
    }
    else
    {
        return "application/octet-stream"; // Default for unknown types
    }
}