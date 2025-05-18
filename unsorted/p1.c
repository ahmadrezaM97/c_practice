#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // For sleep()

// Structure to pass multiple arguments to thread
typedef struct {
    int value;
    int id;
} thread_args_t;

// Function to be run by thread
void *sample_thread(void *arg)
{
    thread_args_t *args = (thread_args_t *)arg;
    int n = args->value;
    int id = args->id;

    printf("Thread %d is running\n", id);
    // Simulate work
    for (int i = 0; i < 10; ++i)
    {
        printf("Thread %d: %d\n", id, i);
        sleep(1);
    }
    free(args); // Free the memory allocated for arguments
    return NULL;
}

int main()
{
    pthread_t threads[3];
    for (int i = 0; i < 3; i++)
    {
        // Allocate memory for thread arguments
        thread_args_t *args = malloc(sizeof(thread_args_t));
        if (args == NULL) {
            perror("Failed to allocate memory");
            return 1;
        }
        args->value = 10;
        args->id = i;
        
        // Create thread
        if (pthread_create(&threads[i], NULL, sample_thread, args) != 0)
        {
            perror("Failed to create thread");
            free(args);
            return 1;
        }
    }
    // Cancel thread 0 (first thread)
    if (pthread_cancel(threads[0]) != 0) {
        perror("Failed to cancel thread");
        return 1;
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // Note: Thread cancellation after join doesn't make sense
    // since the threads have already completed
    
    pthread_exit(NULL);
    return 0;
}