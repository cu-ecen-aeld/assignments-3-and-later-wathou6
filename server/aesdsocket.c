#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>

#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

// #define DEBUG_LOG(msg,...) printf("aesdsocket: " msg "\n" , ##__VA_ARGS__)
// #define ERROR_LOG(msg,...) printf("aesdsocket ERROR: " msg "\n" , ##__VA_ARGS__)

int server_fd = -1;
int client_fd = -1;
FILE *file = NULL;

void cleanexit() {
    if (server_fd != -1) {
        close(server_fd);
    }
    if (client_fd != -1) {
        close(client_fd);
    }
    if (file) {
        fclose(file);
        file = NULL;
    }
    remove(FILE_PATH);
    closelog();
}

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        cleanexit();
        exit(EXIT_SUCCESS);
    }
}

void daemon_run() {
    pid_t pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "Failed to fork daemon: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Child process continues
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_RDWR);   // stderr

}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};
    size_t bytes_received;
    char *packet_buf = NULL;
    size_t packet_buf_size = 0;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);


    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR,"can't open socket: %s", strerror(errno));
        cleanexit();
        return -1;
    }

    int opt = 1;
    int ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1) {
        // syslog(LOG_ERR,"setsockopt failed: %s", strerror(errno));
        cleanexit();
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    
    ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        // syslog(LOG_ERR, "bind failed: %s", strerror(errno));
        cleanexit();
        return -1;
    }

    
    if (daemon_mode) {
        daemon_run();
    }

    ret = listen(server_fd, 10);
    if (ret == -1) {
        // syslog(LOG_ERR, "listen failed: %s", strerror(errno));
        cleanexit();
        return -1;
    }

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            syslog(LOG_ERR, "accept failed: %s", strerror(errno));
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        free(packet_buf);
        packet_buf = NULL;
        packet_buf_size = 0;

        // Receive until newline
        int newline_found = 0;
        while (!newline_found) {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) break;

            char *new_buf = realloc(packet_buf, packet_buf_size + bytes_received + 1);
            if (!new_buf) {
                syslog(LOG_ERR, "realloc failed");
                free(packet_buf);
                packet_buf = NULL;
                break;
            }

            packet_buf = new_buf;
            memcpy(packet_buf + packet_buf_size, buffer, bytes_received);
            packet_buf_size += bytes_received;
            packet_buf[packet_buf_size] = '\0';

            if (memchr(buffer, '\n', bytes_received)) {
                newline_found = 1;
            }
        }

        // Append to file
        if (packet_buf && packet_buf_size > 0) {
            file = fopen(FILE_PATH, "a");
            if (!file) {
                syslog(LOG_ERR, "File open for append failed: %s", strerror(errno));
                free(packet_buf);
                close(client_fd);
                client_fd = -1;
                continue;
            }
            fwrite(packet_buf, 1, packet_buf_size, file);
            fflush(file);
            fsync(fileno(file));
            fclose(file);
            file = NULL;
        }
        free(packet_buf);
        packet_buf = NULL;
        packet_buf_size = 0;

        file = fopen(FILE_PATH, "r");
        if (!file) {
            syslog(LOG_ERR, "File open for read failed: %s", strerror(errno));
            free(packet_buf);
            close(client_fd);
            client_fd = -1;
            continue;
        }

        while ((bytes_received = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            size_t sent = 0;
            while (sent < bytes_received) {
                ssize_t result = send(client_fd, buffer + sent, bytes_received - sent, 0);
                if (result <= 0) break;
                sent += result;
            }
        }

        fclose(file);
        file = NULL;

        close(client_fd);
        client_fd = -1;
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        
    }
    cleanexit();
    return 0;
}