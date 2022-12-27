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
 
struct users {
    char username[20][100];
    int login_fd[20];
    bool mute[20];
    int count_u;
};

int main(int argc, char *argv[]) {
    int listenfd, connfd, udpfd, nready, maxfd;
	char buffer[MAXLINE];
	fd_set rset;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

    int max_clients = 20, client_fds[max_clients], client_index[max_clients];
    for (int i = 0; i < max_clients; i++) {
        client_fds[i] = 0;
        client_index[i] = -1;
    }

    // shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct users), IPC_CREAT | 0600);
    void *shm = shmat(shmid, NULL, 0);
    struct users* users_info = (struct users*)shm;
    users_info->count_u = 0;

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
            // printf("New connection , socket fd is %d, ip is : %s, port : %d\n", connfd , inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port));
            for (int i = 0; i < max_clients; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = connfd;
                    int index = users_info->count_u;
                    users_info->count_u++;
                    client_index[i] = index;
                    users_info->login_fd[index] = connfd;
                    users_info->mute[index] = false;
                    string temp = "**************************\n* Welcome to the BBS server. *\n**************************\n";
                    temp += "Welcome, user" + to_string(index) + ".\n";
                    char msg[MAXLINE]; strcpy(msg, temp.c_str());
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
                    users_info->login_fd[client_index[f]] = 0;
                    close(client_fds[f]);
                    client_fds[f] = 0;
                    client_index[f] = -1;
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
                    // mute
                    if (args[0] == "mute") {
                        if (users_info->mute[f]) {
                            char msg[MAXLINE] = "Yor are already in mute mode.\n";
                            write(client_fds[f], msg, strlen(msg));
                        } else {
                            users_info->mute[f] = true;
                            char msg[MAXLINE] = "Mute mode.\n";
                            write(client_fds[f], msg, strlen(msg));
                        }
                    } else if (args[0] == "unmute") {
                        if (!users_info->mute[f]) {
                            char msg[MAXLINE] = "Yor are already in unmute mode.\n";
                            write(client_fds[f], msg, strlen(msg));
                        } else {
                            users_info->mute[f] = false;
                            char msg[MAXLINE] = "Unmute mode.\n";
                            write(client_fds[f], msg, strlen(msg));
                        }
                    } else if (args[0] == "yell") {
                        string temp = "user" + to_string(client_index[f]) + ": ";
                        for (int i = 1; i < l; i++) {
                            temp += args[i] + " ";
                        }
                        temp += "\n";
                        char msg[MAXLINE]; strcpy(msg, temp.c_str());
                        for (int i = 0; i < users_info->count_u; i++) {
                            if (i != f && users_info->login_fd[i] != 0 && users_info->mute[i] == false) {
                                write(client_fds[i], msg, strlen(msg));
                            }
                        }
                    } else if (args[0] == "tell") {
                        int receiver_index = stoi(args[1].substr(4));
                        if (users_info->mute[receiver_index] == false) {
                            string temp = "user" + to_string(client_index[f]) + " told you: ";
                            for (int i = 2; i < l; i++) {
                                temp += args[i] + " ";
                            }
                            temp += "\n";
                            char msg[MAXLINE]; strcpy(msg, temp.c_str());
                            write(users_info->login_fd[receiver_index], msg, strlen(msg));
                        }
                    } else if (args[0] == "exit") {
                        users_info->login_fd[client_index[f]] = 0;
                    }
                }
            }
        }
    }
    
    return 0;
}