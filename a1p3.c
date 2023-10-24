#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_MESSAGE_SIZE 2000
#define MAX_ADDRESS_SIZE 100
#define MAX_BUFFER_SIZE 10
#define SERVER_PORT 80

int extractKeyAndLink(const char *server, char *getkey, char *getlink) {
    const char *key_start = strstr(server, "Your key is ");

    if (key_start) {
        key_start = strchr(key_start, '=');
        const char *key_end = strchr(key_start, '<');

        if (key_end) {
            strncpy(getkey, key_start, key_end - key_start);
            getkey[key_end - key_start] = '\0';
        } else {
            return 0; // Unable to extract key
        }
    } else {
        return 0; // Key not found
    }

    const char *link_start = strstr(server, "give them this link: ");

    if (link_start) {
        link_start += strlen("give them this link: ");
        const char *link_end = strchr(link_start, '<');

        if (link_end) {
            strncpy(getlink, link_start, link_end - link_start);
            getlink[link_end - link_start] = '\0';
        } else {
            return 0; // Unable to extract link
        }
    } else {
        return 0; // Link not found
    }

    return 1; // Extraction successful
}


int main(void) {
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[MAX_MESSAGE_SIZE];
    char address[MAX_ADDRESS_SIZE];

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        perror("Unable to create socket");
        return -1;
    }

    printf("Socket created successfully\n");

    // Configure server details using getaddrinfo:
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // get the IP of the page we want to scrape
    int out = getaddrinfo("www-test.cs.umanitoba.ca", NULL, &hints, &result);
    if (out != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(out));
        exit(EXIT_FAILURE);
    }

    // Copy server details:
    memcpy(&server_addr, result->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(result);

    // Set port and IP the same as server-side:
    server_addr.sin_port = htons(SERVER_PORT);

    // converts to octets
    printf("Convert...\n");
    inet_ntop(server_addr.sin_family, &server_addr.sin_addr, address, MAX_ADDRESS_SIZE);
    printf("Connecting to %s\n", address);

    // Send connection request to server:
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to connect");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    printf("Connected with server successfully\n");

    const char *request = "POST /~comp3010/cgi-bin/a1/newnote.cgi HTTP/1.1\r\n"
                          "Host: www-test.cs.umanitoba.ca\r\n"
                          "Content-Type: application/x-www-form-urlencoded\r\n";

    // Data to be sent in the request body
    const char *body1 = "name=";
    const char *name = "Linh+Bui+Tien+Huynh";     //CHANGE NAME HERE
    const char *body2 = "&message=";
    const char *message = "This is a secured message";       //CHANGE MESSAGE HERE

    char *body = malloc(strlen(body1) + strlen(name) + strlen(body2) +strlen(message) + 1);
    if (body == NULL) {
        perror("Error allocating memory for header body");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    strcpy(body, body1);
    strcat(body, name);
    strcat(body, body2);
    strcat(body, message);

    // Add Content-Length header based on the length of the body
    char content_length_header[50];
    sprintf(content_length_header, "Content-Length: %lu\r\n\r\n", strlen(body));

    // Concatenate the headers, request, and body
    char *full_request = malloc(strlen(request) + strlen(content_length_header) + strlen(body) + 1);
    if (full_request == NULL) {
        perror("Error allocating memory for request");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    strcpy(full_request, request);
    strcat(full_request, content_length_header);
    strcat(full_request, body);

    // Send the message to the server:
    printf("Sending request with name = %s and message = %s, %lu bytes\n", name,message,strlen(full_request));
    if (send(socket_desc, full_request, strlen(full_request), 0) < 0) {
        perror("Unable to send message");
        close(socket_desc);
        free(full_request);  // Free allocated memory
        exit(EXIT_FAILURE);
    }

    // Receive the server's response:
    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
        perror("Error while receiving server's msg");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    char key[50];
    char link[50];
    //printf("Server's response:\n%s\n", server_message);
    extractKeyAndLink(server_message,key,link);
    printf("Key: %s\nLink: %s\n", key+19, link);

    sleep(3);

    // Send GET request to the GIVEN LINK
    const char *request_to_message = "GET ";
    const char *request_endpoint = " HTTP/1.1\r\nHost: www-test.cs.umanitoba.ca\r\n\r\n";

    // Concatenate the headers, request, and body
    char *full_request_to_message = malloc(strlen(request_to_message) + strlen(link+31) + strlen(request_endpoint) + 1);
    if (full_request_to_message == NULL) {
        perror("Error allocating memory for request");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    strcpy(full_request_to_message, request_to_message);
    strcat(full_request_to_message, link+31);
    strcat(full_request_to_message, request_endpoint);

    //printf("Request to get message: %s\n", full_request_to_message);
    
    // Send the message to the server:
    printf("Sending request to newly-created message, %lu bytes\n", strlen(full_request_to_message));
    if (send(socket_desc, full_request_to_message, strlen(full_request_to_message), 0) < 0) {
        perror("Unable to send message");
        close(socket_desc);
        free(full_request_to_message);  // Free allocated memory
        exit(EXIT_FAILURE);
    }

    // Receive the server's response:
    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
        perror("Error while receiving server's msg");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }
    //printf("Server's response to GET:\n%s\n", server_message);

    // Check if the expected message is present in the server's response
    if ( (strstr(server_message, message) != NULL)  && (strstr(server_message, name) != NULL) ) {
        printf("TEST PASSED: The message from %s is available and is the same message: %s.\n", name, message);
    } else {
        printf("ASSERTION ERROR: The expected message is not found in the server's response.\n");
    }

    // Send the message to the server 2nd time
    printf("Sending request to opened message, %lu bytes\n", strlen(full_request_to_message));
    if (send(socket_desc, full_request_to_message, strlen(full_request_to_message), 0) < 0) {
        perror("Unable to send message");
        close(socket_desc);
        free(full_request_to_message);  // Free allocated memory
        exit(EXIT_FAILURE);
    }

    // Expect error response from server
    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
        perror("Error while receiving server's msg");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    sleep(2);

    //Expect 404 NOT FOUND from this server_message
    //printf("Server's response to GET:\n%s\n", server_message);
    const char *bad_response = "HTTP/1.1 404 Not Found";
    if (strstr(server_message, bad_response) != NULL) {
        printf("TEST PASSED: The message becomes unavailable. \nResponse code: %s", bad_response);
    } else {
        printf("ASSERTION ERROR: The message is still avaiable after read.\n");
        printf("ASSERTION ERROR: Server's response:\n%s\n", server_message);
    }

    // Close the socket:
    close(socket_desc);
    free(full_request);
    free(full_request_to_message);

    return 0;
}