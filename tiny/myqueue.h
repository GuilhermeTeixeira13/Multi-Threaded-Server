#ifndef MYQUEUE_H_
#define MYQUEUE_H_

struct node {
  struct node* next;
  int *client_socket;
  int priority;
};
typedef struct node node_t;

#endif