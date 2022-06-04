// https://stackoverflow.com/questions/11208299/how-to-make-an-http-get-request-in-c-without-libcurl/35680609#35680609
/*
Paul Crocker
Muitas Modificações
*/
// Definir sta linha com 1 ou com 0 se não quiser ver as linhas com debug info.
#define DEBUG 0

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "csapp.h"

/* Function headers */
int connect_and_send_request(int argc, char **argv, int fileNum, int policy);
void *workerCONCURFunc(void *tInf);
void *workerFIFOFunc(void *tInf);

/*Globals and Defined variables*/

// Policies
#define CONCUR 1
#define FIFO 2

pthread_barrier_t mybarrier;
pthread_cond_t condVerb = PTHREAD_COND_INITIALIZER;
pthread_mutex_t fifoMutex = PTHREAD_MUTEX_INITIALIZER;
int fifoTurn;
int numberOfThreads;

/* Data structures*/
typedef struct thread_information
{
    char **argv;
    int argc;
    int id;
} thread_info;

// Get host information (used to establishConnection)
struct addrinfo *getHostInfo(char *host, char *port) {
    int r;
    struct addrinfo hints, *getaddrinfo_res;
    // Setup hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
        fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
        return NULL;
    }

    return getaddrinfo_res;
}

// Establish connection with host
int establishConnection(struct addrinfo *info) {
    if (info == NULL) return -1;

    int clientfd;
    for (; info != NULL; info = info->ai_next) {
        if ((clientfd = socket(info->ai_family,
                               info->ai_socktype,
                               info->ai_protocol)) < 0) {
            perror("[establishConnection:35:socket]");
            continue;
        }

        if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
            close(clientfd);
            perror("[establishConnection:42:connect]");
            continue;
        }

        freeaddrinfo(info);
        return clientfd;
    }

    freeaddrinfo(info);
    return -1;
}

// Send GET request
void GET(int clientfd, char *path) {
    char req[1000] = {0};
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", path);
    send(clientfd, req, strlen(req), 0);
}

int main(int argc, char **argv)
{
    char buffer[BUFSIZ];
    enum CONSTEXPR
    {
        MAX_REQUEST_LEN = 1024
    };
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";
    struct protoent *protoent;

    char *hostname = "localhost";
    unsigned short server_port = 8080; // default port
    char *file;
    char *fileindex = "/";
    char *filedynamic = "/cgi-bin/adder?150&100";
    char *filestatic = "/godzilla.jpg";

    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;

    char *schedAlg;
    int numThreads;



     /* Checks for amount of arguments */
    if (argc == 4) {
        connect_and_send_request(argc, argv, 0, -1);
    }

    /* Check command line args */
    if (argc != 6)
    {
        fprintf(stderr, "usage: %s <host> <port> <threads> <schedalg> <filename1> <filename2 - Optional>\n", argv[0]);
        exit(1);
    }
    if (argc > 1)
        hostname = argv[1];
    if (argc > 2)
        server_port = strtoul(argv[2], NULL, 10);
    if (argc > 3)
        numThreads = atoi(argv[3]);
    if (argc > 4)
        /* getting scheduling policy*/
        schedAlg = argv[4];
    if (argc > 5)
        /* getting scheduling policy*/
        file = argv[5];
    else
        file = fileindex; // ou escolher outra filedynamic filestatic

    int policy = -1;

    if (strcmp(schedAlg, "CONCUR") == 0)
        policy = CONCUR; // FIFO is any policy, after all
    else if (strcmp(schedAlg, "FIFO") == 0)
        policy = FIFO;
    else
    {
        printf("Error: \"%s\" is Invalid scheduling policy, try CONCUR or FIFO\n", schedAlg);
        return 1;
    }

    /* --Thread creation-- */
    pthread_t threadIds[numThreads]; // holds the pthread_t for each thread
    thread_info tInfo[numThreads];   // array of thread_info structs for each thread

    int ret;

    ret = pthread_barrier_init(&mybarrier, NULL, numThreads);

    if (ret != 0)
    {
        printf("Could not create thread barrier\n");
        return 1;
    }

    ret = pthread_cond_init(&condVerb, NULL);
    
    if(ret != 0){
        printf("Could not create condition variable\n");
      return 1;
    }

    numberOfThreads = numThreads;

    int i;

    if (policy == CONCUR) {
        for (i = 0; i < numThreads; i++) {
            //sets struct params
            tInfo[i].id = i;
            tInfo[i].argv = argv;
            tInfo[i].argc = argc;

            //creates a thread
            ret = pthread_create(&threadIds[i], NULL, workerCONCURFunc, (void *) &tInfo[i]);

            if(ret != 0){
                printf("Could not create thread\n");
                return 1;
            }
        }
    }

    if (policy == FIFO) {
        fifoTurn = 0;

        for (i = 0; i < numThreads; i++) {
            //sets struct params
            tInfo[i].id = i;
            tInfo[i].argv = argv;
            tInfo[i].argc = argc;

            //creates a thread
            ret = pthread_create(&threadIds[i], NULL, workerFIFOFunc, (void *) &tInfo[i]);

            if (ret != 0) {
                printf("Could not create thread\n");
                return 1;
            }
        }
    }

    pthread_barrier_wait(&mybarrier);

    pthread_exit(NULL);

    return 0;






    // construção do pedido de http
    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, file, hostname);
    if (request_len >= MAX_REQUEST_LEN)
    {
        fprintf(stderr, "request length large: %d\n", request_len);
        exit(EXIT_FAILURE);
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL)
    {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }

    // Open the socket
    socket_file_descriptor = Socket(AF_INET, SOCK_STREAM, protoent->p_proto);

    /* Build the address. */
    // 1 get the hostname address
    hostent = Gethostbyname(hostname);

    in_addr = inet_addr(inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1)
    {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        exit(EXIT_FAILURE);
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Ligar ao servidor */
    Connect(socket_file_descriptor, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in));

    /* Send HTTP request. */
    Rio_writen(socket_file_descriptor, request, request_len);

    /* Read the response. */
    if (DEBUG)
        fprintf(stderr, "debug: before first read\n");

    rio_t rio;
    char buf[MAXLINE];

    /* Leituras das linhas da resposta . Os cabecalhos - Headers */
    const int numeroDeHeaders = 5;
    Rio_readinitb(&rio, socket_file_descriptor);
    for (int k = 0; k < numeroDeHeaders; k++)
    {
        Rio_readlineb(&rio, buf, MAXLINE);

        // Envio das estatisticas para o canal de standard error
        if (strstr(buf, "Stat") != NULL)
            fprintf(stderr, "STATISTIC : %s", buf);
    }

    // double time_spent = 0.0;
    // clock_t begin = clock();

    // Ler o resto da resposta - o corpo de resposta.
    // Vamos ler em blocos caso que seja uma resposta grande.
    while ((nbytes = Rio_readn(socket_file_descriptor, buffer, BUFSIZ)) > 0)
    {
        if (DEBUG)
            fprintf(stderr, "debug: after a block read\n");
        // commentar a lina seguinte se não quiser ver o output
        Rio_writen(STDOUT_FILENO, buffer, nbytes);
    }

    // clock_t end = clock();
    // time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Elapsed: %f\n", time_spent);

    if (DEBUG)
        fprintf(stderr, "debug: after last read\n");

    Close(socket_file_descriptor);

    exit(EXIT_SUCCESS);
}


