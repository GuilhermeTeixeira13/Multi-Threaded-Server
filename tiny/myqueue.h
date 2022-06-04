#ifndef MYQUEUE_H_
#define MYQUEUE_H_

typedef struct qnode
{
  int *client_socket;
  struct qnode *next;
  int prty;

} Qnode, *QnodePtr;

typedef struct Queue
{
  QnodePtr top;
  QnodePtr tail;

} Queuetype, *Queue;
#endif