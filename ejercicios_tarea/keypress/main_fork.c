#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>

// Función para leer un keypress
int get_keypress()
{
    struct termios oldt, newt;
    int ch;
    // Save the current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    // Modify the terminal settings to disable canonical mode and echo
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

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Proceso hijo
        int keycode = get_keypress();
        printf("Key code: %d\n", keycode);
    }
    else if (pid > 0)
    {
        // Proceso padre
        wait(NULL); // Espera a que el proceso hijo termine
    }
    else
    {
        // Error al crear el proceso
        perror("fork");
        return 1;
    }

    return 0;
}