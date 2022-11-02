// Server program
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <filesystem>
#include <vector>
#include <fstream>  
#include <sstream>
#define MAXLINE 1024

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char *argv[]) {
	int udpfd;
	char buffer[MAXLINE];
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	// create UDP socket 
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    cout << "UDP server is running\n";

    while (true) {
        bzero(buffer, sizeof(buffer));
        len = sizeof(cliaddr);
        recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
        // cout << buffer;
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        } else if (strcmp(buffer, "get-file-list\n") == 0) {
            string path = fs::current_path(), s = "Files: ";
            for (const auto & entry : fs::directory_iterator(".")) {
                string temp  = entry.path();
                temp.erase(temp.begin()); temp.erase(temp.begin());
                s += temp;
                s += " ";
            }
            s += "\n";
            sendto(udpfd, s.c_str(), sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        } else {
            string arg;
	        vector<string> args;
            args.clear();
            stringstream ss(buffer);
            while (getline(ss, arg, ' ')) {
                args.push_back(arg);
            }
            int l = args.size();
            for (int i = 1; i < l; i++) {
                if (args[i][args[i].length()-1] == '\n')
                    args[i].pop_back();
                sendto(udpfd, args[i].c_str(), sizeof(args[i]), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                bzero(buffer, sizeof(buffer));
                stringstream ts; ts << args[i];
                ifstream fin(ts.str());
                stringstream ssf;
                ssf << fin.rdbuf();
                string content = ssf.str();
                sendto(udpfd, content.c_str(), sizeof(content), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            }
        }
    }

	close(udpfd);
}
