#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h> // pthread_ *

void* threadFunction(void* arg)
{
    sleep(10);
    printf("bye-bye from thread\n");
    long i = 63;
    void* p = (void*)i;
    return p;
}

int main()
{
    pthread_t thread;
    pthread_attr_t* threadAttributes = NULL; 
    void* args = NULL;

    if(pthread_create(&thread, threadAttributes, threadFunction, args) != 0)
    {
        printf("Unable to launch thread\n");
        exit(-1);
    }
    printf("thread ID = %ld\n", pthread_self());

    void* retval;
    pthread_join(thread, &retval);  // makes the initial thread wait for the called thread to terminate.
    long J = (long)retval;
    printf("thread returned %ld    %p\n", J, retval);
    sleep(10);

    return 0;
}