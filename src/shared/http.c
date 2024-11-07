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
    int i;
    if (headers == NULL || *headers == NULL)
    {
        return; // Nothing to free
    }

    for (i = 0; i < header_count; i++)
    {
        free_header(&((*headers)[i]));
    }
    free(*headers);  // Free the array of headers
    *headers = NULL; // Set the original pointer to NULL
}

void free_header(Header *header)
{
    if (header != NULL)
    {
        if (header->key != NULL)
        {
            free(header->key);
        }
        if (header->value != NULL)
        {
            free(header->value);
        }
    }
}

int add_header(Header **headers, int *header_index, int *header_count, const char *key, const char *value)
{
    if (*header_index >= *header_count)
    {
        *header_count += 1;
        *headers = (Header *)realloc(*headers, (*header_count) * sizeof(Header));
        if (*headers == NULL)
        {
            fprintf(stderr, "server: error al reasignar memoria: %s\n", strerror(errno));
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
                    fprintf(stderr, "server: error al reasignar memoria: %s\n", strerror(errno));
                    return -1;
                }
                *headers = temp;
            }

            return 0; // Success
        }
    }
    return -1; // Header not found
}

int serialize_headers(Header *headers, int header_count, char **buffer)
{
    const char *key_value_separator;
    const char *line_ending;
    int i, size;

    key_value_separator = ": ";
    line_ending = "\r\n";
    size = 0;

    // Calculate the total size needed for the buffer
    for (i = 0; i < header_count; i++)
    {
        size += strlen(headers[i].key) + strlen(headers[i].value) + strlen(key_value_separator) + strlen(line_ending);
    }

    *buffer = (char *)malloc(sizeof(char) * size + 1); // +1 for the null terminator
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
    (*buffer)[size] = '\0';
    return size; // Return the size of the serialized headers
}

