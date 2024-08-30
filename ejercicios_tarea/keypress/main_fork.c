#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, const char **argv)
{
    while (1)
    {
        puts("Press any key:");
        int keycode = getchar();

        if (keycode == '\n' || keycode == EOF)
        {
            continue; // Ignore Enter or EOF and continue the loop
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            // Child process
            printf("I am the child with pid %d from parent %d - fork pid %d\n", getpid(), getppid(), pid);
            printf("Key code: %d\n", keycode);
            printf("Key: %c\n", keycode);
            put("child end");
        }
        else if (pid > 0)
        {
            // Parent process
            printf("I am the parent with pid %d - child pid %d\n", getpid(), pid);

            // The waitpid() system call suspends execution of the calling process
            // until a child specified by pid argument has changed state.
            // By default, waitpid() waits only for terminated children,
            // but this behavior is modifiable via the options argument, as described below.
            int exitStatus;

            printf("waiting for child to finish...\n");
            // Wait for the child process to finish
            waitpid(pid, &exitStatus, 0);
            printf("Retrieved pid: %d, parent end.\n", pid);
        }
        else
        {
            // Error creating the process
            perror("Error calling fork()");
            exit(EXIT_FAILURE);
        }

        // Clear the input buffer
        while (getchar() != '\n')
            ;
    }
}