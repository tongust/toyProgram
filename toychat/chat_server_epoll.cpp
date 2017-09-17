#define _BSD_SOURCE             /* To get definitions of NI_MAXHOST and
                                   NI_MAXSERV from <netdb.h> */
#include "csapp.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
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


static volatile sig_atomic_t gotSigio = 0;

class mchat {
public:
        static void * connect_proc(void *lfd_ptr) {
                /* Handle clients iteratively */
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

                    if (getnameinfo((struct sockaddr *) &claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
                        snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
                    else
                        snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");

                    printf("Connection from %s\n", addrStr);
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
            cout << "res " << res  << endl;
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

                /* epoll */
                const int max_events = 500;
                const int max_count_chater = max_events;
                const int max_buffer = 1024*1024;
                static int epfd,
                    ready;
                static struct epoll_event epev;
                static struct epoll_event evlist[max_events];
                epfd = epoll_create(max_events);
                if (epfd == -1) {
                        MDebugLog("epoll_create") << endl;
                }


                int lfd = *(int*) lfd_ptr;
                char reqLenStr[INT_LEN];            /* Length of requested sequence */

                static vector<int> client_fds;
                static pid_t ppid = getpid();

                struct sigaction action;
                sigemptyset(&action.sa_mask);
                action.sa_flags = SA_RESTART | SA_SIGINFO;
                static int count_chat = -1;

                void *res;

                /* handler */
                action.sa_sigaction = 
                        [](int sig, siginfo_t *siginfo_ptr, void* ucontext) {
                    /* the pid must be static to be visiable to handler of signal */
                    if (siginfo_ptr->si_pid == ppid) {
                        int client_fd = siginfo_ptr->si_value.sival_int;
                        void *res;

                        client_fds.push_back(client_fd);
                        cout << "Welcome new user! " << client_fd << endl;
                        
                        count_chat++;
                        if (count_chat >= max_count_chater) {
                                cout 
                                        << "there exists more than " 
                                        << max_count_chater 
                                        << " chaters" << endl;
                                return;
                        }

#if 1
                        epev.events = EPOLLIN | EPOLLET;/* edge trager  */
                        epev.data.fd = client_fd;
                        epoll_ctl
                                (
                                 epfd, EPOLL_CTL_ADD,
                                 client_fd,
                                 &epev
                                );

#endif

#if 0
                        pthread_t worker_echo;
                        int ret_pc = pthread_create(
                                        &worker_echo, 
                                        NULL, 
                                        echo, 
                                        &client_fd);
                        if (ret_pc != 0) {
                            printf("wrong\n");
                            exit(1);
                        }
#endif
                        /* TODO Task queue */
                    }
                };
                sigaction(SIGUSR1, &action, NULL);
                while (1) {
                        ready = epoll_wait(epfd, evlist, max_events, -1);
                        for (int i = 0; i < ready; i++) {
                                if (evlist[i].events & EPOLLIN) {
                                        char buffer_epoll[max_buffer];
                                        int lenread = 
                                                read(
                                                evlist[i].data.fd,
                                                buffer_epoll,
                                                max_buffer);
                                        if (lenread > 0) {
                                                buffer_epoll[lenread] = '\0';
                                                cout << buffer_epoll << endl;
                                        }
                                        else {
                                                cout 
                                                        << "exit " 
                                                        << evlist[i].data.fd 
                                                        << endl;
                                                /* close this file discriptor since 
                                                 * the sources of fd is not cheap.*/
                                                close(evlist[i].data.fd);

                                        }
                                } else if (evlist[i].events & (EPOLLHUP | EPOLLERR)) {
                                        cout << "EPOLLHUP EPOLLERR\n";
                                }
                        }
                }

                for (;;) pause();// pause - suspend the thread until a signal is received
            }
};


int main(int argc, char *argv[])
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
                // build the socket from DNS?
                lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if (lfd == -1)
                    continue;                   /* On error, try next address */

                if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
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
