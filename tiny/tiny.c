/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.

 Fom csapp
 Modified by Paul

 */
#include "myqueue.h"
#include "csapp.h"
#include <stdbool.h>

void enqueue(int *client_socket);
int *dequeue();
void *doit(void *p_fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread_function(void *args);

int numeroRequestStat = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
int queueCurrentSize = 0;
int buffersSize = 0;
long Stat_thread_id = 0;

int main(int argc, char **argv)
{
  int listenfd, connfd, port, ThreadPoolSIZE;
  unsigned int clientlen; // change to unsigned as sizeof returns unsigned
  struct sockaddr_in clientaddr;

  /* Check command line args */
  if (argc != 4)
  {
    fprintf(stderr, "usage: %s <port> <threads> <buffers>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  ThreadPoolSIZE = atoi(argv[2]);
  buffersSize = atoi(argv[3]);

  fprintf(stderr, "Server : %s Running on  <%d> <%d> <%d>\n", argv[0], port, ThreadPoolSIZE, buffersSize);

  listenfd = Open_listenfd(port);

  // Uma série de threads é criada para dar conta de futuros pedidos
  pthread_t thread_pool[ThreadPoolSIZE];
  for (long i = 0; i < ThreadPoolSIZE; i++)
    pthread_create(&thread_pool[i], NULL, thread_function, (void *)i);

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept

    // Deixamos a conection aqui para que um dos threads disponíveis a consiga encontrar
    int *pclient = malloc(sizeof(int));
    *pclient = connfd;

    // Proteger a fila (Race condition), com um mutex lock
    pthread_mutex_lock(&mutex);
    if (queueCurrentSize <= buffersSize)
    {
      enqueue(pclient);
      queueCurrentSize++;
    }
    pthread_cond_signal(&condition_var); // Diz ao thread que está à espera, para começar a trabalhar
    pthread_mutex_unlock(&mutex);
  }
}

void *thread_function(void *args)
{
  Stat_thread_id = (long)args;
  while (true) // Threads nunca morrem
  {
    int *pclient;

    pthread_mutex_lock(&mutex);
    if ((pclient = dequeue()) == NULL)
    {
      // Se não há mais ninguém na fila, então espera (ao invés de estar constantemente a criar novos threads)
      pthread_cond_wait(&condition_var, &mutex);
      pclient = dequeue();
      queueCurrentSize--;
    }
    pthread_mutex_unlock(&mutex);

    if (pclient != NULL)
    {
      // Thread tem trabalho a fazer
      doit(pclient);
    }
  }
}

/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void *doit(void *p_fd)
{
  int fd = *((int *)p_fd);
  // printf("fd é --> %d", fd);
  free(p_fd);
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);             // line:netp:doit:readrequest
  sscanf(buf, "%s %s %s", method, uri, version); // line:netp:doit:parserequest
  if (strcasecmp(method, "GET"))
  { // line:netp:doit:beginrequesterr
    clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
    return NULL;
  }                       // line:netp:doit:endrequesterr
  read_requesthdrs(&rio); // line:netp:doit:readrequesthdrs

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs); // line:netp:doit:staticcheck
  if (stat(filename, &sbuf) < 0)
  { // line:netp:doit:beginnotfound
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return NULL;
  } // line:netp:doit:endnotfound

  if (is_static)
  { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    { // line:netp:doit:readable
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return NULL;
    }
    serve_static(fd, filename, sbuf.st_size); // line:netp:doit:servestatic
  }
  else
  { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    { // line:netp:doit:executable
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return NULL;
    }
    serve_dynamic(fd, filename, cgiargs); // line:netp:doit:servedynamic
  }

  Close(fd);

  return NULL;
}

/* $end doit */

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  { // line:netp:readhdrs:checkterm
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content */             // line:netp:parseuri:isstatic
    strcpy(cgiargs, "");             // line:netp:parseuri:clearcgi
    strcpy(filename, ".");           // line:netp:parseuri:beginconvert1
    strcat(filename, uri);           // line:netp:parseuri:endconvert1
    if (uri[strlen(uri) - 1] == '/') // line:netp:parseuri:slashcheck
      strcat(filename, "home.html"); // line:netp:parseuri:appenddefault
    return 1;
  }
  else
  { /* Dynamic content */  // line:netp:parseuri:isdynamic
    ptr = index(uri, '?'); // line:netp:parseuri:beginextract
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, ""); // line:netp:parseuri:endextract
    strcpy(filename, "."); // line:netp:parseuri:beginconvert2
    strcat(filename, uri); // line:netp:parseuri:endconvert2
    return 0;
  }
}

/* $end parse_uri */

/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);    // line:netp:servestatic:getfiletype
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:servestatic:beginserve
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sRequestStat: %d\r\n", buf, numeroRequestStat++);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // sprintf(buf, "%sTarefa realizada pelo thread -> %ld\r\n\r\n", buf, Stat_thread_id);
  Rio_writen(fd, buf, strlen(buf)); // line:netp:servestatic:endserve

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);                        // line:netp:servestatic:open
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // line:netp:servestatic:mmap
  Close(srcfd);                                               // line:netp:servestatic:close
  Rio_writen(fd, srcp, filesize);                             // line:netp:servestatic:write
  Munmap(srcp, filesize);                                     // line:netp:servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 * Deverá adicionar mais tipos
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  // Não ALTERAR!
  int pipefd[2];

  /*Paul Crocker
    Changed so that client content is piped back to parent
  */
  Pipe(pipefd);

  if (Fork() == 0)
  { /* child */ // line:netp:servedynamic:fork
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // line:netp:servedynamic:setenv
    // Dup2 (fd, STDOUT_FILENO);	/* Redirect stdout to client *///line:netp:servedynamic:dup2
    Dup2(pipefd[1], STDOUT_FILENO);

    Execve(filename, emptylist, environ); /* Run CGI program */ // line:netp:servedynamic:execve
  }
  close(pipefd[1]);
  char content[1024]; // max size that cgi program will return

  int contentLength = read(pipefd[0], content, 1024);
  Wait(NULL); /* Parent waits for and reaps child */ // line:netp:servedynamic:wait
  // Não ALTERAR!

  /* Generate the HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:servestatic:beginserve
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sRequestStat: %d\r\n", buf, numeroRequestStat++);
  sprintf(buf, "%sContent-length: %d\r\n", buf, contentLength);
  sprintf(buf, "%sContent-type: text/html\r\n\r\n", buf);
  // sprintf(buf, "%sTarefa realizada pelo thread -> %ld\r\n\r\n", buf, Stat_thread_id);
  Rio_writen(fd, buf, strlen(buf)); // line:netp:servestatic:endserve

  Rio_writen(fd, content, contentLength);
}

/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  /* Fazer primeiro visto que precisamos de saber o tamanho do body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sRequestStat: %d\r\n", buf, numeroRequestStat++);
  // sprintf(buf, "%sTarefa realizada pelo thread -> %ld\r\n\r\n", buf, Stat_thread_id);
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));

  Rio_writen(fd, body, strlen(body));
}

/* $end clienterror */