void *workerCONCURFunc (void *tInf) {
    thread_info *tInfo = tInf;
    int j = 0;

    while (1) {
        if (tInfo->argc == 6) {
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, tInfo->id);
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 1, tInfo->id);
        }
        else
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, -1);

        pthread_barrier_wait(&mybarrier);
        j++;
    }
    return 0;
}

void *workerFIFOFunc (void *tInf) {
    thread_info *tInfo = tInf;
    int j = 0;

    while (1) {
        if (tInfo->id == 0)
           fifoTurn = 0;

        pthread_mutex_lock( &fifoMutex);
        while (tInfo->id != fifoTurn) {
            pthread_cond_signal(&condVerb);
            pthread_cond_wait( &condVerb, &fifoMutex);
        }


        /* WORK */
        if (tInfo->argc == 6) {

            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, tInfo->id);
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 1, tInfo->id);
        }
        else
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, tInfo->id);


        pthread_barrier_wait(&mybarrier);
        j++;
    }
    return 0;
}

int connect_and_send_request (int argc, char **argv, int fileNum, int policy) {
    int clientfd;
    char buf[BUFSIZ];

    // Establish connection with <hostname>:<port>
    clientfd = establishConnection(getHostInfo(argv[1], argv[2]));
    if (clientfd == -1) {
        fprintf(stderr,
                "[main:73] Failed to connect to: %s:%s%s \n",
                argv[1], argv[2], argv[3]);
        return 3;
    }

    // Send GET request > stdout
    if (policy >= 0) {
//        printf("%d done fifo: %d\n", policy, fifoTurn);

        if (fifoTurn == numberOfThreads-1)
            fifoTurn = 0;
        else
            fifoTurn++;
//        printf("%d\n", fifoTurn);

        if (argc == 6)
            GET(clientfd, argv[5]);
        else {
            GET(clientfd, argv[5 + fileNum]);
        }

        pthread_cond_signal(&condVerb);
        pthread_mutex_unlock( &fifoMutex );
    }
    else {
        if (argc == 6)
            GET(clientfd, argv[5]);
        else if (argc == 7) {
            GET(clientfd, argv[5 + fileNum]);
        } else
            GET(clientfd, argv[3]);
    }

    while (recv(clientfd, buf, BUFSIZ, 0) > 0) {
        fputs(buf, stdout);
        memset(buf, 0, BUFSIZ);
    }

    close(clientfd);
    return 0;
}