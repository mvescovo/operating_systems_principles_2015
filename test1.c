#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define NUM_PROCESSES 4 /* Total number of external processes */

struct group {
    long mtype; /* message type */
    int group; /* process group */
};

int main(int argc, char *argv[])
{
    int i;
    int msqid1, msqid2, length, result1, result2;
    struct group msg1, msg2;
#if 0
    int initTemp1; /* starting temperature */
    int initTemp2; /* starting temperature */
#endif
    int mailbox1;
    int mailbox2;

#if 0
    printf("E2BIG: %d\nEACCES: %d\nEFAULT %d\nEIDRM %d\nEINTR "
           "%d\nEINVAL %d\nENOMSG %d\nENOSYS %d\n", E2BIG, EACCES,
           EFAULT, EIDRM, EINTR, EINVAL, ENOMSG, ENOSYS);
#endif

    length = sizeof(int);

    /* Validate that a temperature was given via the command line */
    if(argc != 5) {
        printf("USAGE: Too few arguments --./test1.out Temp");
        exit(0);
    }
    
    /* set command line arguments */
#if 0
    initTemp1 = atoi(argv[1]);
    initTemp2 = atoi(argv[2]);
#endif
    mailbox1 = atoi(argv[3]);
    mailbox2 = atoi(argv[4]);

    /* set group values */
    msg1.mtype = 2; /* group type of message */
    msg1.group = 1; /* actual group */
    msg2.mtype = 2; /* group type of message */
    msg2.group = 2; /* actual group */

    /* create group1 mailbox if not already created */
    msqid1 = msgget(mailbox1, 0600 | IPC_CREAT);
    if (msqid1 == -1)
        fprintf(stderr, "msgget failed. test1.c.\n");
    printf("test1 connected to msqid1: %d\n", msqid1);

    /* create group2 mailbox if not already created */
    msqid2 = msgget(mailbox2, 0600 | IPC_CREAT);
    if (msqid2 == -1)
        fprintf(stderr, "msgget failed. test1.c.\n");
    else
        printf("test1 connected to msqid2: %d\n", msqid2);

    for (i = 0; i < NUM_PROCESSES; i++) {
        /* send a message to a group1 test2 process */
        result1 = msgsnd(msqid1, &msg1, length, 0);
        if (result1 == -1)
            fprintf(stderr, "msgsnd failed. test1.c\n");
        else
            printf("test1 sent message to group 1: %d\n", msg1.group);

        /* send a message to a group2 test2 process */
#if 1
        result2 = msgsnd(msqid2, &msg2, length, 0);
        if (result2 == -1)
            fprintf(stderr, "msgsnd failed. test1.c\n");
        else
            printf("test1 sent message to group 2: %d\n", msg2.group);
#endif
    }

    return EXIT_SUCCESS;
}
