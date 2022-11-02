// Server program
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sstream>
#include <string>
#define MAX 1024

using namespace std;

/* ./tcp_server {port} */
int main(int argc, char *argv[]) {
	int listenfd, connfd, maxfdp;
	char buffer[MAX];
	fd_set rset;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;

	// create listening TCP socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, 10);
    cout << "TCP server is running\n";

	maxfdp = listenfd + 1;
	while (true) {
		bzero(buffer, MAX);
		FD_SET(listenfd, &rset);
		select(maxfdp, &rset, NULL, NULL, NULL);
		len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
		maxfdp = connfd + 1;
		cout << "New connection\n";

		while (strcmp(buffer, "exit\n") != 0) {
			bzero(buffer, MAX);
			read(connfd, buffer, sizeof(buffer));
			printf("Client: %s", buffer);
			write(connfd, buffer, sizeof(buffer));
		}
	}

	close(connfd);
	close(listenfd);
}
