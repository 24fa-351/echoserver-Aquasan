//Alex Nguyen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

DWORD WINAPI handle_client(void *arg);

int start_server(int port, int multi_threaded);

int main(int argc, char *argv[]) {
    int port = 0;
    int multi_threaded = 0;

    // ./echo_server -p <port> <--multi-thread>
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -p <port> [--multi-threaded]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "No port specified after -p\n");
                exit(EXIT_FAILURE);
            }
            //enables the multi-thread
        } else if (strcmp(argv[i], "--multi-thread") == 0) {
            multi_threaded = 1;
        }
    }

    if (port == 0) {
        fprintf(stderr, "Port must be specified with -p\n");
        exit(EXIT_FAILURE);
    }

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }

    int result = start_server(port, multi_threaded);

    WSACleanup();
    return result;
}

DWORD WINAPI handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET *)arg;
    free(arg);
    char buffer[1024];
    int bytes_read;
    int buffer_index = 0;

    printf("Client connected.\n");

    while ((bytes_read = recv(client_socket, &buffer[buffer_index], 1, 0)) > 0) {
        if (buffer[buffer_index] == '\n') {
            buffer[buffer_index + 1] = '\0'; 
            printf("Received line: %s", buffer); 
            buffer_index = 0; 
        } else {
            buffer_index++;
            // Prevent buffer overflow
            if (buffer_index >= sizeof(buffer) - 1) {
                fprintf(stderr, "Line too long, resetting buffer.\n");
                buffer_index = 0;
            }
        }
    }

    if (bytes_read == -1) {
        perror("recv");
    }

    printf("Client disconnected.\n");
    closesocket(client_socket);
    return 0;
}

int start_server(int port, int multi_threaded) {
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    // Create a socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("bind");
        closesocket(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 1) == SOCKET_ERROR) {
        perror("listen");
        closesocket(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        SOCKET *client_socket = malloc(sizeof(SOCKET));
        if ((*client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == INVALID_SOCKET) {
            perror("accept");
            free(client_socket);
            continue;
        }

        // Handle the client connection
        if (multi_threaded) {
            HANDLE thread_handle;
            DWORD thread_id;
            thread_handle = CreateThread(NULL, 0, handle_client, client_socket, 0, &thread_id);
            if (thread_handle == NULL) {
                perror("CreateThread");
                closesocket(*client_socket);
                free(client_socket);
            } else {
                CloseHandle(thread_handle);
            }
        } else {
            handle_client(client_socket);
        }
    }

    closesocket(server_socket);
    return 0;
}
