#define _BSD_SOURCE             /* To get definitions of NI_MAXHOST and
                                   NI_MAXSERV from <netdb.h> */
#include "csapp.h"
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "chat_util.h"
#include "stdlib.h"
#include <iostream>

/* define */
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)
#define BACKLOG 50
#define PORT_NUM "50000"        /* Port number for server */
#ifndef MDebugLog
#define MDebugLog(msg)  std::cout << __FILE__ << ":" << __LINE__ << ": " << msg
#endif

#define INT_LEN 30      

using std::cout;
using std::endl;
using std::string;
using std::vector;

class mchat {
public:
    static void * connect_proc(void *lfd_ptr) {
        //                  /* Handle clients iteratively */
        pid_t cur_pid = getpid();
        int lfd = *(int*)lfd_ptr;
        union sigval sigval_clientfd;
        char host[NI_MAXHOST];
        char addrStr[ADDRSTRLEN];
        char service[NI_MAXSERV];

        socklen_t  addrlen = sizeof(struct sockaddr_storage);
        struct sockaddr_storage claddr;
        while (1) {
            auto cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
            if (cfd == -1) {
                printf("accept\n");
                break;
            }

            if (getnameinfo((struct sockaddr *) &claddr, addrlen,
                        host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
                snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
            else
                snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");

            printf("Connection from %s\n", addrStr);
            cout << "p cfd " << cfd << endl;
            sigval_clientfd.sival_int = cfd;
            sigqueue(cur_pid, SIGUSR1, sigval_clientfd);
        }
        return NULL;
    }
    
    static void* echo(void *cfd_ptr) {
        int cfd = *(int*) cfd_ptr;
        char reqLenStr[INT_LEN];            /* Length of requested sequence */
        while (1) {
            auto res = read(cfd, reqLenStr, INT_LEN);
            if (res <= 0) {
                cout << "exit " << endl;
                break;
            } else {
                cout << reqLenStr << endl;
            }

        }
        return NULL;
    }

    static void* echo_proc(void *lfd_ptr) {

        /* pipe */
        static int pipe_rw[2];
        if (pipe(pipe_rw) == -1) {
            printf("creat pipe falut\n");
            exit(0);
        }

        int lfd = *(int*) lfd_ptr;
        char reqLenStr[INT_LEN];            /* Length of requested sequence */

        static vector<int> client_fds;
        static pid_t ppid = getpid();
        static int max_fd = 0;
        static int max_chat_fd = 0;
        static bool isNew = false;

        static fd_set ready_set, read_set;
        FD_ZERO(&read_set);
        FD_SET(pipe_rw[0], &read_set);
        //FD_SET(lfd, &read_set);
        max_fd = pipe_rw[0];

        struct sigaction action;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART | SA_SIGINFO;
        /* handler */
        action.sa_sigaction = [](int sig, siginfo_t *siginfo_ptr, void* ucontext) {
            /* the pid must be static to be visiable to handler of signal */
            if (siginfo_ptr->si_pid == ppid) {
                int client_fd = siginfo_ptr->si_value.sival_int;
                if (client_fd > max_fd)
                    max_fd = client_fd;
                if (client_fd > max_chat_fd)
                    max_chat_fd = client_fd;

                client_fds.push_back(client_fd);
                cout << "Welcome new user! " << client_fd << endl;
                void *res;
                pthread_t worker_echo;
                int ret_pc = pthread_create(&worker_echo, NULL, echo, &client_fd);
                if (ret_pc != 0) {
                    printf("wrong\n");
                    exit(1);
                }
                /*
                        int cfd = client_fd;
                        char msgs[100];
                        auto nn = read(cfd, msgs, 99);

                        cout << cfd << endl;

                        if (nn >= 0) {
                            cout << msgs << endl;
                        }
                        close(cfd);
                */
            }
        };
        sigaction(SIGUSR1, &action, NULL);
        for (;;) pause();// pause - suspend the thread until a signal is received

        while (0) {
            if (isNew) {
            if (pipe(pipe_rw) == -1) {
                printf("creat pipe falut\n");
                exit(0);
            }
            FD_SET(pipe_rw[0], &read_set);
            if (max_fd < pipe_rw[0]){
                max_fd = pipe_rw[0];
            }
            }
            ready_set = read_set;
            select(max_fd+1, &ready_set, NULL, NULL, NULL);
            MDebugLog("hello")  << endl;
            FD_CLR(pipe_rw[0], &read_set);
            if (max_fd == pipe_rw[0]) {
                max_fd = max_chat_fd;
            }
            isNew = false;
            /*
            for (auto f : client_fds) {
                cout << f << endl;
                        char msgs[100];
                        auto nn = read(f, msgs, 99);
                        if (nn >= 0) {
                            cout << msgs << endl;
                        }
            }
            */

            for (auto f : client_fds) {
                if (FD_ISSET(f, &ready_set)) {
                    if (readLine(f, reqLenStr, INT_LEN) <= 0) {
                        cout << "," << endl;
                        close(f);
                        FD_CLR(f, &ready_set);
                        continue;          
                    }
                    cout << reqLenStr << endl;
                }
            }
        }

    }
};


int
main(int argc, char *argv[])
{
    uint32_t seqNum;
    char reqLenStr[INT_LEN];            /* Length of requested sequence */
    char seqNumStr[INT_LEN];            /* Start of granted sequence */
    struct sockaddr_storage claddr;
    int lfd, cfd, optval, reqLen;
    socklen_t addrlen;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        printf("%s [init-seq-num]\n", argv[0]);

    seqNum = (argc > 1) ? atoi(argv[1]) : 0;

    /* Ignore the SIGPIPE signal, so that we find out about broken connection
       errors via a failure from write(). */

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)    printf("signal");

    /* Call getaddrinfo() to obtain a list of addresses that
       we can try binding to */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;        /* Allows IPv4 or IPv6 */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
                        /* Wildcard IP address; service name is numeric */

    if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
        printf("getaddrinfo");

    /* Walk through returned list until we find an address structure
       that can be used to successfully create and bind a socket */

    optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
            continue;                   /* On error, try next address */

        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
                == -1)
             printf("setsockopt");

        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                      /* Success */

        /* bind() failed: close this socket and try next address */

        close(lfd);
    }

    if (rp == NULL)
        printf("Could not bind socket to any address");

    if (listen(lfd, BACKLOG) == -1)
        printf("listen");

    freeaddrinfo(result);

    /* multipthread */
    mchat chat_util;
    void *res;
    pthread_t worker_echo, worker_connect;
    int ret_pc = pthread_create(&worker_connect, NULL, mchat::connect_proc, &lfd);
    if (ret_pc != 0) {
        printf("wrong\n");
        exit(1);
    }
    ret_pc = pthread_create(&worker_echo, NULL, mchat::echo_proc, &lfd);
    ret_pc = pthread_join(worker_connect, &res);
    ret_pc = pthread_join(worker_echo, &res);
    return 0;

}
