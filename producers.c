#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h> 
#include <string.h>
#include <ctype.h>

#define Q_SIZE 5
#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 5
#define MAX_ENTRIES 20
#define MAX_PROD_MESSAGES 9
#define MAX_PROD_WAIT 2
#define MAX_CONS_WAIT 4
#define RANDOM_NUM_RANGE 9
#define TIMEOUT_VALUE 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
=======
STRUCTS
=======
*/

// QUEUE: Message containing a data value and a priority.
typedef struct {
    int data_value;
    int priority;
} message_t;

// QUEUE: Messages heap / priority queue
typedef struct {
    message_t entries[Q_SIZE];
    int size;
} prio_queue_t;

// THREAD : Struct to pass pointers into threads
struct t_data_t{
    prio_queue_t* shared_queue;
    int* timer;
    int* num_producers;
    int* num_consumers;
    int* timeout_value;
};

/* 
=======================================
QUEUE FUNCTIONS : Priority Queue / Heap
=======================================
The heap is a data structure that always has the highest value and earliest entry
as the root node. The value the heap is ordered in here is the priority, so the
highest priority and earliest entry will always be the root node which will be the 
node that will be accessed when a consumer reads. It is implemented as a max heap.
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
         // First non-leaf index is (n/2) - 1
         // Where n is the current amount of nodes in the heap.
        heapify(heap, (heap->size)/2 -1);
    }
}

void heapInsert(prio_queue_t *heap, message_t* message){
    // Add message to next spot in heap
    heap->entries[heap->size] = *message;
    heap->size++;

    // Restore heap property.
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

// Add message to queue if not full. If successful, returns 0. If queue is full, returns -1.
int enqueue(prio_queue_t* queue, message_t* message){
    if(isFull(queue)){
        // DEV: Print if queue was full on write
        printf("Queue full!\n");
        return -1;
    }
    heapInsert(queue, message);
    return 0;
}

// Reads and removes a message in queue if there is one. 
// If successful, returns the data value. If queue is empty, returns -1.
int dequeue(prio_queue_t* queue){
    // Check if queue is empty
    if(isEmpty(queue)){
        // DEV: Print if queue was empty on read
        printf("Queue empty!\n");
        return -1;
    }
    // Read a message from the heap by removing the root node 
    // which returns the message removed.
    message_t message = heapRemove(queue, 0);
    // Return the data value read from the root node.
    int data_value = message.data_value;
    return data_value;
}

/*
================
THREAD FUNCTIONS
================
Mutex is called here because enqueue() and dequeue() immediately checks if the queue is empty.
If the check is done before the mutex, the thread wouldn't know if the queue is empty
after a mutex wait.
*/

