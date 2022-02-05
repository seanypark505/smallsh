// Sean Park
// CS344 Assignment3: smallsh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAXLINE 2048  // Max number of characters in command
#define MAXARGS 512  // Max number of arguments
#define MAXPROCS 200  // Max number of processes

// Global variables
int foreground_mode = 0;  // 1 to set to foreground only mode

struct command
{
    char *cmd;  // main command
    char *argv[MAXARGS];  // Array of 512 arguments with length of 256
    int argc;  // Number of arguments
    char *input;  // Input file
    char *output; // Output file
    bool background;  // Boolean to indicate background execution
};

/*
*  Prompts the user for input and parses the command to store data in a struct
*/
void getInput(struct command *user_cmd)
{
    char input[MAXLINE];  // String array to hold user input
    // Iterators for $$ variable expansion
    int i;

    // Prompt user for input
    printf(": ");
    fflush(stdout);
    fgets(input, MAXLINE, stdin);

    // Remove newline character
    input[strlen(input) - 1] = '\0';

    // If user enters nothing, update struct with empty string
    if (strlen(input) == 0)
    {
        user_cmd->cmd = "";
        user_cmd->argc = 0;
        user_cmd->input = "";
        user_cmd->output = "";
        user_cmd->background = false;
        return;
    }

    // If user enters a comment, update struct command with '#'
    if (input[0] == '#')
    {
        user_cmd->cmd = "#";
        user_cmd->argc = 0;
        user_cmd->input = "";
        user_cmd->output = "";
        user_cmd->background = false;
        return;
    }

    // Parse input into struct
    char *token = strtok(input, " ");
    user_cmd->cmd = calloc(strlen(token) + 1, sizeof(char));
    strcpy(user_cmd->cmd, token);

    token = strtok(NULL, " ");

    // Iterate through tokens and store in struct
    while(token != NULL){
        // Check for background execution
        if (strcmp(token, "&") == 0)
        {
            user_cmd->background = true;
        } else if (strcmp(token, "<") == 0)  // Check for input redirection
        {
            token = strtok(NULL, " ");
            user_cmd->input = calloc(strlen(token) + 1, sizeof(char));
            strcpy(user_cmd->input, token);
        } else if (strcmp(token, ">") == 0)  // Check for output redirection
        {
            token = strtok(NULL, " ");
            user_cmd->output = calloc(strlen(token) + 1, sizeof(char));
            strcpy(user_cmd->output, token);
        } else {
            // Replace every instance of $$ in the token with the pid of the process
            for (i = 0; i < strlen(token); i++)
            {
                char *temp = strstr(token, "$$");

                if (temp)
                {
                    // Replace token with string before $$ concatenated with pid
                    char pid[10];
                    sprintf(pid, "%d", getpid());
                    char *temp2 = calloc(strlen(token) + strlen(pid) + 1, sizeof(char));
                    strncpy(temp2, token, temp - token);
                    strcat(temp2, pid);
                    strcat(temp2, temp + 2);
                    token = temp2;
                    free(temp2);
                }
                free(temp);
            }
            user_cmd->argv[user_cmd->argc] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(user_cmd->argv[user_cmd->argc], token);
            user_cmd->argc++;
        }
        token = strtok(NULL, " ");
    }
    free(token);

}

/*
*  Executes the command stored in the struct other than built-in commands of exit, cd, and status by forking a child process
*/
void executeOtherCommand(struct command *user_cmd)
{
    int spawn_pid;
    int status;
    int fd_in;
    int fd_out;

    // Fork a child process
    spawn_pid = fork();

    switch(spawn_pid)
    {
        case -1:
            perror("fork() failed!");
            exit(1);
            break;
        case 0:
            // If input redirection is specified, open the file and redirect input
            if (strlen(user_cmd->input) > 0)
            {
                fd_in = open(user_cmd->input, O_RDONLY);
                if (fd_in == -1)
                {
                    perror("open");
                    exit(1);
                }
                dup2(fd_in, 0);
                close(fd_in);
            }

            // If output redirection is specified, open the file and redirect output
            if (strlen(user_cmd->output) > 0)
            {
                fd_out = open(user_cmd->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out == -1)
                {
                    perror("open");
                    exit(1);
                }
                dup2(fd_out, 1);
                close(fd_out);
            }

            // Execute the command
            execvp(user_cmd->cmd, user_cmd->argv);
            perror("execvp");
            exit(1);
            break;
        default:
            // If background execution is specified, wait for the child process to finish
            if (user_cmd->background)
            {
                waitpid(spawn_pid, &status, WNOHANG);
            } else {
                waitpid(spawn_pid, &status, 0);
            }
    }
}

/*
* Free the memory allocated to the struct
*/
void freeCommand(struct command *user_cmd)
{
    // If the command is not a comment or empty, free the memory
    if (strcmp(user_cmd->cmd, "#") != 0 && strlen(user_cmd->cmd) != 0)
    {
        free(user_cmd->cmd);
        for (int i = 0; i < user_cmd->argc; i++)
        {
            free(user_cmd->argv[i]);
        }

    }
   
    // If input or output is not null or empty, free the memory
    if (user_cmd->input != NULL && strlen(user_cmd->input) != 0)
    {
        free(user_cmd->input);
    }
    if (user_cmd->output != NULL && strlen(user_cmd->output) != 0)
    {
        free(user_cmd->output);
    }
}


int main(void)
{   
    struct command user_cmd;

    
    // Program name to indicate start of smallsh
    printf("$ smallsh\n");
    fflush(stdout);

    // Loop until user enters exit
    while (true)
    {
        // Get user input
        getInput(&user_cmd);

        // If user enters exit, exit
        if (strcmp(user_cmd.cmd, "exit") == 0)
        {
            freeCommand(&user_cmd);
            exit(0);
        }

        // If user enters cd, change directory
        if (strcmp(user_cmd.cmd, "cd") == 0)
        {
            if (user_cmd.argc == 1)
            {
                chdir(getenv("HOME"));
            } else {
                chdir(user_cmd.argv[1]);
            }
        }

        // Execute other commands



    }
    return 0;
}

