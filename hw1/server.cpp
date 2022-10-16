// Server program
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
#define MAXLINE 1024

using namespace std;

// struct data to store in shared memory
struct data {
    char username[100][100];
    char email[100][100];
    char password[100][100];
    int login_process[100];
};

int max(int x, int y) {
	if (x > y) return x;
	else return y;
}

// check 4 bit number
bool check_char(const char buff[]) {
    bool ans = true;
    for (int i = 0; i < 4; i++) {
        if (!isdigit(buff[i])) {
            return false;
        }
    }
    if (ans && buff[4] == '\n') {
        return true;
    }
    return false;
}

// get xAxB 
string get_hint(char buff[], char ans[]) {
    bool arr[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int a = 0, b = 0;
    for (int i = 0; i < 4; i++) {
        if (buff[i] == ans[i]) {
            a++;
            arr[buff[i] - '0'] = 1;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (arr[buff[i] - '0']) {
            continue;
        } else {
            arr[buff[i] - '0'] = 1;
        }
        for (int j = i + 1; j < i + 4; j++) {
            if (buff[i] == ans[j % 4]) {
                b++; arr[buff[i] - '0'] = 1;
                break;
            }
        }
    }
    return to_string(a) + "A" + to_string(b) + "B\n";
}

int main(int argc, char *argv[]) {
	int listenfd, connfd, udpfd, nready, maxfdp;
	char buffer[MAXLINE];
	fd_set rset;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;
	char message[MAXLINE] = "*****Welcome to Game 1A2B*****\n";
	void sig_chld(int);

    // shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct data), IPC_CREAT | 0600);
    void *shm = shmat(shmid, NULL, 0);
    struct data* database = (struct data*)shm;
    int* count_u = (int*) mmap(NULL, sizeof (int) , PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *count_u = 0;

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
	listen(listenfd, 10);
    cout << "TCP server is running\n";

	// create UDP socket 
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    cout << "UDP server is running\n";

	// clear the descriptor set
	FD_ZERO(&rset);

	// get maxfd
	maxfdp = max(listenfd, udpfd) + 1;
	while (1) {
        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        // wait until a fd is ready
        nready = select(maxfdp, &rset, NULL, NULL, NULL);
        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

        // fork a process to deal with a client
        if (!fork()) {
            // New connection and welcoming message
            cout << "New connection\n";
            write(connfd, message, sizeof(buffer));
            string arg;
	        vector<string> args;
            // a loop to tackle with a client's input
            while (1) {
                // push connfd and udpfd to ready set and select among them
                FD_SET(connfd, &rset);
                FD_SET(udpfd, &rset);
                maxfdp = max(connfd, maxfdp) + 1;
                nready = select(maxfdp, &rset, NULL, NULL, NULL);
                // message sent by TCP 
                if (FD_ISSET(connfd, &rset)) {
                    bzero(buffer, MAXLINE);
                    read(connfd, buffer, sizeof(buffer));
                    // split the input
                    args.clear();
                    stringstream ss(buffer);
                    while (getline(ss, arg, ' ')) {
                        if (arg != "\n")
                            args.push_back(arg);
                    }
                    int l = args.size();
                    // actions associated with each command
                    // exit
                    if (args[0] == "exit\n" || args[0] == "exit" || buffer[0] == '\0') {
                        if (l == 1 || l == 0) {
                            printf("tcp get msg: exit\n"); 
                            break;
                        } else {
                            char mr[MAXLINE] = "Usage: exit\n";
                            write(connfd, mr, sizeof(buffer));
                        }
                    } 
                    // login
                    else if (args[0] == "login\n" || args[0] == "login") {
                        // correct format
                        if (l == 3) {
                            // already login or not
                            bool login_flag = false;
                            for (int i = 0; i < *count_u; i++) {
                                if (database->login_process[i] == getpid()) {
                                    login_flag = true;
                                    break;
                                }
                            }
                            if (login_flag) {
                                char mr[MAXLINE] = "Please logout first.\n";
                                write(connfd, mr, sizeof(buffer));
                                continue;
                            }
                            // find username and check password
                            bool user_flag = false, passwd_flag = false;
                            int index = 0;
                            for (int i = 0; i < *count_u; i++) {
                                if (strcmp(database->username[i], args[1].c_str()) == 0) {
                                    user_flag = true;
                                    if (strcmp(database->password[i], args[2].c_str()) == 0) {
                                        passwd_flag = true;
                                    }
                                    index = i;
                                    break;
                                }
                            }
                            if (user_flag && passwd_flag) {
                                // prevent 2 clients login a same user
                                if (database->login_process[index] != 0) {
                                    char mr[MAXLINE] = "Please logout first.\n";
                                    write(connfd, mr, sizeof(buffer));
                                } else {
                                    database->login_process[index] = getpid();
                                    string temp = "Welcome " + string(database->username[index]) + "\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(connfd, mr, sizeof(buffer));
                                }
                            } else if (user_flag) {
                                char mr[MAXLINE] = "Password not correct\n";
                                write(connfd, mr, sizeof(buffer));
                            } else {
                                char mr[MAXLINE] = "Username not found.\n";
                                write(connfd, mr, sizeof(buffer));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: login <username> <password>\n";
                            write(connfd, mr, sizeof(buffer));
                        }
                    } 
                    // logout
                    else if (args[0] == "logout\n" || args[0] == "logout") {
                        // correct format
                        if (l == 1) {
                            // login now or not
                            bool can_logout = false; int index = 0;
                            for (int i = 0; i < *count_u; i++) {
                                if (database->login_process[i] == getpid()) {
                                    can_logout = true;
                                    index = i;
                                    break;
                                }
                            }
                            if (can_logout) {
                                database->login_process[index] = 0;
                                string temp = "Bye, " + string(database->username[index]) + "\n";
                                char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                write(connfd, mr, sizeof(buffer));
                            } else {
                                char mr[MAXLINE] = "Please login first.\n";
                                write(connfd, mr, sizeof(buffer));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: logout\n";
                            write(connfd, mr, sizeof(buffer));
                        }
                    } 
                    // start-game
                    else if (args[0] == "start-game\n" || args[0] == "start-game") {
                        // correct format
                        if (l == 1 || l == 2) {
                            // check login or not
                            bool login_flag = false;
                            for (int i = 0; i < *count_u; i++) {
                                if (database->login_process[i] == getpid()) {
                                    login_flag = true;
                                    break;
                                }
                            }
                            if (login_flag) {
                                // use default answer "1234"
                                if (l == 1) {
                                    char mr[MAXLINE] = "Please typing a 4-digit number:\n";
                                    write(connfd, mr, sizeof(buffer));
                                    int count = 5; 
                                    char ans[MAXLINE] = "1234\n";
                                    // loop for guess
                                    while (true) {
                                        read(connfd, buffer, sizeof(buffer));
                                        // check guess is 4 bit number or not
                                        if (!check_char(buffer)) {
                                            char mr[MAXLINE] = "Your guess should be a 4-digit number.\n";
                                            write(connfd, mr, sizeof(buffer));
                                            continue;
                                        }
                                        // get the answer
                                        if (strncmp(buffer, ans, 5) == 0) {
                                            char mr[MAXLINE] = "You got the answer!\n";
                                            write(connfd, mr, sizeof(buffer));
                                            break;
                                        }
                                        // guess more than 5 time
                                        if (--count == 0) {
                                            char mr[MAXLINE] = "You lose the game!\n";
                                            write(connfd, mr, sizeof(buffer));
                                            char mr1[MAXLINE]; strcpy(mr1, get_hint(buffer, ans).c_str());
                                            write(connfd, mr1, sizeof(buffer));
                                            char mr2[MAXLINE] = "You lose the game!\n";
                                            write(connfd, mr2, sizeof(buffer));
                                            break;
                                        }
                                        // return xAxB hint
                                        char mr[MAXLINE]; strcpy(mr, get_hint(buffer, ans).c_str());
                                        write(connfd, mr, sizeof(buffer));
                                    }
                                } else {
                                    // specify the answer
                                    // check the specified answer is 4 bit number or not
                                    if (!check_char(args[1].c_str())) {
                                        char mr[MAXLINE] = "Usage: start-game <4-digit number>\n";
                                        write(connfd, mr, sizeof(buffer));
                                        continue;
                                    }
                                    char mr[MAXLINE] = "Please typing a 4-digit number:\n";
                                    write(connfd, mr, sizeof(buffer));
                                    int count = 5; 
                                    char ans[MAXLINE]; strcpy(ans, args[1].c_str());
                                    // loop for guess
                                    while (true) {
                                        read(connfd, buffer, sizeof(buffer));
                                        // check guess is 4 bit number or not
                                        if (!check_char(buffer)) {
                                            char mr[MAXLINE] = "Your guess should be a 4-digit number.\n";
                                            write(connfd, mr, sizeof(buffer));
                                            continue;
                                        }
                                        // get the answer
                                        if (strncmp(buffer, ans, 5) == 0) {
                                            char mr[MAXLINE] = "You got the answer!\n";
                                            write(connfd, mr, sizeof(buffer));
                                            break;
                                        }
                                        // guess more than 5 time
                                        if (--count == 0) {
                                            char mr[MAXLINE] = "You lose the game!\n";
                                            write(connfd, mr, sizeof(buffer));
                                            char mr1[MAXLINE]; strcpy(mr1, get_hint(buffer, ans).c_str());
                                            write(connfd, mr1, sizeof(buffer));
                                            char mr2[MAXLINE] = "You lose the game!\n";
                                            write(connfd, mr2, sizeof(buffer));
                                            break;
                                        }
                                        // return xAxB hint
                                        char mr[MAXLINE]; strcpy(mr, get_hint(buffer, ans).c_str());
                                        write(connfd, mr, sizeof(buffer));
                                    }
                                }
                            } 
                            // haven't login
                            else {
                                char mr[MAXLINE] = "Please login first.\n";
                                write(connfd, mr, sizeof(buffer));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: start-game <4-digit number>\n";
                            write(connfd, mr, sizeof(buffer));
                        }
                    }
                }
                // message sent by UDP
                if (FD_ISSET(udpfd, &rset)) {
                    bzero(buffer, sizeof(buffer));
                    n = recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
                    // split the input
                    args.clear();
                    stringstream ss(buffer);
                    while (getline(ss, arg, ' ')) {
                        if (arg != "\n")
                            args.push_back(arg);
                    }
                    int l = args.size();
                    // actions associated with each command
                    // register
                    if (args[0] == "register\n" || args[0] == "register") {
                        // correct format
                        if (l == 4) {
                            // check username and email
                            bool f = true;
                            for (int i = 0; i < *count_u; i++) {
                                if (strcmp(database->username[i], args[1].c_str()) == 0) {
                                    char rf1[MAXLINE] = "Username is already used.\n";
                                    sendto(udpfd, rf1, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                                    f = false; break;
                                }
                                if (strcmp(database->email[i], args[2].c_str()) == 0) {
                                    char rf2[MAXLINE] = "Email is already used.\n";
                                    sendto(udpfd, rf2, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                                    f = false; break;
                                }
                            }
                            // insert use info to database
                            if (f) {
                                strcpy(database->username[*count_u], args[1].c_str());
                                strcpy(database->email[*count_u], args[2].c_str());
                                strcpy(database->password[*count_u], args[3].c_str());
                                database->login_process[*count_u] = 0;
                                *count_u = *count_u + 1;
                                char mr[MAXLINE] = "Register successfully.\n";
                                sendto(udpfd, mr, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: register <username> <email> <password>\n";
                            sendto(udpfd, mr, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                        }
                    } 
                    // game-rule
                    else if (args[0] == "game-rule\n" || args[0] == "game-rule") {
                        // correct format
                        if (l == 1) {
                            // send game rule
                            char mr[MAXLINE] =  "1.     Each question is a 4-digit secret number.\n"
                                                "2.     After each guess, you will get a hint with the following information:\n"
                                                "2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n"
                                                "2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\n"
                                                "The hint will be formatted as \"xAyB\".\n"
                                                "3.     5 chances for each question.\n";
                            sendto(udpfd, mr, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: game-rule\n";
                            sendto(udpfd, mr, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                        }
                    }
                }
            }
            close(connfd);
            exit(0);
        }
        close(connfd);
	}
}
