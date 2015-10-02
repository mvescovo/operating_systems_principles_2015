/*
 * central442T.c
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
#include <pthread.h>

#define NUM_PROCESSES 8 /* Total number of external processes */
#define GROUP1_SIZE 4
#define GROUP2_SIZE 4

struct msg {
    long priority; /* message priority */
    int temp; /* temperature */
    int pid; /* process id */
    int stable; /* boolean for temperature stability */
} msgp1, msgp2, cmbox1, cmbox2;

struct buff {
    int group;
    struct msg *msgp;
    struct msg *cmbox;
    int i;
    int max;
    int result;
    int status;
    int unstable;
    int msqidC;
    int cTemp;
    int (*tempAry)[NUM_PROCESSES];
    int (*msqid)[NUM_PROCESSES];
    int length;
} group1, group2;

static void* threadfunc(void *arg);

/* MAIN function */
int main(int argc, char *argv[])
{
    struct timeval t1, t2;
    double elapsedTime;
    int initTemp1, initTemp2; /* starting temperatures */
    int mailbox1, mailbox2;
    int msqid[NUM_PROCESSES]; /* mailbox IDs for all processes */
    int i, presult, length, status; /* counter for loops */

    int uid = 0; /* central process ID */
    int tempAry[NUM_PROCESSES]; /* array of process temperatures */
    pthread_t thread1; /* group 1 thread */
    pthread_t thread2; /* group 2 thread */

    /* Validate that a temperature was given via the command line */
    if(argc != 5) {
        printf("USAGE: Too few arguments --./central.out Temp");
        exit(0);
    }

    initTemp1 = atoi(argv[1]);
    initTemp2 = atoi(argv[2]);
    mailbox1 = atoi(argv[3]);
    mailbox2 = atoi(argv[4]);
    group1.cTemp = initTemp1;
    group2.cTemp = initTemp2;
    /* Create the Central Server Mailboxes */
    group1.msqidC = msgget(mailbox1, 0600 | IPC_CREAT);
    group2.msqidC = msgget(mailbox2, 0600 | IPC_CREAT);

    /* Create the mailboxes for the other processes and store their IDs */
    for(i = 1; i <= NUM_PROCESSES; i++) {
        if (i <= GROUP1_SIZE)
            msqid[(i-1)] = msgget((mailbox1 + i), 0600 | IPC_CREAT);
        else
            msqid[(i-1)] = msgget((mailbox2 + i), 0600 | IPC_CREAT);
    }

    /* The length is essentially the size of the structure minus
     * sizeof(mtype) */
    length = sizeof(struct msg) - sizeof(long);

    group1.group = 1;
    group1.msgp = &msgp1;
    group1.cmbox = &cmbox1;
    group1.i = 0;
    group1.max = GROUP1_SIZE;
    group1.result = 0;
    group1.status = 0;
    group1.unstable = 1;
    group1.msgp->priority = 2;
    group1.msgp->pid = uid;
    group1.tempAry = &tempAry;
    group1.msqid = &msqid;
    group1.length = length;

    group2.group = 2;
    group2.msgp = &msgp2;
    group2.cmbox = &cmbox2;
    group2.i = GROUP1_SIZE;
    group2.max = NUM_PROCESSES;
    group2.result = 0;
    group2.status = 0;
    group2.unstable = 1;
    group2.msgp->priority = 2;
    group2.msgp->pid = uid;
    group2.tempAry = &tempAry;
    group2.msqid = &msqid;
    group2.length = length;

    printf("\nStarting Server...\n");

    /* start timer */
    gettimeofday(&t1, NULL);

    /* create new thread for group 1 */
    if (pthread_create(&thread1, NULL, threadfunc, &group1) != 0)
        fprintf(stderr, "Failed to create thread1.\n");
    /* create new thread for group 2 */
    if (pthread_create(&thread2, NULL, threadfunc, &group2) != 0)
        fprintf(stderr, "Failed to create thread2.\n");

    printf("Message from main\n");
    presult = pthread_join(thread1, NULL);
    if (presult != 0)
        fprintf(stderr, "Failed to join thread.\n");
    printf("Thread1 returned.\n");
    presult = pthread_join(thread2, NULL);
    if (presult != 0)
        fprintf(stderr, "Failed to join thread.\n");
    printf("Thread2 returned.\n");

    usleep(100000);
    printf("\nShutting down Server...\n");

    /* Remove mailbox1 */
    status = msgctl(group1.msqidC, IPC_RMID, 0);

    /* Validate nothing when wrong when trying to remove mailbox */
    if(status != 0)
        printf("\nERROR closing mailbox\n");

    /* Remove mailbox2 */
    status = msgctl(group2.msqidC, IPC_RMID, 0);

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

void * threadfunc(void *arg)
{
    struct buff *buff = (struct buff*) arg;

    printf("thread %d started.\n", buff->group);

    /* While the processes have different temperatures */
    while(buff->unstable == 1){
        int sumTemp = 0; /* sum up the temps as we loop */
        int stable = 1; /* stability trap */
        int i;
        
#if 1
        /* Get new messages from the processes */
        for(i = buff->i; i < buff->max; i++) {
            if ((i < buff->max) && buff->unstable) {
                buff->result = msgrcv( buff->msqidC, buff->cmbox,
                                       buff->length,
                                     2, 0);
                if (buff->result == -1)
                    fprintf(stderr, "Failed to receive messsage. "
                            "External.c");
                /* If any of the new temps are different from the old 
                 *  temps then we are still unstable. Set the new temp
                 *  to the corresponding process ID in the array */
                if((*(buff->tempAry))[(buff->cmbox->pid - 1)] != 
                   buff->cmbox->temp) {
                    stable = 0;
                    (*(buff->tempAry))[(buff->cmbox->pid - 1)] =
                    buff->cmbox->temp;
                }

                /* Add up all the temps as we go for the temperature
                * algorithm */
                sumTemp += buff->cmbox->temp;
            }
        }

        /* When all the processes have the same temp twice: 1) Break the
         * loop 2) Set the messages stable field to stable*/
        if (buff->unstable) {
            buff->msgp->temp = buff->cTemp;
            if(stable) {
                printf("Group%d Temperature Stabilized: %d\n", buff->group,
                                                               buff->cTemp);
                buff->unstable = 0;
                buff->msgp->stable = 1;
            } else { /* Calculate a new temp and set the temp field in the
                      * message */
                int newTemp = (buff->cTemp + 1000*sumTemp) /
                              (1000*GROUP1_SIZE + 1); /* TODO */
                usleep(100000);
                buff->cTemp = newTemp;
                buff->msgp->temp = newTemp;
                printf("The new temp in group%d central is %d\n",
                        buff->group, newTemp);
                buff->msgp->stable = 0;
            }
            /* Send a new message to all processes to inform of new temp or
             * stability */
            for(i = buff->i; i < buff->max; i++) { /* TODO */
                buff->result = msgsnd( (*(buff->msqid))[i], buff->msgp,
                                        buff->length, 0);
                if (buff->result == -1)
                    fprintf(stderr, "Failed to send messsage. External.c");
#if 0
                printf("group %d sent msg to msqid: %d\n", buff->group, 
                        (*(buff->msqid))[i]);
#endif
            }
        }
#endif
    }
    return 0;
}
