#include "string.h"
#include "mailbox.h"

#define MAX_NUM_BOX 32

static mailbox_t mboxs[MAX_NUM_BOX] = {0};

void mbox_init()
{
  // unused
}

mailbox_t *mbox_open(char *name)
{
  int i;
  for (i=0; i<MAX_NUM_BOX; i++) {
    if (!strcmp(mboxs[i].name, name)) {
      atomic_add(&mboxs[i].refcount, 1);
      return &mboxs[i];
    }
  }
  for (i=0; i<MAX_NUM_BOX; i++) {
    if (mboxs[i].name[0] == '\0') {
      strcpy(mboxs[i].name, name);
      mboxs[i].refcount = 1;
      if (!mboxs[i].init) {
        mboxs[i].init = 1;
        mutex_lock_init(&mboxs[i].lock);
        condition_init(&mboxs[i].cond);
      }
      mboxs[i].sendptr = 0;
      mboxs[i].recvptr = 0;
      mboxs[i].full = 0;
      return &mboxs[i];
    }
  }
  return NULL;
}

void mbox_close(mailbox_t *mailbox)
{
  if (atomic_add(&mailbox->refcount, -1) <= 1) {
    mailbox->name[0] = '\0';
  }
}

void mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
  char *p = msg;
  int i, avail;
  mutex_lock_acquire(&mailbox->lock);
  while (msg_length > 0) {
    avail = mailbox->recvptr - mailbox->sendptr;
    if (avail < 0) avail += MAILBOX_BUFLEN;
    if (avail == 0 && !mailbox->full) avail = MAILBOX_BUFLEN;
    i = mailbox->sendptr;
    while (avail>0 && msg_length>0) {
      mailbox->buffer[i] = *p++;
      avail--;
      msg_length--;
      i = (i+1) % MAILBOX_BUFLEN;
    }
    mailbox->sendptr = i;
    mailbox->full = avail == 0;
    if (msg_length > 0) {
      condition_wait(&mailbox->lock, &mailbox->cond);
    }
  }
  condition_signal(&mailbox->cond);
  mutex_lock_release(&mailbox->lock);
}

void mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
  char *p = msg;
  int i, avail;
  mutex_lock_acquire(&mailbox->lock);
  while (msg_length > 0) {
    avail = mailbox->sendptr - mailbox->recvptr;
    if (avail < 0) avail += MAILBOX_BUFLEN;
    if (avail == 0 && mailbox->full) avail = MAILBOX_BUFLEN;
    i = mailbox->recvptr;
    while (avail>0 && msg_length>0) {
      *p++ = mailbox->buffer[i];
      avail--;
      msg_length--;
      i = (i+1) % MAILBOX_BUFLEN;
    }
    mailbox->recvptr = i;
    mailbox->full = avail == MAILBOX_BUFLEN;
    if (msg_length > 0) {
      condition_wait(&mailbox->lock, &mailbox->cond);
    }
  }
  condition_signal(&mailbox->cond);
  mutex_lock_release(&mailbox->lock);
}
