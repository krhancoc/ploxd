#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    const char *hostname = argv[1];
    struct hostent *host;

    // Use gethostbyname to resolve the hostname to an IP address
    host = gethostbyname(hostname);
    if (host == NULL) {
        herror("gethostbyname");
        return 1;
    }

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80); // Port 80 for HTTP
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    // Connect to the remote host
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to %s\n", hostname);

    // Clean up and close the socket
    close(sockfd);

    return 0;
}
