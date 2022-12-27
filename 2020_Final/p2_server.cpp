#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <tuple>
#include <algorithm>

using namespace std;

#define MAXLINE 1024
#define PORT 8888
 
struct accounts {
    int balance[2];
};

int main(int argc, char *argv[]) {
    int listenfd, connfd, udpfd, nready, maxfd, current_clients = 0;
	char buffer[MAXLINE];
	fd_set rset;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

    int max_clients = 4, client_fds[max_clients], client_number[max_clients];
    for (int i = 0; i < max_clients; i++) {
        client_fds[i] = 0;
        client_number[i] = -1;
    }

    // shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct accounts), IPC_CREAT | 0600);
    void *shm = shmat(shmid, NULL, 0);
    struct accounts* accounts_info = (struct accounts*)shm;
    accounts_info->balance[0] = 0;
    accounts_info->balance[1] = 0;

	// create listening TCP socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // set server to SO_REUSEADDR
	const int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("setsockopt(SO_REUSEADDR) failed");
		exit(0);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, 20);

    cout << "Server is running" << endl;

    while (true) {
        bzero(buffer, MAXLINE); 
        // clear the descriptor set
	    FD_ZERO(&rset);
        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);

        // get maxfd
	    maxfd = listenfd;
        // push connect fd in to ready set
        for (int i = 0; i < max_clients; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &rset);
            }
            if (client_fds[i] > maxfd) {
                maxfd = client_fds[i];
            }
        }

        // wait until a fd is ready
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        len = sizeof(cliaddr);

        string arg;
        vector<string> args;

        // new TCP connection -> add to client_fds
        if (FD_ISSET(listenfd, &rset)) {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            cout << "New connection from " << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port);
            cout << " (user" << (char)(current_clients + 'A') << ")" << endl;
            // printf("New connection , socket fd is %d, ip is : %s, port : %d\n", connfd , inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port));
            for (int i = 0; i < max_clients; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = connfd;
                    client_number[i] = current_clients;
                    current_clients++;
                    char msg[MAXLINE] = "**************************\n* Welcome to the TCP server. *\n**************************\n";
                    write(client_fds[i], msg, strlen(msg));
                    break;
                }
            }
        }

        for (int f = 0; f < max_clients; f++) {
            if (FD_ISSET(client_fds[f], &rset)) {
                bzero(buffer, MAXLINE); int test;
                // if a sd is set but send nothing -> client disconnect
                if ((test = read(client_fds[f], buffer, sizeof(buffer))) == 0) {
                    cout << "(user" << (char)(client_number[f] + 'A') << ") ";
                    cout << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << " disconnected\n";
                    close(client_fds[f]);
                    client_fds[f] = 0;
                    client_number[f] = -1;
                } else {
                    // split the input
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
                    // actions associated with each command
                    cout << "From client " << f << ": " << buffer;
                    // show-accounts
                    if (args[0] == "show-accounts") {
                        if (l == 1) {
                            string temp = "ACCOUNT1: " + to_string(accounts_info->balance[0]) + "\n";
                            temp += "ACCOUNT2: " + to_string(accounts_info->balance[1]) + "\n";
                            char msg[MAXLINE]; strcpy(msg, temp.c_str());
                            write(client_fds[f], msg, strlen(msg));
                        } else {
                            char msg[MAXLINE] = "Usage: show-acconts\n";
                            write(client_fds[f], msg, strlen(msg));
                        }
                    } else if (args[0] == "deposit") {
                        if (l == 3 && (args[1] == "ACCOUNT1" || args[1] == "ACCOUNT2")) {
                            int money = stoi(args[2]);
                            if (money <= 0) {
                                char msg[MAXLINE] = "Deposit a non-positive number into accounts.\n";
                                write(client_fds[f], msg, strlen(msg));
                            } else {
                                if (args[1] == "ACCOUNT1") {
                                    accounts_info->balance[0] += money;
                                } else {
                                    accounts_info->balance[1] += money;
                                }
                                string temp = "Successfully deposits " + to_string(money) + " into " + args[1] + ".\n";
                                char msg[MAXLINE]; strcpy(msg, temp.c_str());
                                write(client_fds[f], msg, strlen(msg));
                            }
                        } else {
                            char msg[MAXLINE] = "Usage: deposit <ACCOUNT1|ACCOUNT2> <money>\n";
                            write(client_fds[f], msg, strlen(msg));
                        }
                    } else if (args[0] == "withdraw") {
                        if (l == 3 && (args[1] == "ACCOUNT1" || args[1] == "ACCOUNT2")) {
                            int money = stoi(args[2]);
                            if (money <= 0) {
                                char msg[MAXLINE] = "Withdraw a non-positive number into accounts.\n";
                                write(client_fds[f], msg, strlen(msg));
                            } else {
                                if (args[1] == "ACCOUNT1") {
                                    if (money > accounts_info->balance[0]) {
                                        char msg[MAXLINE] = "Withdraw excess money from accounts.\n";
                                        write(client_fds[f], msg, strlen(msg));
                                    } else {
                                        accounts_info->balance[0] -= money;
                                        string temp = "Successfully withdraws " + to_string(money) + " from " + args[1] + ".\n";
                                        char msg[MAXLINE]; strcpy(msg, temp.c_str());
                                        write(client_fds[f], msg, strlen(msg));
                                    }
                                } else {
                                    if (money > accounts_info->balance[1]) {
                                        char msg[MAXLINE] = "Withdraw excess money from accounts.\n";
                                        write(client_fds[f], msg, strlen(msg));
                                    } else {
                                        accounts_info->balance[1] -= money;
                                        string temp = "Successfully withdraws " + to_string(money) + " from " + args[1] + ".\n";
                                        char msg[MAXLINE]; strcpy(msg, temp.c_str());
                                        write(client_fds[f], msg, strlen(msg));
                                    }
                                }
                            }
                        } else {
                            char msg[MAXLINE] = "Usage: withdraw <ACCOUNT1|ACCOUNT2> <money>\n";
                            write(client_fds[f], msg, strlen(msg));
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}