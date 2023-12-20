#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000   
#define BUFLEN          100   

void save_file_content(int sd, const char *filename);

int main(int argc, char **argv)
{
    int n, sd, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char *host, filename[BUFLEN];

    switch(argc) {
    case 2:
        host = argv[1];
        port = SERVER_TCP_PORT;
        break;
    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
        exit(1);
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((hp = gethostbyname(host))) {
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    } else if (!inet_aton(host, (struct in_addr *) &server.sin_addr)) {
        fprintf(stderr, "Can't get server's address\n");
        exit(1);
    }

    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't connect \n");
        exit(1);
    }

    printf("Enter the filename to request: ");
    fgets(filename, BUFLEN, stdin);
    n = strlen(filename);
    if (filename[n-1] == '\n') filename[n-1] = '\0'; // Remove newline character
    write(sd, filename, n);   // Send filename to server

    save_file_content(sd, filename);

    close(sd);
    return(0);
}

void save_file_content(int sd, const char *filename) {
    char buffer[BUFLEN];
    int bytes_received;

    FILE *local_file = fopen(filename, "wb");
    if (!local_file) {
        fprintf(stderr, "Unable to open local file for writing\n");
        return;
    }

    while ((bytes_received = read(sd, buffer, BUFLEN)) > 0) {
        if (buffer[0] == '$') {
            fprintf(stderr, "Error from server: %s\n", buffer + 1);
            break;
        }
        fwrite(buffer, 1, bytes_received, local_file);
    }

    fclose(local_file);
}
