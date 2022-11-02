// Server program
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#define MAXLINE 1024

using namespace std;

int main(int argc, char *argv[]) {
	int udpfd;
	char buffer[MAXLINE];
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	// create UDP socket 
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    cout << "UDP server is running\n";

    while (true) {
        bzero(buffer, sizeof(buffer));
        len = sizeof(cliaddr);
        recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
        cout << buffer;
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }
    }

	close(udpfd);
}
