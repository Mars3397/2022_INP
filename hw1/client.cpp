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
	char message[MAXLINE] = "New connection\n";
	struct sockaddr_in servaddr;

	// Creating socket file descriptor
	if ((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("TCP socket creation failed");
		exit(0);
	}
	if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("UDP socket creation failed");
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(tcpsockfd, (struct sockaddr*)&servaddr,
							sizeof(servaddr)) < 0) {
		printf("\n Error : TCP connect failed \n");
		return 0;
	}

	memset(buffer, 0, sizeof(buffer));
	read(tcpsockfd, buffer, sizeof(buffer));
	printf("%s", buffer);

	string arg;
	vector<string> args;
    while (1) {
        bzero(buffer, sizeof(buffer));
        printf("%% ");
        fgets(buffer, MAXLINE, stdin);
		args.clear();
		stringstream ss(buffer);
		while (getline(ss, arg, ' ')) {
			args.push_back(arg);
		}
		int l = args.size();
		if (args[0] == "register\n" || args[0] == "register") {
			sendto(udpsockfd, buffer, sizeof(buffer), 0,
                (struct sockaddr*)&servaddr, sizeof(servaddr));
			socklen_t len = sizeof(servaddr);
			bzero(buffer, sizeof(buffer));
			recvfrom(udpsockfd, buffer, sizeof(buffer), 0,
                (struct sockaddr*)&servaddr, &len);
			printf("%s", buffer);
		} 
		else if (args[0] == "login\n" || args[0] == "login") {
			write(tcpsockfd, buffer, sizeof(buffer));
			read(tcpsockfd, buffer, sizeof(buffer));
			printf("%s", buffer);
        } 
		else if (args[0] == "logout\n" || args[0] == "logout") {
			write(tcpsockfd, buffer, sizeof(buffer));
			read(tcpsockfd, buffer, sizeof(buffer));
			printf("%s", buffer);
        } 
		else if (args[0] == "game-rule\n" || args[0] == "game-rule") {
			sendto(udpsockfd, buffer, sizeof(buffer), 0,
                (struct sockaddr*)&servaddr, sizeof(servaddr));
			socklen_t len = sizeof(servaddr);
			bzero(buffer, sizeof(buffer));
			recvfrom(udpsockfd, buffer, sizeof(buffer), 0,
                (struct sockaddr*)&servaddr, &len);
			printf("%s", buffer);
        } 
		else if (args[0] == "start-game\n" || args[0] == "start-game") {
			write(tcpsockfd, buffer, sizeof(buffer));
			read(tcpsockfd, buffer, sizeof(buffer));
			printf("%s", buffer);
			bool f = true;
			if (strncmp(buffer, "Usage: start-game <4-digit number>\n", 35) == 0) {
				f = false;
			}
			while (f) {
				bzero(buffer, sizeof(buffer));
				printf("guess: ");
				fgets(buffer, MAXLINE, stdin);
				write(tcpsockfd, buffer, sizeof(buffer));
				read(tcpsockfd, buffer, sizeof(buffer));
				if (strncmp(buffer, "You lose the game!\n", 19) == 0) {
					read(tcpsockfd, buffer, sizeof(buffer));
					printf("%s", buffer);
					bzero(buffer, sizeof(buffer));
					read(tcpsockfd, buffer, sizeof(buffer));
					printf("%s", buffer);
					break;
				} 
				printf("%s", buffer);
				if (strncmp(buffer, "You got the answer!\n", 20) == 0) {
					break;
				} 
			}
        } 
		else if ((args[0] == "exit\n" || args[0] == "exit") && l == 1) {
			write(tcpsockfd, buffer, sizeof(buffer));
            break; 
        } 
		else {
			cout << "No such command\n";
		}
    }

	close(tcpsockfd);
	close(udpsockfd);
    return 0;
}

