#include "shell.h"



int main(void) {

    string command;
    pid_t pid;
    int status;
	int cnt_line = 0;   
    int cross_line_pipe_table[FILE_MAX][2] = {0}; 

    if ((pid = fork()) == -1)
    {
        perror("fork");
    }
    else if (pid == 0)
    {
        setenv("PATH", "bin:.", 1);
        while (1)
        {

            cout << "% ";

            switch(readline(command))
            {
                case -1: // EOF
                    exit(0);
                case 0:
                    continue;
                default:
                    my_shell(command, &cnt_line, cross_line_pipe_table);
                    break;
            }

        }
    }
    else
    {
        // parent.
        wait(&status);
    } 
}
