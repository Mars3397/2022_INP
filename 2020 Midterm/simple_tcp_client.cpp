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

void func(int tcpsockfd) {
	char buffer[MAX];
	while (strcmp(buffer, "exit\n") != 0) {
		bzero(buffer, MAX);
        printf("%% ");
        fgets(buffer, MAX, stdin);
        write(tcpsockfd, buffer, sizeof(buffer));
		read(tcpsockfd, buffer, sizeof(buffer));
	}
}

/* ./tcp_client 127.0.0.1 {port} */
int main(int argc, char *argv[]) {
	int tcpsockfd;
	char buffer[MAX];
	struct sockaddr_in servaddr;

	// Creating TCP socket file descriptor
	if ((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("TCP socket creation failed");
		exit(0);
	}
	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	// connect to server using TCP
	if (connect(tcpsockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("\n Error : TCP connect failed \n");
		return 0;
	}

    func(tcpsockfd);
	close(tcpsockfd);
    return 0;
}

