#include <pthread.h>
#include <stdio.h>

struct thread_safe_int {
    pthread_mutex_t mutex;
    int x ATTRIBUTE((guarded_by(mutex)));
};

int main(void)
{
    struct thread_safe_int tsi = {.mutex = PTHREAD_MUTEX_INITIALIZER, .x = 0};
    pthread_mutex_lock(&tsi.mutex);
    printf("x = %d\n", tsi.x);
    pthread_mutex_unlock(&tsi.mutex);
    pthread_mutex_destroy(&tsi.mutex);
}
