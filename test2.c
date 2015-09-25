#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

struct group {
    long mtype; /* message type */
    int group; /* process group */
};

int main(int argc, char *argv[])
{
    int msqid, length, result;
    struct group msg;
#if 0
    int initTemp;
    int uid;
#endif
    int mailbox;

    length = sizeof(int) * 1000;

    /* Validate that a temperature and a Unique process ID and a mailbox
       key was given via the command */
    if(argc != 4) {
        printf("USAGE: Too few arguments --./test2.out Temp UID");
        exit(0);
    }

    /* set command line arguments */
#if 0
    initTemp = atoi(argv[1]);
    uid = atoi(argv[2]);
#endif
    mailbox = atoi(argv[3]);

    /* set group values */
    msg.mtype = 0; /* group type of message */
    msg.group = 0; /* actual group */

    /* create mailbox if not already created */
    msqid = msgget(mailbox, 0600 | IPC_CREAT);
    if (msqid == -1)
        fprintf(stderr, "msgget failed. test2.c.\n");
    else
        printf("test2 connected to msqid: %d\n", msqid);
    
    /* receive a message from test1 process */
    result = msgrcv(msqid, &msg, length, 2, 0);
    if (result == -1) {
        int errsv = errno;
        fprintf(stderr, "msgrcv failed. test2.c on: %d, errno: %d\n",
                msqid, errsv);

    } else {
        printf("test2 received message from group: %d, msqid: %d\n", 
                msg.group, msqid);
    }

    return EXIT_SUCCESS;
}
