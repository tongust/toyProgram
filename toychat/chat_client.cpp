/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"
#include "chat_util.h"
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <iostream>

#define PORT_NUM "50000"

using std::string;
using std::cin;
using std::cout;
using std::endl;

string build_message() {
    string msgs, msg;
    while (cin >> msg) {
        if ((!msg.empty()) && msg[0] == '^')
            break;
        msgs += msg + " ";
    }
    return msgs;
}

int main(int argc, char **argv) 
{
    char *reqLenStr;                    /* Requested length of sequence */
    int cfd;
    ssize_t numRead;
    struct addrinfo hints;
    struct addrinfo *result, *rp;


    if (argc < 2 || strcmp(argv[0], "--help") == 0)
    {
        printf("%s server-host \n", argv[0]);
        exit(0);
    }

    /* Call getaddrinfo() to obtain a list of addresses that
       we can try connecting to */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;                /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if (getaddrinfo(argv[1], PORT_NUM, &hints, &result) != 0)
        printf("getaddrinfo\n");

    /* Walk through returned list until we find an address structure
       that can be used to successfully connect a socket */

    while (true) {
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (cfd == -1)
            {
                continue;                           /* On error, try next address */
            }
            if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)
            {
                break;                              /* Success */
            }
            /* Connect failed: close this socket and try next address */
            close(cfd);
        }

        if (rp == NULL)
            printf("Could not connect socket to any address\n");

        cout << "cfd " << cfd << endl;
        /* Send requested sequence length, with terminating newline */

        
        char sm[10] = "a234";
        sm[4] = 0;
        //write(cfd, sm, 4);
        cout << sm << endl;
        //close(cfd);
        //write(cfd, sm, 4);
        cout << sm << endl;
        //close(cfd);
        //return 0;
        while (true) {
            string content =  build_message();
            size_t len_content = strlen(content.c_str());
            if (write(cfd, content.c_str(),len_content)!=len_content){
                    printf("error send\n");
                    close(cfd);
                    break;
            }
            cout << "\t\t\t\t\t\t\t\t\t\t " << content << std::endl;
        }
    }

    freeaddrinfo(result);

    exit(0);
}
