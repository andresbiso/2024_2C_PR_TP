#ifndef HTTP_H
#define HTTP_H

#define INITIAL_HEADER_COUNT 1
#define METHOD_SIZE 16
#define URI_SIZE 256
#define VERSION_SIZE 16
#define REASON_PHRASE_SIZE 256

typedef struct
{
    char *key;
    char *value;
} Header;

typedef struct
{
    char *method;  // HTTP method (GET, POST, etc.)
    char *uri;     // Request/Response URI
    char *version; // HTTP version (e.g., HTTP/1.1)
} Request_Line;

typedef struct
{
    Request_Line request_line; // Request line containing method, URI, and version
    Header *headers;           // Array of headers
    int header_count;          // Number of headers
    char *body;                // Request or response body
    int body_length;           // Length of the body
} HTTP_Request;

typedef struct
{
    char *version;       // HTTP version (e.g., HTTP/1.1)
    int status_code;     // Status code (e.g., 200, 404)
    char *reason_phrase; // Reason phrase (e.g., OK, Not Found)
} Response_Line;

typedef struct
{
    Response_Line response_line; // Response line containing version, status code, and reason phrase
    Header *headers;             // Array of headers
    int header_count;            // Number of headers
    char *body;                  // Response body
    int body_length;             // Length of the body
} HTTP_Response;

Header *create_headers(int initial_count);
void free_headers(Header **headers, int header_count);
void free_header(Header *header);
int add_header(Header **headers, int *header_index, int *header_count, const char *key, const char *value);
int remove_header(Header **headers, int *header_index, int *header_count, const char *key);
int serialize_headers(Header *headers, int header_count, char **buffer);
Header *deserialize_headers(const char *headers_str, int *header_count);
const char *find_header_value(Header *headers, int header_count, const char *key);
void log_headers(Header *headers, int header_count);

HTTP_Request *create_http_request(const char *method, const char *uri, const char *version, Header *headers, int header_count, const char *body);
void free_http_request(HTTP_Request **request);
int serialize_http_request_header(HTTP_Request *request, char **buffer);
HTTP_Request *deserialize_http_request_header(const char *buffer);
int send_http_request(int sockfd, HTTP_Request *request);
HTTP_Request *receive_http_request(int sockfd);

HTTP_Response *create_http_response(const char *version, const int status_code, const char *reason_phrase, Header *headers, int header_count, const char *body);
void free_http_response(HTTP_Response **response);
int serialize_http_response(HTTP_Response *response, char **buffer);
HTTP_Response *deserialize_http_response_header(const char *buffer);
int send_http_response(int sockfd, HTTP_Response *response);
HTTP_Response *receive_http_response(int sockfd);

int read_until_double_end_line(int sockfd, char *buffer, int length);
const char *get_extension(const char *content_type);
const char *get_content_type(const char *extension);

#endif // HTTP_H