#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h> 

#define Q_SIZE 10
#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 5
#define MAX_ENTRIES 20
#define MAX_PROD_MESSAGES 9
#define MAX_PROD_WAIT 2
#define MAX_CONS_WAIT 4
#define RANDOM_NUM_RANGE 9

/*
=============
QUEUE STRUCTS
=============
*/

typedef struct {
    int data_value;
    int priority;
} message_t;

typedef struct {
    message_t entries[Q_SIZE];
    int head, tail, count;
} queue_t;

/*
================
THREAD FUNCTIONS
================
*/

// THREAD: Producer process
void* producerThread(void* arg){
    // Assign random priority from 0-9
    int priority = rand() % 10;

    // Generate a stream of random integer data values after a random time from 1 - 2 seconds.
    // Then write onto shared queue
    for (int i = 0; i < 4; i++){
        sleep(rand() % MAX_PROD_WAIT + 1);
        int data_value = rand() % 10;
        message_t *message;
        message->data_value = data_value;
        printf("Message from producer %d: %d\n", gettid(),message->data_value);
    }
}

// THREAD: Consumer process
void* consumerThread(void* arg){
    // Read after a random time from 1-MAX_CONS_WAIT seconds
    for (int i = 0; i < 4; i++){
        printf("Consumer %d read\n", gettid());
        sleep(rand() % MAX_CONS_WAIT + 1);
    }
}

// THREAD: Create NUM_PRODUCER Athreads that run producerThread()
void createProducerThreads(pthread_t* threads){
    int num_producers = NUM_PRODUCERS; // to use in for loop
    
    for (int i = 0; i < num_producers; i++) {
        pthread_attr_t* threadAttributes = NULL; 
        void* args = NULL;
        if(pthread_create(&threads[i], threadAttributes, producerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Create NUM_CONSUMER threads that run consumerThread()
void createConsumerThreads(pthread_t* threads){
    int num_consumers = NUM_CONSUMERS; // to use in for loop
    
    for (int i = 0; i < num_consumers; i++) {
        pthread_attr_t* threadAttributes = NULL; 
        void* args = NULL;
        if(pthread_create((pthread_t*) &threads[i], threadAttributes, consumerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Primary threading function
void runThreads(queue_t *shared_queue){

    int num_producers = NUM_PRODUCERS; // to use in for loop

     // Store all threads in an array
    pthread_t producer_threads[num_producers];
    pthread_t consumer_threads[num_producers];

    // Create threads and store them in respective arrays.
    createProducerThreads(producer_threads); 
    createConsumerThreads(consumer_threads); 

    // Wait for all threads in arrays to terminate
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
        pthread_join(consumer_threads[i], NULL);
    }

}

/*
=============
PID FUNCTIONS
=============
*/

// PID: Producer process
void producerChild(int id, queue_t* shared_queue){
    srand(getpid());
    int priority = rand() % 10;
}

// PID: Consumer process
void consumerChild(int id, queue_t* shared_queue){
    srand(getpid());
    for (int i = 0; i < 4; i++){
        printf("Consumer %d read\n", id);
        sleep(rand() % MAX_CONS_WAIT + 1);
    }
}

// PID: Create producers
void instantiateProducers(int num_producers, queue_t *shared_queue){
    for (int i = 0; i < num_producers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            producerChild(i, shared_queue);
            exit(0);
        }
    }
}

// PID: Create consumers
void instantiateConsumers(int num_producers, queue_t *shared_queue){
    for (int i = 0; i < num_producers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            consumerChild(i, shared_queue);
            exit(0);
        }
    }
}

/*
====
MAIN
====
*/

int main(int argc, char *argv[]) {

    srand(time(NULL)); // Random seed
    queue_t shared_queue;
    int num_producers = NUM_PRODUCERS;
    int num_consumers = NUM_CONSUMERS;

    //int status; // PID: stores return value of children
    //void* retval; // Thread: stores return value of children
    
    // Create child processes
    //instantiateProducers(num_producers, &shared_queue);
    //instantiateConsumers(num_consumers, &shared_queue);
    runThreads(&shared_queue);

    // Wait for all children to terminate.
    /*
    for (int i = 0; i < NUM_CONSUMERS + NUM_PRODUCERS; i++){
        waitpid(-1, &status, 0);
    }
    */
};
