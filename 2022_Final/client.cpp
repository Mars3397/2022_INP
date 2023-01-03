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
#define MAXLINE 1024

using namespace std;

int main(int argc, char *argv[]) {
	int tcpsockfd, udpsockfd;
	char buffer[MAXLINE];
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
		printf("Error : TCP connect failed \n");
		return 0;
	}

	// read and print welcoming message
	memset(buffer, 0, sizeof(buffer));
	read(tcpsockfd, buffer, sizeof(buffer));
	printf("%s", buffer);

	string arg;
	vector<string> args;

	pid_t pid = fork();
	if (pid > 0) {
		// Parent process: loop for write
		while (1) {
			bzero(buffer, sizeof(buffer));
			// read and split the command
			// cout << "% ";
			fgets(buffer, MAXLINE, stdin);
			args.clear();
			stringstream ss(buffer);
			while (getline(ss, arg, ' ')) {
				if (arg != "\n") {
					if (arg[arg.size()-1] == '\n')
						arg.pop_back();
					args.push_back(arg);
				}
			}
			int l = args.size();
			// exit
			if ((args[0] == "exit")) {
				kill(pid, SIGTERM);
				break;
			} 
			write(tcpsockfd, buffer, sizeof(buffer));
		}
	} else {
		// Child process: loop for read
		while (1) {
			bzero(buffer, sizeof(buffer));
			read(tcpsockfd, buffer, sizeof(buffer));
			cout << buffer;
		}
	}
	close(tcpsockfd);
    return 0;
}