// THREAD: PRODUCER : Writes message to the queue with mutex
int writeMessage(prio_queue_t *queue, message_t * message){

    pthread_mutex_lock(&mutex);
    int retval = enqueue(queue, message);
    pthread_mutex_unlock(&mutex);
    return retval;
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

// Timer process
void* timerThread(void* arg){
    // Recast void* arg to unpack timer address
    struct t_data_t* t_data = (struct t_data_t*) arg;
    int * timer = (int*) t_data->timer;

    // Increment timer until timeout value in seconds
    for (int i = 0; i < TIMEOUT_VALUE; i++){
        sleep(1);
        *timer = *timer + 1;
    }
    return 0;
}

// THREAD: Producer process
void* producerThread(void* arg){
    // Assign random priority from 0-9
    int priority = rand() % 10;

    // Recast void* arg to unpack shared_queue and timer address
    struct t_data_t* t_data = (struct t_data_t*) arg;
    prio_queue_t * shared_queue = (prio_queue_t*) t_data->shared_queue;
    int * timer = (int*) t_data->timer;

    // Integer to store amount of successful writes by this thread.
    int writes = 0;

    // Generate a stream of random integer data values after a random time from 1 - 2 seconds
    // Then write onto shared queue
    while(*timer < TIMEOUT_VALUE){
        if(shared_queue){
            // retval stores writeMessage() return value. if -1, 
            // queue was full and message is not written.
            int retval = -1;

            // Generate a message
            message_t message = generateMessage(priority);

            // Loop until message is successfully written.
            while(retval == -1 && *timer < TIMEOUT_VALUE){
                // Write message onto queue
                retval = writeMessage(shared_queue, &message);
                // Wait for a random time
                sleep(rand() % MAX_PROD_WAIT + 1);
            }
            writes++;
        }
    }
    return ((void*) writes);
}

// THREAD: Consumer process
void* consumerThread(void* arg){

    // Recast void* arg to unpack shared_queue and timer address
    struct t_data_t* t_data = (struct t_data_t*) arg;
    prio_queue_t * shared_queue = (prio_queue_t*) t_data->shared_queue;
    int * timer = (int*) t_data->timer;

    // Integer to store amount of successful reads by this thread.
    int reads = 0;
    // Read after a random time from 1-MAX_CONS_WAIT seconds
    for(int i = 0; i < 10; i++){
        if(shared_queue && *timer < TIMEOUT_VALUE){
            int data_value = readMessage(shared_queue);

            // DEV: Print out data. If readMessage() returns -1 that means queue was empty.
            if (data_value != -1){
                printf("Consumer %d read: %d\n", gettid(), data_value);
                reads++;
            }
            else{
                printf("Consumer %d failed to read\n", gettid());
            }
            sleep(rand() % MAX_CONS_WAIT + 1);
        }
    }
    return ((void*)reads);
}

// Create timer thread
void createTimerThread(pthread_t* thread, struct t_data_t* t_data){
    // Passing shared_queue as argument into thread
    void* args = (void*) t_data;

    pthread_attr_t* threadAttributes = NULL;
    if(pthread_create(thread, threadAttributes, timerThread, args) != 0){
        printf("Unable to launch thread\n");
        exit(-1);
    }
}

// THREAD: Create NUM_PRODUCER threads that run producerThread() with arguments in thread_data
void createProducerThreads(pthread_t* threads, struct t_data_t* t_data){
    int num_producers = *t_data->num_producers; // to use in for loop

    for (int i = 0; i < num_producers; i++) {
        
        // Passing shared_queue as argument into thread
        void* args = (void*) t_data;

        pthread_attr_t* threadAttributes = NULL; // Unused
        
        if(pthread_create(&threads[i], threadAttributes, producerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Create NUM_CONSUMER threads that run consumerThread()
void createConsumerThreads(pthread_t* threads, struct t_data_t* t_data){
    int num_consumers = *t_data->num_consumers; // to use in for loop
    for (int i = 0; i < num_consumers; i++) {
        // Passing shared_queue as argument into thread
        void* args = (void*) t_data;

        pthread_attr_t* threadAttributes = NULL;
        if(pthread_create(&threads[i], threadAttributes, consumerThread, args) != 0){
            printf("Unable to launch thread\n");
            exit(-1);
        }
    }
}

// THREAD: Primary threading function
void runThreads(struct t_data_t* t_data){

    // Get number of producers and consumers from thread data passed.
    int num_producers = *t_data->num_producers; 
    int num_consumers = *t_data->num_consumers;

    // Store producer and consumer threads in an array so they can be joined.
    pthread_t producer_threads[num_producers];
    pthread_t consumer_threads[num_consumers];
    pthread_t timer_thread;

    // Create threads and store them in respective arrays.
    createProducerThreads(producer_threads,t_data); 
    createConsumerThreads(consumer_threads,t_data);
    createTimerThread(&timer_thread,t_data);

    // Wait for all threads in arrays to terminate
    // DEV: Prints out amount of reads/writes of each producer and consumer.
    for (int i = 0; i < num_producers; i++) {
        void* retval;
        pthread_join(producer_threads[i], &retval);
        printf("Producer %d exited with %d writes\n", i, (int)retval);
    }

    for (int i = 0; i < num_consumers; i++) {
        void* retval;
        pthread_join(consumer_threads[i], &retval);
        printf("Consumer %d exited with %d reads\n", i, (int)retval);
    }

    pthread_join(timer_thread,NULL);
}

/*
====
MAIN
====
*/

int main(int argc, char *argv[]) {
    // Random seed based on OS time in seconds.
    srand(time(NULL));

    // Initialize variables and shared queue.
    int num_producers = NUM_PRODUCERS; 
    int num_consumers = NUM_CONSUMERS; 
    int timeout_value = TIMEOUT_VALUE;
    int timer = 0;
    prio_queue_t shared_queue; 
    initQueue(&shared_queue);

    // Pack t_data with addresses of variables and shared queue.
    struct t_data_t t_data;
    t_data.shared_queue = &shared_queue;
    t_data.timer = &timer;
    t_data.num_producers = &num_producers;
    t_data.num_consumers = &num_consumers;
    t_data.timeout_value = &timeout_value;

    // Check for user terminal arguments.
    for (int i = 0 ; i < argc; i++){

        // Look for flag
        // -p = producers, -c = consumers, -t = timeout value, -e = number of entries in queue
        
        if (strcmp(argv[i],"-p") == 0){
            // Check if arg next to flag is a digit
            
            if (isdigit(*argv[i+1])){
                
                // Cast user argument into ints
                int user_num_producers = atoi(argv[i+1]);
                // Check if user argument is within range.
                if (user_num_producers > 0 && user_num_producers <= NUM_PRODUCERS){
                    // Change number of producers
                    num_producers = user_num_producers;
                }
                else{
                    printf("User supplied value for number of producers is out of range!\n");
                    printf("Please enter a value between 1 and %d\n", (int)NUM_PRODUCERS);     
                    return 0;           
                }
            }
            else{
                printf("Please enter an integer value after -p.\n");
                return 0;
            }
        } 
    } 

    // Pass thread data to the threads and run.
    runThreads(&t_data);
};