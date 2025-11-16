#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h> 

#define Q_SIZE 100
#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 5
#define MAX_ENTRIES 20
#define MAX_PROD_MESSAGES 9
#define MAX_PROD_WAIT 2
#define MAX_CONS_WAIT 4
#define RANDOM_NUM_RANGE 9

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
=============
QUEUE STRUCTS
=============
*/

// Message containing a data value and a priority.
typedef struct {
    int data_value;
    int priority;
} message_t;

// Heap of messages (MAX HEAP)
typedef struct {
    message_t entries[Q_SIZE];
    int size;
} prio_queue_t;

/* 
=======================================
QUEUE FUNCTIONS : Priority Queue / Heap
=======================================
*/

void initQueue(prio_queue_t* queue){
    queue->size = 0;
}

// HEAP: Swap message between two message pointers.
void swap(message_t* a, message_t*b){
    message_t temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify starting from a given element i
// To restore heap structure, use restoreHeap()
void heapify(prio_queue_t *heap, int i)
{
    int largest = i; // Set current element index as largest
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    // Check if left child is larger than current largest element.
    if (left < heap->size && heap->entries[left].priority > heap->entries[largest].priority)
        largest = left;

    // Check if left child is larger than current largest element.
    if (right < heap->size && heap->entries[right].priority > heap->entries[largest].priority)
        largest = right;

    // Repeats for all sub trees, working up the tree.
    if (largest != i){
        swap(&heap->entries[i],&heap->entries[largest]);
    }
    if (i > 0){
        i--;
        heapify(heap,i--);
    }
}

// Restores heap structure by starting from the first non-leaf index.
void restoreHeap(prio_queue_t* heap){
    if(heap->size>1){
        heapify(heap, (heap->size)/2 -1);
    }
}

void heapInsert(prio_queue_t *heap, message_t* message){
    // Add message to next spot in heap
    heap->entries[heap->size] = *message;
    heap->size++;

    // Heapify tree
    restoreHeap(heap);
}

message_t heapRemove(prio_queue_t *heap, int index){
    // Swap index with last node in the heap.
    swap(&heap->entries[index], &heap->entries[heap->size - 1]);

    // Cut the last element from the heap.
    heap->size--;

    // Heapify tree
    restoreHeap(heap);
    // Return the cut element.
    return heap->entries[heap->size];

}

// Check if queue is empty or full. Returns 1 if true, 0 if false.
int isEmpty(prio_queue_t* queue) { 
    return (queue->size == 0); 
}
int isFull(prio_queue_t* queue) { 
    return (queue->size == Q_SIZE); 
}

// Add message to queue. If successful, returns 0. If queue is full, returns -1.
int enqueue(prio_queue_t* queue, message_t* message){
    if(isFull(queue)){
        printf("Queue full!\n");
        return -1;
    }
    heapInsert(queue, message);
    return 0;
}

// Reads and removes message in queue. 
// If successful, returns the data value. If queue is full, returns -1.
int dequeue(prio_queue_t* queue){
    // Check if queue is empty
    if(isEmpty(queue)){
        printf("Queue empty!\n");
        return -1;
    }
    message_t message = heapRemove(queue, 0);
    int data_value = message.data_value;
    return data_value;
}

/*
================
THREAD FUNCTIONS
================
*/

// THREAD: Producer process

// THREAD: PRODUCER : Writes message to the queue with mutex
void writeMessage(prio_queue_t *queue, message_t * message){
    pthread_mutex_lock(&mutex);
    enqueue(queue, message);
    pthread_mutex_unlock(&mutex);
}

// THREAD: CONSUMER : Reads message from the queue with mutex
int readMessage(prio_queue_t* queue){
    pthread_mutex_lock(&mutex);
    int data_value = dequeue(queue);
    pthread_mutex_unlock(&mutex);
    return data_value;
}

// Generate message with random value and given priority 
message_t generateMessage(int priority){
    int data_value = rand() % 10;
    message_t message;
    message.data_value = data_value;
    message.priority = priority;
    return message;
}

// THREAD: Producer process
void* producerThread(void* arg){
    // Assign random priority from 0-9
    int priority = rand() % 10;

    // Recast void* arg to unpack shared_queue address
    prio_queue_t * shared_queue = (prio_queue_t*) arg;

    // Generate a stream of random integer data values after a random time from 1 - 2 seconds
    // Then write onto shared queue
    for (int i = 0; i < 4; i++){
        // Wait for a random time
        sleep(rand() % MAX_PROD_WAIT + 1);

        // Generate a message
        message_t message = generateMessage(priority);

        // Write message onto queue
        writeMessage(shared_queue, &message);
    }
    return 0;
}

// THREAD: Consumer process
void* consumerThread(void* arg){

    // Recast void* arg to unpack shared_queue address
    prio_queue_t * shared_queue = (prio_queue_t*) arg;

    // Read after a random time from 1-MAX_CONS_WAIT seconds
    for (int i = 0; i < 4; i++){
        if(shared_queue){
            int data_value = readMessage(shared_queue);
            printf("Consumer %d read: %d\n", gettid(), data_value);
            if (data_value > 9){
                printf("??");
            }
        }
        sleep(rand() % MAX_CONS_WAIT + 1);
    }
    return 0;
}

// THREAD: Create NUM_PRODUCER threads that run producerThread() with arguments in thread_data
void createProducerThreads(pthread_t* threads, prio_queue_t* shared_queue){
    int num_producers = NUM_PRODUCERS; // to use in for loop
    for (int i = 0; i < num_producers; i++) {

        // Passing shared_queue as argument into thread
        void* args = (void*) shared_queue;

        pthread_attr_t* threadAttributes = NULL; // Unused
        
        if(pthread_create(&threads[i], threadAttributes, producerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Create NUM_CONSUMER threads that run consumerThread()
void createConsumerThreads(pthread_t* threads, prio_queue_t* shared_queue){
    int num_consumers = NUM_CONSUMERS; // to use in for loop
    for (int i = 0; i < num_consumers; i++) {

        // Passing shared_queue as argument into thread
        void* args = (void*) shared_queue;

        pthread_attr_t* threadAttributes = NULL;
        if(pthread_create((pthread_t*) &threads[i], threadAttributes, consumerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Primary threading function
void runThreads(prio_queue_t*shared_queue){

    // to use in for loops
    int num_producers = NUM_PRODUCERS; 
    int num_consumers = NUM_CONSUMERS; 

     // Store all threads in an array so they can be joined.
    pthread_t producer_threads[num_producers];
    pthread_t consumer_threads[num_consumers];

    // Create threads and store them in respective arrays.
    createProducerThreads(producer_threads,shared_queue); 
    createConsumerThreads(consumer_threads,shared_queue); 

    // Wait for all threads in arrays to terminate
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }

    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }
}

/*
====
MAIN
====
*/

int main(int argc, char *argv[]) {
    srand(time(NULL)); // Random seed
    // Initialize shared queue.
    prio_queue_t shared_queue; 
    initQueue(&shared_queue);

    // Pass the pointer to the shared queue to the threads.
    runThreads(&shared_queue);
};