Header *deserialize_headers(const char *headers_str, int *header_count)
{
    int count, header_index, initial_count;
    char *colon, *headers_copy, *key, *line, *value;
    const char *temp_str;
    Header *headers;

    // Count the number of headers
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

    // Create initial headers array
    initial_count = count > 0 ? count : 1; // Ensure at least one slot
    headers = create_headers(initial_count);
    if (headers == NULL)
    {
        return NULL;
    }

    // Copy headers_str to a temporary buffer
    headers_copy = (char *)malloc(sizeof(char) * strlen(headers_str) + 1);
    if (headers_copy == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        free(headers);
        return NULL;
    }
    strcpy(headers_copy, headers_str);

    // Tokenize and parse each header
    line = strtok(headers_copy, "\r\n");
    header_index = 0;
    *header_count = initial_count;
    while (line != NULL)
    {
        colon = strchr(line, ':');
        if (colon != NULL)
        {
            *colon = '\0'; // Terminate the key string
            key = line;
            value = colon + 2; // Skip ": " to get the value

            if (add_header(&headers, &header_index, header_count, key, value) != 0)
            {
                free(headers_copy);
                free_headers(&headers, *header_count);
                return NULL;
            }
        }
        line = strtok(NULL, "\r\n");
    }

    free(headers_copy);
    *header_count = header_index;
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

HTTP_Request *create_http_request(const char *method, const char *uri, const char *version, Header *headers, int header_count, const char *body)
{
    HTTP_Request *request;

    if (method == NULL || uri == NULL || version == NULL)
    {
        fprintf(stderr, "Uno o más valores de creación de HTTP Request es inválido\n");
        return NULL;
    }

    request = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (request == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(request, 0, sizeof(HTTP_Request));

    request->request_line.method = (char *)malloc(strlen(method) + 1);
    if (request->request_line.method == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para method\n");
        free(request);
        return NULL;
    }
    strcpy(request->request_line.method, method);

    request->request_line.uri = (char *)malloc(strlen(uri) + 1);
    if (request->request_line.uri == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para uri\n");
        free(request->request_line.method);
        free(request);
        return NULL;
    }
    strcpy(request->request_line.uri, uri);

    request->request_line.version = (char *)malloc(strlen(version) + 1);
    if (request->request_line.version == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para version\n");
        free(request->request_line.uri);
        free(request->request_line.method);
        free(request);
        return NULL;
    }
    strcpy(request->request_line.version, version);

    if (headers != NULL)
    {
        request->headers = headers;
    }

    if (header_count >= 0)
    {
        request->header_count = header_count;
    }
    else
    {
        request->header_count = 0;
    }

    if (body != NULL)
    {
        request->body = (char *)malloc(strlen(body) + 1);
        if (request->body == NULL)
        {
            fprintf(stderr, "Error al asignar memoria para body\n");
            free(request->request_line.version);
            free(request->request_line.uri);
            free(request->request_line.method);
            free(request);
            return NULL;
        }
        strcpy(request->body, body);
    }

    return request;
}

void free_http_request(HTTP_Request **request)
{
    if (request != NULL && *request != NULL)
    {
        if ((*request)->request_line.method != NULL)
        {
            free((*request)->request_line.method);
        }
        if ((*request)->request_line.uri != NULL)
        {
            free((*request)->request_line.uri);
        }
        if ((*request)->request_line.version != NULL)
        {
            free((*request)->request_line.version);
        }
        free_headers(&((*request)->headers), (*request)->header_count);
        if ((*request)->body != NULL)
        {
            free((*request)->body);
        }
        free(*request);
        *request = NULL; // Set the original pointer to NULL
    }
}

int serialize_http_request_header(HTTP_Request *request, char **buffer)
{
    const char *line_ending;
    const char *space;
    char *headers_buffer;
    int size_buffer, size_headers_buffer;

    line_ending = "\r\n";
    space = " ";

    size_headers_buffer = serialize_headers(request->headers, request->header_count, &headers_buffer);
    if (size_headers_buffer < 0)
    {
        fprintf(stderr, "Error al serializar headers\n");
        return -1;
    }

    size_buffer = strlen(request->request_line.method) + strlen(request->request_line.uri) + strlen(request->request_line.version) + strlen(space) * 2 + strlen(line_ending);
    size_buffer += size_headers_buffer;
    size_buffer += strlen(line_ending); // For \r\n after headers

    *buffer = (char *)malloc(size_buffer * sizeof(char));
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    sprintf(*buffer, "%s %s %s%s", request->request_line.method, request->request_line.uri, request->request_line.version, line_ending);
    strcat(*buffer, headers_buffer);
    strcat(*buffer, line_ending);

    return size_buffer;
}

HTTP_Request *deserialize_http_request_header(const char *buffer)
{
    char method[METHOD_SIZE], uri[URI_SIZE], version[VERSION_SIZE];
    char *line, *temp_buffer;
    char *header_end_ptr, *headers_part;
    const char *line_ending;
    int header_length;
    HTTP_Request *request;

    line_ending = "\r\n";

    request = (HTTP_Request *)malloc(sizeof(HTTP_Request));
    if (request == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return NULL;
    }

    temp_buffer = (char *)malloc(sizeof(char) * strlen(buffer) + 1);
    if (temp_buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        free(request);
        return NULL;
    }
    strcpy(temp_buffer, buffer);

    line = strtok(temp_buffer, "\r\n");
    if (line == NULL)
    {
        fprintf(stderr, "strtok falló al tratar de tokenizar el buffer\n");
        free(temp_buffer);
        free(request);
        return NULL;
    }

    if (sscanf(line, "%s %s %s", method, uri, version) != 3)
    {
        fprintf(stderr, "Error al parsear request line\n");
        free(temp_buffer);
        free(request);
        return NULL;
    }

    request->request_line.method = (char *)malloc(strlen(method) + 1);
    strcpy(request->request_line.method, method);

    request->request_line.uri = (char *)malloc(strlen(uri) + 1);
    strcpy(request->request_line.uri, uri);

    request->request_line.version = (char *)malloc(strlen(version) + 1);
    strcpy(request->request_line.version, version);

    // Find the end of headers, marked by \r\n\r\n
    header_end_ptr = strstr(buffer, "\r\n\r\n");
    if (header_end_ptr == NULL)
    {
        fprintf(stderr, "Formato inválido de HTTP request\n");
        free(temp_buffer);
        free_http_request(&request);
        return NULL;
    }

    header_length = header_end_ptr - buffer + (strlen(line_ending) * 2); // move past \r\n\r\n
    headers_part = (char *)malloc((header_length + 1) * sizeof(char));
    if (headers_part == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers_part\n");
        free(temp_buffer);
        free_http_request(&request);
        return NULL;
    }
    strncpy(headers_part, buffer, header_length);
    headers_part[header_length] = '\0';

    // Use the deserialize_headers function
    request->headers = deserialize_headers(headers_part, &request->header_count);
    if (request->headers == NULL)
    {
        free(headers_part);
        free(temp_buffer);
        free_http_request(&request);
        return NULL;
    }

    free(headers_part);
    free(temp_buffer);
    return request;
}

int send_http_request(int sockfd, HTTP_Request *request)
{
    char *buffer;
    int size;

    // Serialize request line and headers
    size = serialize_http_request_header(request, &buffer);
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
        if (sendall(sockfd, request->body, request->body_length) < 0)
        {
            perror("send body");
            return -1;
        }
    }

    return 0;
}

HTTP_Request *receive_http_request(int sockfd)
{
    char *buffer;
    const char *content_length_str;
    int size, extra_data_length;
    HTTP_Request *request;

    buffer = (char *)malloc(DEFAULT_BUFFER_SIZE);
    if (buffer == NULL)
    {
        fprintf(stderr, "error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    // Read headers first
    size = read_until_double_end_line(sockfd, &buffer, DEFAULT_BUFFER_SIZE, &extra_data_length);
    if (size <= 0)
    {
        free(buffer);
        return NULL;
    }

    request = deserialize_http_request_header(buffer);

    // Get the Content-Length header value
    content_length_str = find_header_value(request->headers, request->header_count, "Content-Length");
    request->body_length = 0;
    if (content_length_str != NULL)
    {
        request->body_length = atoi(content_length_str); // Convert Content-Length to an integer
    }

    // Read the body if it exists
    if (request->body_length > 0)
    {
        request->body = (char *)malloc(request->body_length + 1); // +1 for null-terminator
        if (request->body == NULL)
        {
            fprintf(stderr, "Error allocating memory for body\n");
            free(buffer);
            free_http_request(&request);
            return NULL;
        }

        // If extra data length is greater than 0, copy it to response body
        if (extra_data_length > 0)
        {
            memcpy(request->body, buffer + (size - extra_data_length), extra_data_length);
        }

        // Read the remaining body
        int remaining_length = request->body_length - extra_data_length;
        if (remaining_length > 0)
        {
            if (recvall(sockfd, request->body + extra_data_length, remaining_length) <= 0)
            {
                free(buffer);
                free_http_request(&request);
                return NULL;
            }
        }

        // Null-terminate the body
        request->body[request->body_length] = '\0';
    }
    else
    {
        request->body = NULL;
    }

    free(buffer); // Now it's safe to free buffer
    return request;
}

HTTP_Response *create_http_response(const char *version, const int status_code, const char *reason_phrase, Header *headers, int header_count, const char *body)
{
    HTTP_Response *response;

    if (version == NULL || status_code <= 0 || reason_phrase == NULL)
    {
        fprintf(stderr, "Uno o más valores de creación de HTTP Response es inválido\n");
        return NULL;
    }

    response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }
    memset(response, 0, sizeof(HTTP_Response));

    response->response_line.version = (char *)malloc(sizeof(char) * strlen(version) + 1);
    if (response->response_line.version == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para version\n");
        free(response);
        return NULL;
    }
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;

    response->response_line.reason_phrase = (char *)malloc(sizeof(char) * strlen(reason_phrase) + 1);
    if (response->response_line.reason_phrase == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para reason_phrase\n");
        free(response->response_line.version);
        free(response);
        return NULL;
    }
    strcpy(response->response_line.reason_phrase, reason_phrase);

    if (headers != NULL)
    {
        response->headers = headers;
    }

    if (header_count >= 0)
    {
        response->header_count = header_count;
    }
    else
    {
        response->header_count = 0;
    }

    if (body != NULL)
    {
        response->body = (char *)malloc(sizeof(char) * strlen(body) + 1);
        if (response->body == NULL)
        {
            fprintf(stderr, "Error al asignar memoria para body\n");
            free(response->response_line.version);
            free(response->response_line.reason_phrase);
            free(response);
            return NULL;
        }
        strcpy(response->body, body);
    }

    return response;
}

void free_http_response(HTTP_Response **response)
{
    if (response != NULL && *response != NULL)
    {
        if ((*response)->response_line.version != NULL)
        {
            free((*response)->response_line.version);
        }
        if ((*response)->response_line.reason_phrase != NULL)
        {
            free((*response)->response_line.reason_phrase);
        }
        free_headers(&((*response)->headers), (*response)->header_count);
        if ((*response)->body != NULL)
        {
            free((*response)->body);
        }
        free(*response);
        *response = NULL; // Set the original pointer to NULL
    }
}

int serialize_http_response(HTTP_Response *response, char **buffer)
{
    const char *line_ending, *space;
    char *headers_buffer, *ptr;
    int size_buffer, size_headers_buffer;

    line_ending = "\r\n";
    space = " ";

    size_headers_buffer = serialize_headers(response->headers, response->header_count, &headers_buffer);
    if (size_headers_buffer < 0)
    {
        fprintf(stderr, "Error al serializar headers\n");
        return -1;
    }

    size_buffer = strlen(response->response_line.version) + sizeof(response->response_line.status_code) +
                  strlen(response->response_line.reason_phrase) + strlen(space) * 2 + strlen(line_ending);
    size_buffer += size_headers_buffer;
    size_buffer += strlen(line_ending); // For \r\n after headers

    if (response->body != NULL)
    {
        size_buffer += response->body_length;
    }

    *buffer = (char *)malloc(size_buffer);
    if (*buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    ptr = *buffer;
    ptr += sprintf(ptr, "%s %d %s%s", response->response_line.version, response->response_line.status_code, response->response_line.reason_phrase, line_ending);
    memcpy(ptr, headers_buffer, size_headers_buffer);
    ptr += size_headers_buffer;
    memcpy(ptr, line_ending, strlen(line_ending));
    ptr += strlen(line_ending);

    if (response->body != NULL)
    {
        memcpy(ptr, response->body, response->body_length);
    }

    free(headers_buffer);

    return size_buffer;
}

HTTP_Response *deserialize_http_response_header(const char *buffer)
{
    char reason_phrase[REASON_PHRASE_SIZE], version[VERSION_SIZE];
    char *line, *temp_buffer, *reason_start;
    char *header_end_ptr, *headers_part;
    const char *line_ending;
    int header_length, status_code;
    HTTP_Response *response;

    line_ending = "\r\n";

    response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    if (response == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        return NULL;
    }

    temp_buffer = (char *)malloc(sizeof(char) * strlen(buffer) + 1);
    if (temp_buffer == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
        free(response);
        return NULL;
    }
    strcpy(temp_buffer, buffer);

    line = strtok(temp_buffer, "\r\n");
    if (line == NULL)
    {
        fprintf(stderr, "strtok falló al tratar de tokenizar el buffer\n");
        free(temp_buffer);
        free(response);
        return NULL;
    }
    if (sscanf(line, "%s %d", version, &status_code) != 2)
    {
        fprintf(stderr, "Error al parsear response line\n");
        free(temp_buffer);
        free(response);
        return NULL;
    }

    reason_start = strchr(line, ' ');
    if (reason_start != NULL)
    {
        reason_start = strchr(reason_start + 1, ' ');
        if (reason_start != NULL)
        {
            strcpy(reason_phrase, reason_start + 1);
        }
        else
        {
            fprintf(stderr, "Error al parsear reason phrase\n");
            free(temp_buffer);
            free(response);
            return NULL;
        }
    }
    else
    {
        fprintf(stderr, "Error al parsear response line\n");
        free(temp_buffer);
        free(response);
        return NULL;
    }

    response->response_line.version = (char *)malloc(sizeof(char) * strlen(version) + 1);
    strcpy(response->response_line.version, version);

    response->response_line.status_code = status_code;

    response->response_line.reason_phrase = (char *)malloc(sizeof(char) * strlen(reason_phrase) + 1);
    strcpy(response->response_line.reason_phrase, reason_phrase);

    // Find the end of headers, marked by \r\n\r\n
    header_end_ptr = strstr(buffer, "\r\n\r\n");
    if (header_end_ptr == NULL)
    {
        fprintf(stderr, "Formato inválido de HTTP response\n");
        free(temp_buffer);
        free_http_response(&response);
        return NULL;
    }

    header_length = header_end_ptr - buffer + (strlen(line_ending) * 2); // move past \r\n\r\n
    headers_part = (char *)malloc(sizeof(char) * header_length + 1);
    if (headers_part == NULL)
    {
        fprintf(stderr, "Error al asignar memoria para headers_part\n");
        free(temp_buffer);
        free_http_response(&response);
        return NULL;
    }
    strncpy(headers_part, buffer, header_length);
    headers_part[header_length] = '\0';

    // Use the deserialize_headers function
    response->headers = deserialize_headers(headers_part, &response->header_count);
    if (response->headers == NULL)
    {
        free(headers_part);
        free(temp_buffer);
        free_http_response(&response);
        return NULL;
    }

    free(headers_part);
    free(temp_buffer);
    return response;
}

int send_http_response(int sockfd, HTTP_Response *response)
{
    char *buffer;
    int size;

    // Serialize response line and headers
    size = serialize_http_response(response, &buffer);
    if (size < 0)
    {
        return -1;
    }

    // Send the complete response (headers + body)
    if (sendall(sockfd, buffer, size) < 0)
    {
        perror("send response");
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

HTTP_Response *receive_http_response(int sockfd)
{
    char *buffer;
    const char *content_length_str;
    int size, extra_data_length;
    HTTP_Response *response;

    buffer = (char *)malloc(DEFAULT_BUFFER_SIZE);
    if (buffer == NULL)
    {
        fprintf(stderr, "error al asignar memoria: %s\n", strerror(errno));
        return NULL;
    }

    // Read headers first
    size = read_until_double_end_line(sockfd, &buffer, DEFAULT_BUFFER_SIZE, &extra_data_length);
    if (size <= 0)
    {
        free(buffer);
        return NULL;
    }

    response = deserialize_http_response_header(buffer);

    // Get the Content-Length header value
    content_length_str = find_header_value(response->headers, response->header_count, "Content-Length");
    response->body_length = 0;
    if (content_length_str != NULL)
    {
        response->body_length = atoi(content_length_str); // Convert Content-Length to an integer
    }

    // Read the body if it exists
    if (response->body_length > 0)
    {
        response->body = (char *)malloc(response->body_length + 1); // +1 for null-terminator
        if (response->body == NULL)
        {
            fprintf(stderr, "Error allocating memory for body\n");
            free(buffer);
            free_http_response(&response);
            return NULL;
        }

        // If extra data length is greater than 0, copy it to response body
        if (extra_data_length > 0)
        {
            memcpy(response->body, buffer + (size - extra_data_length), extra_data_length);
        }

        // Read the remaining body
        int remaining_length = response->body_length - extra_data_length;
        if (remaining_length > 0)
        {
            if (recvall(sockfd, response->body + extra_data_length, remaining_length) <= 0)
            {
                free(buffer);
                free_http_response(&response);
                return NULL;
            }
        }

        // Null-terminate the body
        response->body[response->body_length] = '\0';
    }
    else
    {
        response->body = NULL;
    }

    free(buffer); // Now it's safe to free buffer
    return response;
}

int read_until_double_end_line(int sockfd, char **buffer_ptr, int length, int *extra_data_length)
{
    int total_bytes = 0; // Total bytes received
    int bytes_recv;
    char *buffer, *end_of_headers;

    buffer = *buffer_ptr;
    *extra_data_length = 0; // Initialize extra data length

    while (total_bytes < length)
    {
        bytes_recv = recv(sockfd, buffer + total_bytes, length - total_bytes, 0);
        if (bytes_recv == -1)
        {
            fprintf(stderr, "Error al intentar recibir datos: %s\n", strerror(errno));
            return -1; // Error
        }
        else if (bytes_recv == 0)
        {
            // Connection closed
            fprintf(stderr, "conexión cerrada al intentar leer\n");
            return 0;
        }

        total_bytes += bytes_recv;
        buffer[total_bytes] = '\0'; // Null-terminate the buffer

        // Check if we've received two consecutive CRLF
        end_of_headers = strstr(buffer, "\r\n\r\n");
        if (end_of_headers != NULL)
        {
            // Found the end of headers
            *extra_data_length = total_bytes - (end_of_headers - buffer + 4); // 4 for "\r\n\r\n"
            break;
        }
    }

    return total_bytes; // Return total bytes received
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
    else if (strcmp(extension, ".jpeg") == 0)
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
    else if (strcmp(extension, ".html") == 0)
    {
        return "text/html";
    }
    else
    {
        return "application/octet-stream"; // Default for unknown types
    }
}