/*
 * central.c
 *
 * Michael Vescovo s3459317
 */

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CENTRAL_MAILBOX 1200 //Central Mailbox number
#define NUM_PROCESSES 4

//Total number of external processes
struct {
long priority;
int temp;
int pid;
int stable;
} msgp, cmbox;

//message priority
//temperature
//process id
//boolean for temperature stability

//MAIN function
int main(int argc, char *argv[]) {
    struct timeval t1, t2;
    double elapsedTime;

    // start timer
    gettimeofday(&t1, NULL);

    //Validate that a temperature was given via the command line
    if(argc != 2) {
        printf("USAGE: Too few arguments --./central.out Temp");
        exit(0);
    }

    printf("\nStarting Server...\n");

    //Set up local variables
    int i,result,length,status;

    //counter for loops
    int uid = 0;

    //central process ID
    int initTemp = atoi(argv[1]);

    //starting temperature
    int msqid[NUM_PROCESSES];

    //mailbox IDs for all processes
    int unstable = 1;

    //boolean to denote temp stability
    int tempAry[NUM_PROCESSES]; //array of process temperatures

    //Create the Central Servers Mailbox
    int msqidC = msgget(CENTRAL_MAILBOX, 0600 | IPC_CREAT);

    //Create the mailboxes for the other processes and store their IDs
    for(i = 1; i <= NUM_PROCESSES; i++){
        msqid[(i-1)] = msgget((CENTRAL_MAILBOX + i), 0600 | IPC_CREAT);
    }

    //Initialize the message to be sent
    msgp.priority = 1;
    msgp.pid = uid;
    msgp.temp = initTemp;
    msgp.stable = 1;

    /* The length is essentially the size of the structure minus
     * sizeof(mtype) */
    length = sizeof(msgp) - sizeof(long);

    //While the processes have different temperatures
    while(unstable == 1){
        int sumTemp = 0;
        int stable = 1;
        RMIT University - S2 2015

        //sum up the temps as we loop
        //stability trap
        // Get new messages from the processes
        for(i = 0; i < NUM_PROCESSES; i++){
            result = msgrcv( msqidC, &cmbox, length, 1, 0);
            /* If any of the new temps are different from the old temps then
             * we are still unstable. Set the new temp to the corresponding
             * process ID in the array */
            if(tempAry[(cmbox.pid - 1)] != cmbox.temp) {
                stable = 0;
                tempAry[(cmbox.pid - 1)] = cmbox.temp;
            }

            //Add up all the temps as we go for the temperature algorithm
            sumTemp += cmbox.temp;
        }

        /* When all the processes have the same temp twice: 1) Break the loop
         * 2) Set the messages stable field to stable*/
        if(stable){
            printf("Temperature Stabilized: %d\n", msgp.temp);
            unstable = 0;
            msgp.stable = 0;
        } else { //Calculate a new temp and set the temp field in the message
            int newTemp = (msgp.temp + 1000*sumTemp) / (1000*NUM_PROCESSES + 1);
            usleep(100000);
            msgp.temp = newTemp;
            printf("The new temp in central is %d\n",newTemp);
        }

        /* Send a new message to all processes to inform of new temp or
         * stability */
        for(i = 0; i < NUM_PROCESSES; i++)
            result = msgsnd( msqid[i], &msgp, length, 0);
    }

    printf("\nShutting down Server...\n");

    //Remove the mailbox
    status = msgctl(msqidC, IPC_RMID, 0);

    //Validate nothing when wrong when trying to remove mailbox
    if(status != 0)
        printf("\nERROR closing mailbox\n");


    // stop timer
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;

    // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms
    printf("The elapsed time is %fms\n", elapsedTime);
    return 0;
}
