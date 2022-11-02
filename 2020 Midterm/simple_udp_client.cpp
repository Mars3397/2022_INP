// TCP Client program
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <strings.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <string>
#define MAX 1024

using namespace std;

int main(int argc, char *argv[]) {
	int udpsockfd;
	char buffer[MAX];
	struct sockaddr_in servaddr;

	// create UDP socket file descriptor
	if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("UDP socket creation failed");
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    while (true) {
        bzero(buffer, sizeof(buffer));
        cout << "% ";
        fgets(buffer, MAX, stdin);
        sendto(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }
    }

    close(udpsockfd);

    return 0;
}

