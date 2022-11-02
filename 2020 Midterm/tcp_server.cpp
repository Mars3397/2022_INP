// Server program
#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
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
#include <sys/shm.h>
#define MAX 1024

using namespace std;

// struct data to store in shared memory
struct data {
	int count;
    bool login[100];
};

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

	// shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct data), IPC_CREAT | 0600);
    void *shm = shmat(shmid, NULL, 0);
    struct data* database = (struct data*)shm;
	database->count = 0;

	maxfdp = listenfd + 1;
	while (true) {
		bzero(buffer, MAX);
		FD_SET(listenfd, &rset);
		select(maxfdp, &rset, NULL, NULL, NULL);
		len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

		if (!fork()) {
			maxfdp = connfd + 1;
			int uid = ++database->count;
			database->login[uid] = true;
			cout << "New connection from " << inet_ntoa(cliaddr.sin_addr) << ":";
			cout << ntohs(cliaddr.sin_port) << " user" << database->count << "\n";
			string s = "Welcome, you are user" + to_string(uid) + "\n";
			write(connfd, s.c_str(), sizeof(s));

			while (true) {
				bzero(buffer, MAX);
				read(connfd, buffer, sizeof(buffer));
				// printf("Client: %s", buffer);
				if (strcmp(buffer, "exit\n") == 0) {
					database->login[uid] = false;
					cout << "user" << uid << " ";
					cout << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << " disconnected\n";
					string s = "Sucess: \nBye, user" + to_string(uid) + "\n";
					write(connfd, s.c_str(), sizeof(s));
					break;
				} else if (strcmp(buffer, "list-users\n") == 0) {
					string s = "Success:\n";
					for (int i = 1; i <= database->count; i++) {
						if (database->login[i]) {
							s += "user" + to_string(i) + "\n";
						}
					}
					write(connfd, s.c_str(), sizeof(s));
				} else {
					string s = "IP: " + string(inet_ntoa(cliaddr.sin_addr)) + ":" 
							+ to_string(ntohs(cliaddr.sin_port)) + "\n";
					write(connfd, s.c_str(), sizeof(s));
				}
			}
		}
	}
	close(listenfd);
	close(connfd);
}
