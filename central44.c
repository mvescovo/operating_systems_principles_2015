/*
 * central44.c
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
#include <sys/time.h>

#define NUM_PROCESSES 8 /* Total number of external processes */
#define GROUP1_SIZE 4
#define GROUP2_SIZE 4

struct {
    long priority; /* message priority */
    int temp; /* temperature */
    int pid; /* process id */
    int stable; /* boolean for temperature stability */
} msgp, cmbox;

/* MAIN function */
int main(int argc, char *argv[])
{
    struct timeval t1, t2;
    double elapsedTime;
    int mailbox1, mailbox2;
    int i,result,length,status; /* counter for loops */
    int uid = 0; /* central process ID */
    int initTemp1, initTemp2; /* starting temperatures */
    int cTemp1, cTemp2; /* central temperates */
    int msqid[NUM_PROCESSES]; /* mailbox IDs for all processes */
    int group1_unstable = 1; /* boolean to denote temp stability */
    int group2_unstable = 1; /* boolean to denote temp stability */
    int unstable = 1; /* boolean to denote temp stability */
    int tempAry[NUM_PROCESSES]; /* array of process temperatures */
    int msqidC1, msqidC2; /* central server mailboxes */

    /* Validate that a temperature was given via the command line */
    if(argc != 5) {
        printf("USAGE: Too few arguments --./central.out Temp");
        exit(0);
    }

    initTemp1 = atoi(argv[1]);
    initTemp2 = atoi(argv[2]);
    mailbox1 = atoi(argv[3]);
    mailbox2 = atoi(argv[4]);
    cTemp1 = initTemp1;
    cTemp2 = initTemp2;

    /* Create the Central Server Mailboxes */
    msqidC1 = msgget(mailbox1, 0600 | IPC_CREAT);
    msqidC2 = msgget(mailbox2, 0600 | IPC_CREAT);

    printf("\nStarting Server...\n");

    /* start timer */
    gettimeofday(&t1, NULL);

    /* Create the mailboxes for the other processes and store their IDs */
    for(i = 1; i <= NUM_PROCESSES; i++) {
        if (i <= GROUP1_SIZE)
            msqid[(i-1)] = msgget((mailbox1 + i), 0600 | IPC_CREAT);
        else
            msqid[(i-1)] = msgget((mailbox2 + i), 0600 | IPC_CREAT);
    }

    /* Initialize the message to be sent */
    msgp.priority = 2;
    msgp.pid = uid;

    /* The length is essentially the size of the structure minus
     * sizeof(mtype) */
    length = sizeof(msgp) - sizeof(long);

    /* While the processes have different temperatures */
    while(unstable == 1){
        int sumTemp1 = 0; /* sum up the temps as we loop */
        int sumTemp2 = 0; /* sum up the temps as we loop */
        int stable1 = 1; /* stability trap */
        int stable2 = 1; /* stability trap */
        
        /* Get new messages from the processes */
        for(i = 0; i < NUM_PROCESSES; i++) {
            if ((i < GROUP1_SIZE) && group1_unstable) {
                result = msgrcv( msqidC1, &cmbox, length, 2, 0);
                if (result == -1)
                    fprintf(stderr, "Failed to receive messsage. "
                            "External.c");
                /* If any of the new temps are different from the old 
                 *  temps then we are still unstable. Set the new temp
                 *  to the corresponding process ID in the array */
                if(tempAry[(cmbox.pid - 1)] != cmbox.temp) {
                    stable1 = 0;
                    tempAry[(cmbox.pid - 1)] = cmbox.temp;
                }

                /* Add up all the temps as we go for the temperature
                * algorithm */
                sumTemp1 += cmbox.temp;
            } else if ((i >= GROUP1_SIZE) && group2_unstable) {
                result = msgrcv( msqidC2, &cmbox, length, 2, 0);
                if (result == -1)
                    fprintf(stderr, "Failed to receive messsage. "
                            "External.c");
                /* If any of the new temps are different from the old 
                 *  temps then we are still unstable. Set the new temp
                 *  to the corresponding process ID in the array */
                if(tempAry[(cmbox.pid - 1)] != cmbox.temp) {
                    stable2 = 0;
                    tempAry[(cmbox.pid - 1)] = cmbox.temp;
                }

                /* Add up all the temps as we go for the temperature
                * algorithm */
                sumTemp2 += cmbox.temp;
            }
        }

        /* When all the processes have the same temp twice: 1) Break the
         * loop 2) Set the messages stable field to stable*/
        if (group1_unstable) {
            msgp.temp = cTemp1;
            if(stable1) {
                printf("Group1 Temperature Stabilized: %d\n", cTemp1);
                group1_unstable = 0;
                msgp.stable = 1;
            } else { /* Calculate a new temp and set the temp field in the
                      * message */
                int newTemp = (cTemp1 + 1000*sumTemp1) /
                              (1000*GROUP1_SIZE + 1);
                usleep(100000);
                cTemp1 = newTemp;
                msgp.temp = newTemp;
                printf("The new temp in group1 central is %d\n",newTemp);
                msgp.stable = 0;
            }
            /* Send a new message to all processes to inform of new temp or
             * stability */
            for(i = 0; i < GROUP1_SIZE; i++) {
                result = msgsnd( msqid[i], &msgp, length, 0);
                if (result == -1)
                    fprintf(stderr, "Failed to send messsage. External.c");
#if 0
                printf("group1 sent msg to msqid: %d\n", msqid[i]);
#endif
            }
        }

        if (group2_unstable) {
            msgp.temp = cTemp2;
            if(stable2) {
                printf("Group2 Temperature Stabilized: %d\n", cTemp2);
                group2_unstable = 0;
                msgp.stable = 1;
            } else { /* Calculate a new temp and set the temp field in the
                      * message */
                int newTemp = (msgp.temp + 1000*sumTemp2) /
                              (1000*GROUP2_SIZE + 1);
                usleep(100000);
                cTemp2 = newTemp;
                msgp.temp = newTemp;
                printf("The new temp in group2 central is %d\n",newTemp);
                msgp.stable = 0;
            }
            /* Send a new message to all processes to inform of new temp or
             * stability */
            for(i = GROUP1_SIZE; i < NUM_PROCESSES; i++) {
                result = msgsnd( msqid[i], &msgp, length, 0);
                if (result == -1)
                    fprintf(stderr, "Failed to send messsage. External.c");
#if 0
                printf("group2 sent msg to msqid: %d\n", msqid[i]);
#endif
            }
        }

        if ((group1_unstable == 0) && (group2_unstable == 0))
            unstable = 0;
    }

    usleep(100000);
    printf("\nShutting down Server...\n");

    /* Remove mailbox1 */
    status = msgctl(msqidC1, IPC_RMID, 0);

    /* Validate nothing when wrong when trying to remove mailbox */
    if(status != 0)
        printf("\nERROR closing mailbox\n");

    /* Remove mailbox2 */
    status = msgctl(msqidC2, IPC_RMID, 0);

    /* Validate nothing when wrong when trying to remove mailbox */
    if(status != 0)
        printf("\nERROR closing mailbox\n");

    /* stop timer */
    gettimeofday(&t2, NULL);

    /* compute and print the elapsed time in millisec */
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;

    /* sec to ms */
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; /* us to ms */
    usleep(100000);
    printf("The elapsed time is %fms\n", elapsedTime);
    return 0;
}
