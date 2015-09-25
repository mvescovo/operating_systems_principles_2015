/*
 * external8.c
 *
 * Michael Vescovo s3459317
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct {
    long priority; /* message priority */
    int temp; /* temperature */
    int pid; /* process id */
    int stable; /* boolean for temperature stability */
} msgp, cmbox;

/* MAIN function */
int main(int argc, char *argv[])
{
    int unstable = 1;
    int mailbox;
    int result, length, status;
    int initTemp;
    int uid;
    int msqidC, msqid;

    /* Validate that a temperature and a Unique process ID was given via
       the command */
    if(argc != 4) {
        printf("USAGE: Too few arguments --./central.out Temp UID");
        exit(0);
    }

    initTemp = atoi(argv[1]);
    uid = atoi(argv[2]);
    mailbox = atoi(argv[3]);

    /* Create the Central Servers Mailbox */
    msqidC = msgget(mailbox, 0600 | IPC_CREAT);
    /* Create the mailbox for this process and store it's IDs */
    msqid = msgget((mailbox + uid), 0600 | IPC_CREAT);
    
    /* Initialize the message to be sent */
    cmbox.priority = 2;
    cmbox.pid = uid;
    cmbox.temp = initTemp;
    cmbox.stable = 1;

    /* The length is essentially the size of the structure minus
     * sizeof(mtype) */
    length = sizeof(msgp) - sizeof(long);

    /* While all the processes have different temps */
    while(unstable == 1){
        /* Send the current temp to the central server */
        result = msgsnd( msqidC, &cmbox, length, 0);
        if (result == -1)
            fprintf(stderr, "Failed to send messsage. External.c");
        
        /* Wait for a new message from the central server */
#if 0
        printf("uid: %d waiting on msqid: %d\n", uid, msqid);
#endif
        result = msgrcv( msqid, &msgp, length, 2, 0);
        if (result == -1)
            fprintf(stderr, "Failed to receive messsage. External.c");

        /* If the new message indicates all the processes have the same
         * temp break the loop and print out the final temperature */
        if(msgp.stable == 1) {
            unstable = 0;
            printf("Process %d Temp: %d\n", cmbox.pid, cmbox.temp);
        } else { /* otherwise calculate the new temp and store it */
            int newTemp = (10*cmbox.temp + msgp.temp) / 11;
            cmbox.temp = newTemp;
        }
    }

    /* Remove the mailbox */
    status = msgctl(msqid, IPC_RMID, 0);

    /* Validate nothing wrong when trying to remove mailbox */
    if(status != 0)
        printf("\nERROR closing mailbox\n");
    return 0;
}
