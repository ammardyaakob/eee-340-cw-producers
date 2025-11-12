#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#define Q_SIZE 10
#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 3
#define MAX_ENTRIES 20
#define MAX_PROD_MESSAGES 9
#define MAX_PROD_WAIT 2
#define MAX_CONS_WAIT 4

typedef struct {
    int data_value;
    int priority;
} message_t;

typedef struct {
    message_t entries[Q_SIZE];
    int head, tail, count;
} queue_t;

void producerChild(int id, queue_t* shared_queue){
    srand(getpid());
    int priority = rand() % 10;
}

void consumerChild(int id, queue_t* shared_queue){
    srand(getpid());
    for (int i = 0; i < 4; i++){
        printf("Consumer %d read\n", id);
        sleep(rand() % MAX_CONS_WAIT + 1);
    }

}

void instantiateProducers(int num_producers, queue_t *shared_queue){
    for (int i = 0; i < num_producers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            producerChild(i, shared_queue);
            exit(0);
        }
    }
}

void instantiateConsumers(int num_producers, queue_t *shared_queue){
    for (int i = 0; i < num_producers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            consumerChild(i, shared_queue);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]) {

    int status;
    srand(time(NULL));
    queue_t shared_queue;
    int num_producers = NUM_PRODUCERS;
    int num_consumers = NUM_CONSUMERS;
    
    instantiateProducers(num_producers, &shared_queue);
    instantiateConsumers(num_consumers, &shared_queue);

    // Wait for all children to terminate.
    for (int i = 0; i < NUM_CONSUMERS + NUM_PRODUCERS; i++){
        waitpid(-1, &status, 0);
    }
    
};
