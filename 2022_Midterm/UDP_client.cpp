// UDP Client
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
#include <vector>
#include <strings.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <fstream>  
#define MAX 1024

using namespace std;

int main(int argc, char *argv[]) {
	int udpsockfd;
	char buffer[MAX];
    socklen_t len;
	struct sockaddr_in servaddr;

	// create UDP socket file descriptor
	if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cout << "UDP socket creation failed\n";
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    string arg;
	vector<string> args;
    while (true) {
        bzero(buffer, sizeof(buffer));
        cout << "% ";
        fgets(buffer, MAX, stdin);
        args.clear();
		stringstream ss(buffer);
		while (getline(ss, arg, ' ')) {
			args.push_back(arg);
		}
		int l = args.size();
        if (strcmp(buffer, "exit\n") == 0) {
            sendto(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            break;
        } else if (strcmp(buffer, "get-file-list\n") == 0) {
            sendto(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            bzero(buffer, sizeof(buffer));
            len = sizeof(servaddr);
            recvfrom(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, &len);
            cout << buffer;
        } else if (strncmp(buffer, "get-file", 8) == 0) {
            sendto(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            bzero(buffer, sizeof(buffer));
            len = sizeof(servaddr);
            int cnt = l - 1;
            while (cnt--) {
                recvfrom(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, &len);
                stringstream fs; fs << buffer;
                ofstream fout(fs.str());
                bzero(buffer, sizeof(buffer));
                recvfrom(udpsockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&servaddr, &len);
                fout << buffer;
            }
        } else {
            cout << "Command not found\n";
        }
    }

    close(udpsockfd);

    return 0;
}
