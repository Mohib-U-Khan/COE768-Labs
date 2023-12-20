#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000   
#define BUFLEN          100   

void send_file_content(int sd, FILE *file);
void reaper(int sig);

int main(int argc, char **argv)
{
    int sd, new_sd, client_len, port;
    struct sockaddr_in server, client;

    switch(argc) {
    case 1:
        port = SERVER_TCP_PORT;
        break;
    case 2:
        port = atoi(argv[1]);
        break;
    default:
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't bind name to socket\n");
        exit(1);
    }

    listen(sd, 5);
    (void) signal(SIGCHLD, reaper);

    while(1) {
        client_len = sizeof(client);
        new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
        if(new_sd < 0) {
            fprintf(stderr, "Can't accept client \n");
            exit(1);
        }

        switch (fork()) {
        case 0:     // child
            close(sd);
            exit(echod(new_sd));
        default:    // parent
            close(new_sd);
            break;
        case -1:
            fprintf(stderr, "fork: error\n");
        }
    }

    return 0;
}

int echod(int sd) {
    char filename[BUFLEN];
    int n;
    FILE *file;

    n = read(sd, filename, BUFLEN);
    if (n <= 0) {
        perror("Read error");
        close(sd);
        return 1;
    }
    filename[n] = '\0';  // Null-terminate

    file = fopen(filename, "r");
    if (file) {
        send_file_content(sd, file);
        fclose(file);
    } else {
        char *errMsg = "$Error: File not found or cannot be opened.\n";
        write(sd, errMsg, strlen(errMsg));
    }

    close(sd);
    return 0;
}

void send_file_content(int sd, FILE *file) {
    char buffer[BUFLEN];
    int bytes_read, total_bytes = 0;

    while (!feof(file)) {
        bytes_read = fread(buffer + total_bytes, 1, 1, file);
        total_bytes += bytes_read;

        if (total_bytes == 100 || (bytes_read == 0 && total_bytes > 0)) {
            if (write(sd, buffer, total_bytes) < total_bytes) {
                perror("Write error");
                break;
            }
            total_bytes = 0;
        }
    }
}

void reaper(int sig)
{
    int status;
    while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
