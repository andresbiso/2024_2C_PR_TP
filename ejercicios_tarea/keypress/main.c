#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>

typedef void *(*PCALLBACK)(void *);

// Lets you read a keypress without pressing Return
int get_keypress()
{
    struct termios oldt, newt;
    int ch;
    // Save the current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    // Modify the terminal settings to disable canonical mode and echo
    // ICANON: Deactivates canonical mode, lets you read the input character by character instead of line by line.
    // ECHO: Deactivates echo, pressed character won't show up on the screen.
    newt.c_lflag &= ~(ICANON | ECHO);
    // Apply the new terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    // Read a character from STDIN
    ch = getchar();
    // Restore the original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    // Return the key code
    return ch;
}

// Función que se ejecutará en el hilo
void *print_keycode(void *arg)
{
    printf("Press any key: ");
    int keycode = get_keypress();
    printf("Key code: %d\n", keycode);
    return NULL;
}

int main()
{
    pthread_t thread;
    PCALLBACK callback = print_keycode;
    pthread_create(&thread, NULL, callback, NULL);
    // wait for thread
    pthread_join(thread, NULL);
    printf("main thread finished... \n");
    return 0;
}