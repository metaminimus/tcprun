#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

int usage(const char* name)
{
    fprintf(stderr, "Helper utility to open a socket and run a command with stdin and stdout set to the socket\n\n");
    fprintf(stderr, "USAGE: %s address port cmd [cmd_arg1 [cmd_arg2 ..]] \n", name);
    return 0;
}

int main(int argc, char * const argv[])
{
    if (argc < 2) {
    	usage(argv[0]);
	    return 1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock_fd) {
        perror("ERROR: could not open socket");
    	return 2;
    }

    int port = atoi(argv[2]);

    if (port <1 || port >= (1 << 16)) {
        fprintf(stderr, "ERROR: invalid port %d\n", port);
	    return 3;
    }

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(argv[1]);
    remote_addr.sin_port = htons(port);

    if (-1 == connect(sock_fd, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr))) {
        fprintf(stderr, "ERROR: could not connect to %s %d: err %d %s\n", argv[1], port, errno, strerror(errno));
    	return 4;
    }

    fprintf(stderr, "CONNECTED to %s %d\n", argv[1], port);

    char buff[1025];

    size_t sz = sizeof(buff) -1;
    size_t n_bytes= 0;

    while (sz != 0) {
        size_t ret = read (0, buff + n_bytes, sz);
        if (!ret) break; //EOF
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror ("ERROR: read");
            return 5;
        }
        sz -= ret;
        n_bytes += ret;
    }

    //fprintf(stderr, "INFO: read %lu bytes\n", n_bytes);

    sz = 0;
    while (n_bytes) {
        size_t ret = write(sock_fd, buff + sz, n_bytes);
        if (ret  == -1) {
            if (errno == EINTR) continue;
            perror("ERROR: write");
            return 6;
        }
        n_bytes -= ret;
        sz += ret;
    }

    buff[sz] = 0;
    fprintf(stderr, "INFO: sent %lu bytes: %s\n", sz, buff);
    
    if (-1 == shutdown(sock_fd, SHUT_WR)) {
        perror("ERROR: could not shutdown WR socket");
        return 6;
    }

    if (-1 == dup2(sock_fd, 0)) {
        perror("ERROR: could not set stdin");
        return 7;
    }

    fprintf(stderr, "INFO: starting %s", argv[3]);
    for(int i = 4; i < argc; ++i) fprintf(stderr, " %s", argv[i]);
    fprintf(stderr, "\n");

    if (-1 == execvp(argv[3], argv+4)) {
        fprintf(stderr, "ERROR: could not start cmd %s: errno %d: %s\n", argv[3], errno, strerror(errno));
        return 8;
    }

    return 0;
}
