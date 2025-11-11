#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#define Q_SIZE 10
#define NUM_PRODUCERS 5
#define NUM_CONSUMERS 5

typedef struct {
    int message;
    int priority;
} entry_t;

typedef struct {
    entry_t entries[Q_SIZE];
    int head, tail, count;
} queue_t;

void producerChild(int id, queue_t* shared_queue){
    printf("Im producer child %d looking at address queue %d \n", id, shared_queue);
}

void consumerChild(int id, queue_t* shared_queue){
    printf("Im consumer child %d looking at address queue %d \n", id, shared_queue);
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
    printf("Hello world!\n");
    queue_t shared_queue;
    int num_producers = NUM_PRODUCERS;
    int num_consumers = NUM_CONSUMERS;
    instantiateProducers(num_producers, &shared_queue);
    instantiateConsumers(num_consumers, &shared_queue);
};
