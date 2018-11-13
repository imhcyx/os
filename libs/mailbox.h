#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "lock.h"
#include "cond.h"

#define MAILBOX_BUFLEN 512

typedef struct mailbox
{
  char name[32];
  int refcount;
  int init;
  mutex_lock_t lock;
  condition_t cond;
  int sendptr;
  int recvptr;
  int full;
  char buffer[MAILBOX_BUFLEN];
} mailbox_t;


void mbox_init();
mailbox_t *mbox_open(char *);
void mbox_close(mailbox_t *);
void mbox_send(mailbox_t *, void *, int);
void mbox_recv(mailbox_t *, void *, int);

#endif
