/*
 * external44.c
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
#include <errno.h>

struct {
    long priority; /* message priority */
    int temp; /* temperature */
    int pid; /* process id */
    int stable; /* boolean for temperature stability */
    int group; /* process group */
} msgp, cmbox;

/* MAIN function */
int main(int argc, char *argv[]) {
    /* Setup local variables */
    int unstable = 1;
    int result, length, status;
    int initTemp = atoi(argv[1]);
    int uid = atoi(argv[2]);
    int msqidC;
    int msqid;;
    
    /* Validate that a temperature and a Unique process ID was given via
       the command */
    if(argc != 4) {
        printf("USAGE: Too few arguments --./central.out Temp UID");
        exit(0);
    }

    /* Create the mailbox for this process and store it's IDs */
    switch(uid) {
        case 1:
        case 2:
        case 3:
        case 4:
            msqid = msgget((CENTRAL_MAILBOX1 + uid), 0600 | IPC_CREAT);
            printf("\nmsqid from external: %d", msqid);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            msqid = msgget((CENTRAL_MAILBOX2 + uid), 0600 | IPC_CREAT);
            printf("\nmsqid from external: %d", msqid);
            break;
    }

    /* Find out what group the process is in */
    result = msgrcv( msqid, &group, sizeof(int), 1, 0);
    if (result == -1) {
        int errsv = errno;
        fprintf(stderr, "\nFailed to receive messsage. external.c errno: "
                " %d", errsv);
    }
    if (group.group[0] == '1') {
        /* Create the Central Servers Mailbox */
        msqidC = msgget(CENTRAL_MAILBOX1, 0600 | IPC_CREAT);
    } else {
        /* Create the Central Servers Mailbox */
        msqidC = msgget(CENTRAL_MAILBOX2, 0600 | IPC_CREAT);
    }

    /* Initialize the message to be sent */
    cmbox.priority = 1;
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
            fprintf(stderr, "\nFailed to send messsage. external.c");
        
        /* Wait for a new message from the central server */
        result = msgrcv( msqid, &msgp, length, 1, 0);
        if (result == -1)
            fprintf(stderr, "\nFailed to receive messsage. external.c");

        /* If the new message indicates all the processes have the same
         * temp break the loop and print out the final temperature */
        if(msgp.stable == 0) {
            unstable = 0;
            printf("\nProcess %d Temp: %d\n", cmbox.pid, cmbox.temp);
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
