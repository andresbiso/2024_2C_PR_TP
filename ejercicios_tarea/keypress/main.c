#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

typedef void *(*PCALLBACK)(void *);

// Función que se ejecutará en el hilo
void *print_keycode(void *arg)
{
    int keycode = *(int *)arg;
    char key = keycode;
    printf("Key code: %d\n", keycode);
    printf("Key: %c\n", key);
    return NULL;
}

int main()
{
    pthread_t thread;
    PCALLBACK callback = print_keycode;
    while (1)
    {
        puts("Press any key:");
        int keycode = getchar();
        pthread_create(&thread, NULL, callback, &keycode);
        // wait for thread
        pthread_join(thread, NULL);
    }
    printf("main thread finished... \n");
    return 0;
}