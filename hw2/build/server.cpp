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
#include <sys/time.h>
#include <tuple>
#include <algorithm>

#define MAXLINE 1024
#define PORT 8888

using namespace std;

// struct data to store in shared memory
struct data {
    char username[100][100];
    char email[100][100];
    char password[100][100];
    int login_fd[100];
    int room_id[100];
    int count_u;
};

struct rooms {
    char owner[100][100];
    char type[100][100];
    int id[100];
    bool status [100]; // false: open, true: playing
    uint32_t code[100];
    int count_r;
};

string list_rooms(struct rooms* room_data) {
    vector<tuple<int, string, bool>> v; // id, type, status
    for (int i = 0; i < room_data->count_r; i++) {
        v.push_back(make_tuple(room_data->id[i], string(room_data->type[i]), room_data->status[i]));
    }
    sort(v.begin(), v.end());
    string ret = "List Game Rooms\n";
    for (int i = 0; i < v.size(); i++) {
        ret += to_string(i+1) + ". (" + get<1>(v[i]) + ") Game Room ";
        if (get<2>(v[i])) {
            ret += to_string(get<0>(v[i])) + " has started playing\n";
        } else {
            ret += to_string(get<0>(v[i])) + " is open for players\n";
        }
    }
    return ret;
}

string list_users(struct data* database) {
    vector<tuple<string, string, string>> v; // name, email, status
    for (int i = 0; i < database->count_u; i++) {
        if (database->login_fd[i] != 0) {
            v.push_back(make_tuple(string(database->username[i]), string(database->email[i]), " Online\n"));
        } else {
            v.push_back(make_tuple(string(database->username[i]), string(database->email[i]), " Offline\n"));
        }
    }
    sort(v.begin(), v.end());
    string ret = "List Users\n";
    for (int i = 0; i < v.size(); i++) {
        ret += to_string(i+1) + ". " + get<0>(v[i]) + "<" + get<1>(v[i]) + ">" + get<2>(v[i]);
    }
    return ret;
}

int max(int x, int y) {
	if (x > y) return x;
	else return y;
}

