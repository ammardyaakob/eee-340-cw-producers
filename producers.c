#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#define Q_SIZE 20
#define NUM_PRODUCERS 20

typedef struct {
    int message;
    int priority;
} entry_t;

typedef struct {
    entry_t entries[Q_SIZE];
    int head, tail, count;
} queue_t;

void producerChild(int id, int* shared_queue){
    printf("Im child %d \n", id);
}

void instantiateProducers(int num_producers, queue_t shared_queue){
    for (int i = 0; i < num_producers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            producerChild(i, &shared_queue);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("Hello world!\n");
    queue_t shared_queue;
    int num_producers = 10;
    instantiateProducers(num_producers, shared_queue);
};
