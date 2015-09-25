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
#include <errno.h>

#define NUM_PROCESSES 4 /* Total number of external processes */

struct {
    long priority; /* message priority */
    int temp; /* temperature */
    int pid; /* process id */
    int stable; /* boolean for temperature stability */
    int group; /* process group */
} msgp, cmbox;

/* MAIN function */
int main(int argc, char *argv[]) {
    struct timeval t1, t2;
    double elapsedTime;
    int i, length;
    int result,status; /* counter for loops */

    /* variables for group 1 */
    int uid1 = 0; /* central process ID */
    int initTemp1; /* starting temperature */
    int msqid1[NUM_PROCESSES]; /* mailbox IDs for all processes */
    int unstable1 = 1; /* boolean to denote temp stability */
    int tempAry1[NUM_PROCESSES]; /* array of process temperatures */

    /* variables for group 1 */
    int uid2 = 0; /* central process ID */
    int initTemp2; /* starting temperature */
    int msqid2[NUM_PROCESSES]; /* mailbox IDs for all processes */
    int unstable2 = 1; /* boolean to denote temp stability */
    int tempAry2[NUM_PROCESSES]; /* array of process temperatures */

    /* Create the Central Servers Mailbox */
    int msqidC1 = msgget(CENTRAL_MAILBOX1, 0600 | IPC_CREAT);
    int msqidC2 = msgget(CENTRAL_MAILBOX2, 0600 | IPC_CREAT);

    int length2 = sizeof(group1) - sizeof(long);

    /* start timer */
    gettimeofday(&t1, NULL);

    /* Validate that a temperature was given via the command line */
    if(argc != 5) {
        printf("USAGE: Too few arguments --./central.out Temp");
        exit(0);
    }
    initTemp1 = atoi(argv[1]);
    initTemp2 = atoi(argv[2]);

    printf("\nStarting Server...\n");
    printf("\nEACCES: %d", EACCES);
    printf("\nEAGAIN: %d", EAGAIN);
    printf("\nEFAULT: %d", EFAULT);
    printf("\nEIDRM: %d", EIDRM);
    printf("\nEINTR: %d", EINTR);
    printf("\nEINVAL: %d", EINVAL);
    printf("\nENOMEM: %d", ENOMEM);
    printf("\nEFAULT: %d", EFAULT);

    /* Create the mailboxes for the other processes and store their IDs */
    for(i = 1; i <= NUM_PROCESSES; i++){
        msqid1[(i-1)] = msgget((CENTRAL_MAILBOX1 + i), 0600 | IPC_CREAT);
    }
    for(i = NUM_PROCESSES + 1; i <= NUM_PROCESSES * 2; i++){
        msqid2[(i-1)] = msgget((CENTRAL_MAILBOX2 + i), 0600 | IPC_CREAT);
    }

    /* Tell the group1 processes what group they're in */
    for (i = 1; i <= NUM_PROCESSES; i++) {
        group1.mtype = 1;
        group1.group[0] = '1';
        result1 = msgsnd( msqid1[i], &group1, length2, 0);
        if (result1 == -1) {
            int errsv = errno;
            printf("\nmsqid: %d", msqid1[i]);
            printf("\nmsqid: %d", msqid1[i]);
            fprintf(stderr, "\nFailed to send messsage. central.c "
                    "errno: %d", errsv);
        }
    }

    /* Tell the group2 processes what group they're in */
    for (i = NUM_PROCESSES + 1; i <= NUM_PROCESSES * 2; i++) {
        group2.mtype = 1;
        group2.group[0] = '2';
        result2 = msgsnd( msqid2[i], &group2, length2, 0);
        if (result1 == -1)
            fprintf(stderr, "\nFailed to send messsage. central.c");
    }

    /* Initialize the message1 to be sent */
    msgp1.priority = 1;
    msgp1.pid = uid1;
    msgp1.temp = initTemp1;
    msgp1.stable = 1;

    /* Initialize the message2 to be sent */
    msgp2.priority = 1;
    msgp2.pid = uid2;
    msgp2.temp = initTemp2;
    msgp2.stable = 1;

    /* The length is essentially the size of the structure minus
     * sizeof(mtype) */
    length = sizeof(msgp1) - sizeof(long);

    /* While the processes have different temperatures */
    while((unstable1 == 1) || (unstable2 == 1)){
        int sumTemp1 = 0; /* sum up the temps as we loop */
        int stable1 = 1; /* stability trap */
        int sumTemp2 = 0; /* sum up the temps as we loop */
        int stable2 = 1; /* stability trap */

        /* Get new messages from the processes */
        for(i = 0; i < NUM_PROCESSES; i++){
            result1 = msgrcv( msqidC1, &cmbox1, length, 1, 0);
            if (result1 == -1)
                fprintf(stderr, "\nFailed to receive messsage. central.c");
            result2 = msgrcv( msqidC2, &cmbox2, length, 1, 0);
            if (result2 == -1)
                fprintf(stderr, "\nFailed to receive messsage. central.c");
            /* If any of the new temps are different from the old temps then
             * we are still unstable. Set the new temp to the corresponding
             * process ID in the array */
            if(tempAry1[(cmbox1.pid - 1)] != cmbox1.temp) {
                stable1 = 0;
                tempAry1[(cmbox1.pid - 1)] = cmbox1.temp;
            }
            if(tempAry2[(cmbox2.pid - 1)] != cmbox2.temp) {
                stable2 = 0;
                tempAry2[(cmbox2.pid - 1)] = cmbox2.temp;
            }
            /* Add up all the temps as we go for the temperature
             * algorithm */
            sumTemp1 += cmbox1.temp;
            sumTemp2 += cmbox2.temp;
        }

        /* When all the processes have the same temp twice: 1) Break the
         * loop 2) Set the messages stable field to stable*/
        if(stable1) {
            printf("Group 1 Temperature Stabilized: %d\n", msgp1.temp);
            unstable1 = 0;
            msgp1.stable = 0;
        } else { /* Calculate a new temp and set the temp field in the
                  * message */
            int newTemp1 = (msgp1.temp + 1000*sumTemp1) /
                          (1000*NUM_PROCESSES + 1);
            usleep(100000);
            msgp1.temp = newTemp1;
            printf("The new temp in central group 1 is %d\n",newTemp1);
        }

        if(stable2) {
            printf("Group 2 Temperature Stabilized: %d\n", msgp2.temp);
            unstable2 = 0;
            msgp2.stable = 0;
        } else { /* Calculate a new temp and set the temp field in the
                  * message */
            int newTemp2 = (msgp2.temp + 1000*sumTemp2) /
                          (1000*NUM_PROCESSES + 1);
            usleep(100000);
            msgp2.temp = newTemp2;
            printf("The new temp in central group 2 is %d\n",newTemp2);
        }

        /* Send a new message to all processes to inform of new temp or
         * stability */
        for(i = 0; i < NUM_PROCESSES; i++) {
            result1 = msgsnd( msqid1[i], &msgp1, length, 0);
            if (result1 == -1)
                fprintf(stderr, "\nFailed to send messsage. central.c");
            result2 = msgsnd( msqid2[i], &msgp2, length, 0);
            if (result2 == -1)
                fprintf(stderr, "\nFailed to send messsage. central.c");
        }
    }

    usleep(100000);
    printf("\nShutting down Server...\n");

    /* Remove the mailbox */
    status1 = msgctl(msqidC1, IPC_RMID, 0);
    status2 = msgctl(msqidC2, IPC_RMID, 0);

    /* Validate nothing when wrong when trying to remove mailbox */
    if((status1 != 0) && (status2 != 0))
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