// check 4 bit number
bool check_char(const char buff[]) {
    if (strlen(buff) != 4) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        if (!isdigit(buff[i])) {
            return false;
        }
    }
    return true;
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
	int listenfd, connfd, udpfd, nready, maxfd;
	char buffer[MAXLINE];
	fd_set rset;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;
	void sig_chld(int);

    int client_fds[20], max_clients = 20;
    for (int i = 0; i < max_clients; i++) {
        client_fds[i] = 0;
    }

    // shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct data), IPC_CREAT | 0600);
    void *shm = shmat(shmid, NULL, 0);
    struct data* database = (struct data*)shm;
    database->count_u = 0;

    int shmid1 = shmget(IPC_PRIVATE, sizeof(struct rooms), IPC_CREAT | 0600);
    void *shm1 = shmat(shmid1, NULL, 0);
    struct rooms* room_data = (struct rooms*)shm1;
    room_data->count_r = 0;

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
	servaddr.sin_port = htons(PORT);

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, 20);

	// create UDP socket 
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    cout << "Server is running\n";

	while (1) {
        // clear the descriptor set
	    FD_ZERO(&rset);
        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        // get maxfd
	    maxfd = max(listenfd, udpfd);
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
                    for (int i = 0; i < database->count_u; i++) {
                        if (strcmp(database->username[i], args[1].c_str()) == 0 || 
                            strcmp(database->email[i], args[2].c_str()) == 0) {
                            char rf1[MAXLINE] = "Username or Email is already used\n";
                            sendto(udpfd, rf1, strlen(rf1), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                            f = false; break;
                        }
                    }
                    // insert use info to database
                    if (f) {
                        strcpy(database->username[database->count_u], args[1].c_str());
                        strcpy(database->email[database->count_u], args[2].c_str());
                        strcpy(database->password[database->count_u], args[3].c_str());
                        database->login_fd[database->count_u] = 0;
                        database->room_id[database->count_u] = -1;
                        database->count_u = database->count_u + 1;
                        char mr[MAXLINE] = "Register Successfully\n";
                        sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                    }
                } 
                // incorrect format
                else {
                    char mr[MAXLINE] = "Usage: register <username> <email> <password>\n";
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                }
            } 
            // game-rule
            else if (args[0] == "game-rule\n" || args[0] == "game-rule") {
                // correct format
                if (l == 1) {
                    // send game rule
                    char mr[MAXLINE] =  "1. Each question is a 4-digit secret number.\n"
                                        "2. After each guess, you will get a hint with the following information:\n"
                                        "2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n"
                                        "2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\n"
                                        "The hint will be formatted as \"xAyB\".\n"
                                        "3. 5 chances for each question.\n";
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                } 
                // incorrect format
                else {
                    char mr[MAXLINE] = "Usage: game-rule\n";
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                }
            }
            // list users
            else if (args[0] == "list" && (args[1] == "users" || args[1] == "users\n")) {
                if (database->count_u == 0) {
                    char mr[MAXLINE] = "List Users\nNo Users\n";
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                } else {
                    string temp = list_users(database);
                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                }
            }
            // list rooms
            else if (args[0] == "list" && (args[1] == "rooms" || args[1] == "rooms\n")) {
                if (room_data->count_r == 0) {
                    char mr[MAXLINE] = "List Game Rooms\nNo Rooms\n";
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                } else {
                    string temp = list_rooms(room_data);
                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                    sendto(udpfd, mr, strlen(mr), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                }
            }
        }

        // new TCP connection -> add to client_fds
        if (FD_ISSET(listenfd, &rset)) {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            printf("New connection , socket fd is %d, ip is : %s, port : %d\n", connfd , inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port));
            for (int i = 0; i < max_clients; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = connfd;
                    break;
                }
            }
        }

        // message sent by connected TCP (in client_fds)
        for (int f = 0; f < max_clients; f++) {
            if (FD_ISSET(client_fds[f], &rset)) {
                bzero(buffer, MAXLINE); int test;
                // if a sd is set but send nothing -> client disconnect
                if ((test = read(client_fds[f], buffer, sizeof(buffer))) == 0) {
                    close(client_fds[f]);
                    client_fds[f] = 0;
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
                    // exit
                    if (args[0] == "exit\n" || args[0] == "exit" || buffer[0] == '\0') {
                        if (l == 1 || l == 0) {
                            for (int i = 0; i < database->count_u; i++) {
                                if (database->login_fd[i] == client_fds[f]) {
                                    database->login_fd[i] = 0;
                                    break;
                                }
                            }
                            printf("(server) client exit\n"); 
                            break;
                        } else {
                            char mr[MAXLINE] = "Usage: exit\n";
                            write(client_fds[f], mr, strlen(mr));
                        }
                    } 
                    // login
                    else if (args[0] == "login\n" || args[0] == "login") {
                        // correct format
                        if (l == 3) {
                            // already login or not
                            string login_name;
                            bool login_flag = false;
                            for (int i = 0; i < database->count_u; i++) {
                                if (database->login_fd[i] == client_fds[f]) {
                                    login_flag = true;
                                    login_name = string(database->username[i]);
                                    break;
                                }
                            }
                            // find username and check password
                            bool user_flag = false, passwd_flag = false;
                            int index = 0;
                            for (int i = 0; i < database->count_u; i++) {
                                if (strcmp(database->username[i], args[1].c_str()) == 0) {
                                    user_flag = true;
                                    if (strcmp(database->password[i], args[2].c_str()) == 0) {
                                        passwd_flag = true;
                                    }
                                    index = i;
                                    break;
                                }
                            }
                            if (!login_flag && user_flag && passwd_flag) {
                                // prevent 2 clients login a same user
                                if (database->login_fd[index] != 0) {
                                    string temp = "Someone already logged in as " + string(database->username[index]) + "\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(client_fds[f], mr, strlen(mr));
                                } else {
                                    database->login_fd[index] = client_fds[f];
                                    string temp = "Welcome, " + string(database->username[index]) + "\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(client_fds[f], mr, strlen(mr));
                                }
                            } else if (!login_flag && user_flag) {
                                char mr[MAXLINE] = "Wrong password\n";
                                write(client_fds[f], mr, strlen(mr));
                            } else if (!user_flag) {
                                char mr[MAXLINE] = "Username does not exist\n";
                                write(client_fds[f], mr, strlen(mr));
                            } else {
                                string temp = "You already logged in as " + login_name + "\n";
                                char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                write(client_fds[f], mr, strlen(mr));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: login <username> <password>\n";
                            write(client_fds[f], mr, strlen(mr));
                        }
                    } 
                    // logout
                    else if (args[0] == "logout\n" || args[0] == "logout") {
                        // correct format
                        if (l == 1) {
                            // login now or not
                            bool can_logout = false; int index = 0;
                            for (int i = 0; i < database->count_u; i++) {
                                if (database->login_fd[i] == client_fds[f]) {
                                    can_logout = true;
                                    index = i;
                                    break;
                                }
                            }
                            if (can_logout) {
                                if (database->room_id[index] != -1) {
                                    string temp = "You are already in game room " + to_string(database->room_id[index]) + 
                                                    ", please leave game room\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(client_fds[f], mr, strlen(mr));
                                } else {
                                    database->login_fd[index] = 0;
                                    string temp = "Goodbye, " + string(database->username[index]) + "\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(client_fds[f], mr, strlen(mr));
                                }
                                
                            } else {
                                char mr[MAXLINE] = "You are not logged in\n";
                                write(client_fds[f], mr, strlen(mr));
                            }
                        } 
                        // incorrect format
                        else {
                            char mr[MAXLINE] = "Usage: logout\n";
                            write(client_fds[f], mr, strlen(mr));
                        }
                    } 
                    // start game 
                    else if (args[0] == "start") {
                        bool logged = false, in = false, manager = false, started = false, correct = false;
                        string name; int login_index, room_index;
                        for (int i = 0; i < database->count_u; i++) {
                            if (database->login_fd[i] == client_fds[f]) {
                                logged = true; login_index = i;
                                name = string(database->username[i]);
                                if (database->room_id[i] != -1) {
                                    in = true;
                                    for (int j = 0; j < room_data->count_r; j++) {
                                        if (database->room_id[i] == room_data->id[j]) {
                                            room_index = j;
                                            if (strcmp(database->username[i], room_data->owner[j]) == 0) {
                                                manager = true;
                                                if (room_data->status[j]) {
                                                    started = true;
                                                }
                                            } 
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        if (!logged) {
                            char mr[MAXLINE] = "You are not logged in\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (!in) {
                            char mr[MAXLINE] = "You did not join any game room\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (!manager) {
                            char mr[MAXLINE] = "You are not game room manager, you can't start game\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (started) {
                            char mr[MAXLINE] = "Game has started, you can't start again\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (l == 4 && !check_char(args[3].c_str())) {
                            char mr[MAXLINE] = "Please enter 4 digit number with leading zero\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else {
                            room_data->status[room_index] = true;
                            for (int i = 0; i < database->count_u; i++) {
                                if (database->room_id[i] == database->room_id[login_index]) {
                                    string temp = "Game start! Current player is " + name + "\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                    write(database->login_fd[i], mr, strlen(mr));
                                }
                            }
                        }
                    }
                    // start-game (old)
                    // else if (args[0] == "start-game\n" || args[0] == "start-game") {
                    //     // correct format
                    //     if (l == 1 || l == 2) {
                    //         // check login or not
                    //         bool login_flag = false;
                    //         for (int i = 0; i < database->count_u; i++) {
                    //             if (database->login_fd[i] == client_fds[f]) {
                    //                 login_flag = true;
                    //                 break;
                    //             }
                    //         }
                    //         if (login_flag) {
                    //             // use default answer "1234"
                    //             if (l == 1) {
                    //                 char mr[MAXLINE] = "Please typing a 4-digit number:\n";
                    //                 write(client_fds[f], mr, strlen(mr));
                    //                 int count = 5; 
                    //                 char ans[MAXLINE] = "1234\n";
                    //                 // loop for guess
                    //                 while (true) {
                    //                     read(client_fds[f], buffer, strlen(mr));
                    //                     // check guess is 4 bit number or not
                    //                     if (!check_char(buffer)) {
                    //                         char mr[MAXLINE] = "Your guess should be a 4-digit number.\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         continue;
                    //                     }
                    //                     // get the answer
                    //                     if (strncmp(buffer, ans, 5) == 0) {
                    //                         char mr[MAXLINE] = "You got the answer!\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         break;
                    //                     }
                    //                     // guess more than 5 time
                    //                     if (--count == 0) {
                    //                         char mr[MAXLINE] = "You lose the game!\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         char mr1[MAXLINE]; strcpy(mr1, get_hint(buffer, ans).c_str());
                    //                         write(client_fds[f], mr1, strlen(mr));
                    //                         char mr2[MAXLINE] = "You lose the game!\n";
                    //                         write(client_fds[f], mr2, strlen(mr));
                    //                         break;
                    //                     }
                    //                     // return xAxB hint
                    //                     char mr[MAXLINE]; strcpy(mr, get_hint(buffer, ans).c_str());
                    //                     write(client_fds[f], mr, strlen(mr));
                    //                 }
                    //             } else {
                    //                 // specify the answer
                    //                 // check the specified answer is 4 bit number or not
                    //                 if (!check_char(args[1].c_str())) {
                    //                     char mr[MAXLINE] = "Usage: start-game <4-digit number>\n";
                    //                     write(client_fds[f], mr, strlen(mr));
                    //                     continue;
                    //                 }
                    //                 char mr[MAXLINE] = "Please typing a 4-digit number:\n";
                    //                 write(client_fds[f], mr, strlen(mr));
                    //                 int count = 5; 
                    //                 char ans[MAXLINE]; strcpy(ans, args[1].c_str());
                    //                 // loop for guess
                    //                 while (true) {
                    //                     read(client_fds[f], buffer, strlen(mr));
                    //                     // check guess is 4 bit number or not
                    //                     if (!check_char(buffer)) {
                    //                         char mr[MAXLINE] = "Your guess should be a 4-digit number.\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         continue;
                    //                     }
                    //                     // get the answer
                    //                     if (strncmp(buffer, ans, 5) == 0) {
                    //                         char mr[MAXLINE] = "You got the answer!\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         break;
                    //                     }
                    //                     // guess more than 5 time
                    //                     if (--count == 0) {
                    //                         char mr[MAXLINE] = "You lose the game!\n";
                    //                         write(client_fds[f], mr, strlen(mr));
                    //                         char mr1[MAXLINE]; strcpy(mr1, get_hint(buffer, ans).c_str());
                    //                         write(client_fds[f], mr1, strlen(mr));
                    //                         char mr2[MAXLINE] = "You lose the game!\n";
                    //                         write(client_fds[f], mr2, strlen(mr));
                    //                         break;
                    //                     }
                    //                     // return xAxB hint
                    //                     char mr[MAXLINE]; strcpy(mr, get_hint(buffer, ans).c_str());
                    //                     write(client_fds[f], mr, strlen(mr));
                    //                 }
                    //             }
                    //         } 
                    //         // haven't login
                    //         else {
                    //             char mr[MAXLINE] = "Please login first.\n";
                    //             write(client_fds[f], mr, strlen(mr));
                    //         }
                    //     } 
                    //     // incorrect format
                    //     else {
                    //         char mr[MAXLINE] = "Usage: start-game <4-digit number>\n";
                    //         write(client_fds[f], mr, strlen(mr));
                    //     }
                    // }
                    // create room
                    else if (args[0] == "create") {
                        // check valid id
                        bool logged = false, already = false, valid_id = true;
                        int id = stoi(args[3]), in_id, login_index;
                        for (int i = 0; i < database->count_u; i++) {
                            if (database->login_fd[i] == client_fds[f]) {
                                logged = true;
                                login_index = i;
                                if (database->room_id[i] != -1) {
                                    in_id = database->room_id[i];
                                    already = true;
                                }
                                break;
                            }
                        }
                        for (int i = 0; i < room_data->count_r; i++) {
                            if (room_data->id[i] == id) {
                                valid_id = false;
                                break;
                            }
                        }
                        if (!logged) {
                            char mr[MAXLINE] = "You are not logged in\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (already) {
                            string temp = "You are already in game room " + to_string(in_id) + ", please leave game room\n";
                            char mr[MAXLINE]; strcpy(mr, temp.c_str());
                            write(client_fds[f], mr, strlen(mr));
                        } else if (!valid_id) {
                            char mr[MAXLINE] = "Game room ID is used, choose another one\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else {
                            string temp, t;
                            if (args[1] == "public") {
                                temp = "You create public game room " + args[3] + "\n";
                                t = "Public";
                            } else {
                                temp = "You create private game room " + args[3] + "\n";
                                t = "Private";
                                room_data->code[room_data->count_r] = stoi(args[4]);
                            }
                            database->room_id[login_index] = id;
                            room_data->id[room_data->count_r] = id;
                            room_data->status[room_data->count_r] = false;
                            strcpy(room_data->type[room_data->count_r], t.c_str());
                            strcpy(room_data->owner[room_data->count_r], database->username[login_index]);
                            room_data->count_r += 1;
                            char mr[MAXLINE]; strcpy(mr, temp.c_str());
                            write(client_fds[f], mr, strlen(mr));
                        }
                    }
                    // leave room
                    else if (args[0] == "leave") {
                        bool logged = false, in = true; int login_index;
                        for (int i = 0; i < database->count_u; i++) {
                            if (database->login_fd[i] == client_fds[f]) {
                                logged = true;
                                login_index = i;
                                if (database->room_id[i] == -1) {
                                    in = false;
                                }
                                break;
                            }
                        }
                        if (!logged) {
                            char mr[MAXLINE] = "You are not logged in\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (!in) {
                            char mr[MAXLINE] = "You did not join any game room\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else {
                            for (int i = 0; i < room_data->count_r; i++) {
                                if (room_data->id[i] == database->room_id[login_index]) {
                                    if (strcmp(room_data->owner[i], database->username[login_index]) == 0) { // success 1
                                        for (int j = 0; j < database->count_u; j++) {
                                            if (j != login_index && database->room_id[j] == room_data->id[i]) {
                                                database->room_id[j] = -1;
                                                // send msg to others
                                                string temp = "Game room manager leave game room " + to_string(room_data->id[i]) + 
                                                ", you are forced to leave too\n";
                                                char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                                                write(database->login_fd[j], mr, strlen(mr));
                                            }
                                        }
                                        string temp = "You leave game room " + to_string(room_data->id[i]) + "\n";
                                        char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                        write(client_fds[f], mr, strlen(mr));
                                        room_data->id[login_index] = -1;
                                        room_data->status[login_index] = false;
                                    } else if (room_data->status[i]) { // success 2
                                        for (int j = 0; j < database->count_u; j++) {
                                            if (j != login_index && database->room_id[j] == room_data->id[i]) {
                                                // send msg to others
                                                string temp = string(database->username[i]) + " leave game room " + to_string(room_data->id[i]) + 
                                                ", game ends\n";
                                                char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                                                write(database->login_fd[j], mr, strlen(mr));
                                                database->room_id[j] = -1;
                                            }
                                        }
                                        string temp = "You leave game room " + to_string(room_data->id[i]) + ", game ends\n";
                                        char mr[MAXLINE]; strcpy(mr, temp.c_str());
                                        write(client_fds[f], mr, strlen(mr));
                                        room_data->id[i] = -1;
                                        room_data->status[i] = false;
                                    } else { // success 3
                                        string temp = "You leave game room " + to_string(room_data->id[i]) + "\n";
                                        char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                                        write(client_fds[f], mr, strlen(mr));
                                        for (int j = 0; j < database->count_u; j++) {
                                            if (j != login_index && database->room_id[j] == room_data->id[i]) {
                                                // send msg to others
                                                string temp = string(database->username[login_index]) + " leave game room " + to_string(room_data->id[i]) + "\n";
                                                char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                                                write(database->login_fd[j], mr, strlen(mr));
                                            }
                                        }
                                    }
                                    database->room_id[login_index] = -1;
                                    break;
                                }
                            }
                        }
                    }
                    // join room
                    else if (args[0] == "join") {
                        bool logged = false, in = false, exist = false, priv = false, started = false;
                        string in_id; int login_index;
                        for (int i = 0; i < database->count_u; i++) {
                            if (database->login_fd[i] == client_fds[f]) {
                                login_index = i;
                                logged = true;
                                if (database->room_id[i] != -1) {
                                    in = true;
                                    in_id = to_string(database->room_id[i]);
                                } 
                                break;
                            }
                        }
                        for (int i = 0; i < room_data->count_r; i++) {
                            if (to_string(room_data->id[i]) == args[2]) {
                                exist = true;
                                if (strcmp(room_data->type[i], "Private") == 0) {
                                    priv = true;
                                } else {
                                    if (room_data->status[i]) {
                                        started = true;
                                    }
                                }
                                break;
                            }
                        }
                        if (!logged) {
                            char mr[MAXLINE] = "You are not logged in\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (in) {
                            string temp = "You are already in game room " + in_id + ", please leave game room\n";
                            char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                            write(client_fds[f], mr, strlen(mr));
                        } else if (!exist) {
                            string temp = "Game room " + args[2] + " is not exist\n";
                            char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                            write(client_fds[f], mr, strlen(mr));
                        } else if (priv) {
                            char mr[MAXLINE] = "Game room is private, please join game by invitation code\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else if (started) {
                            char mr[MAXLINE] = "Game has started, you can't join now\n";
                            write(client_fds[f], mr, strlen(mr));
                        } else {
                            string temp = "You join game room " + args[2] + "\n";
                            char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                            write(client_fds[f], mr, strlen(mr));
                            database->room_id[login_index] = stoi(args[2]);
                            for (int i = 0; i < database->count_u; i++) {
                                if (i != login_index && database->room_id[i] == database->room_id[login_index]) {
                                    string temp = "Welcome, " + string(database->username[login_index]) + " to game!\n";
                                    char mr[MAXLINE]; strcpy(mr, temp.c_str()); 
                                    write(database->login_fd[i], mr, strlen(mr));
                                }
                            }
                        }
                    }
                }
            }
        }
	}
    return 0;
}
