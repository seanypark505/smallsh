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

# define MAXLINE 2048  // Limit of characters in user input
# define MAXARGS 512  // Limit of args in user input
# define MAXJOBS 200  // Limit of jobs in processes list

// Global variables
int processes[MAXJOBS];
int exit_status;
int childStatus;

// Struct for processes
typedef struct process {
    pid_t pid;
    char *cmd;
    int status;
    bool stopped;
} process;

// Struct for commands
typedef struct command {
    char *args[MAXARGS];
    int argc;
    char *infile;
    char *outfile;
    bool background;
} command;

/*
*  Return user input string
*/
char *read_cmd(char *input) {
    // Prompt user for input
    printf(": ");
    fflush(stdout);

    // Read user input
    fgets(input, MAXLINE, stdin);

    // Remove newline characters
    input[strcspn(input, "\n")] = 0;

    return input;
}

/*
*  Parse the input line into a command struct
*/
command *parse_cmd(char *line) {
    command *cmd = malloc(sizeof(command));

    char *token;
    int i = 0;

    // Set default values
    cmd->infile = NULL;
    cmd->outfile = NULL;
    cmd->background = false;

    // Split the input line into tokens
    token = strtok(line, " ");
    while (token != NULL) {
        // Check for background, if found and it is the last token, set background to true
        if (strcmp(token, "&") == 0 && i == strlen(line) - 1) {
            cmd->background = true;
            token = strtok(NULL, " ");
        } else if (strcmp(token, ">") == 0) {
            // Set the output file
            cmd->outfile = strtok(NULL, " ");
            token = strtok(NULL, " ");
        } else if (strcmp(token, "<") == 0) {
            // Set the input file
            cmd->infile = strtok(NULL, " ");
            token = strtok(NULL, " ");
        } else {
            // Add the token to the args array
            cmd->args[i] = token;
            i++;
        }
        token = strtok(NULL, " ");
    }
    cmd->argc = i;
    return cmd;
}

/*
*  Execute the command arguments
*/
void execute_cmd(command *cmd) {
    int fd[2];  // 0 is read, 1 is write
    pid_t spawnPid;

    // Check if the command is a built-in command
    if (strcmp(cmd->args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(cmd->args[0], "cd") == 0) {
        // Check if the user is changing to the home directory
        if (cmd->argc == 1) {
            chdir(getenv("HOME"));
        } else {
            // Change to the directory specified
            chdir(cmd->args[1]);
        }
    } else if (strcmp(cmd->args[0], "status") ==0) {
        // Print the exit status or terminating signal of the last foreground process
        if (WIFEXITED(childStatus)) {
            printf("exit value %d\n", WEXITSTATUS(childStatus));
        } else if (WIFSIGNALED(childStatus)) {
            printf("terminated by signal %d\n", WTERMSIG(childStatus));
        }
    } else {
        // Fork a child process
        if ((spawnPid = fork()) == 0) {
            // Check if the command has an input file
            if (cmd->infile != NULL) {
                // Open the file for reading
                if ((fd[0] = open(cmd->infile, O_RDONLY)) == -1) {
                    perror("open");
                    exit(1);
                }
                // Close stdin
                close(0);
                // Connect stdin to the input file
                dup2(fd[0], 0);
                // Close the input file
                close(fd[0]);
            }
            // Check if the command has an output file
            if (cmd->outfile != NULL) {
                // Open the file for writing
                if ((fd[1] = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                    perror("open");
                    exit(1);
                }
                // Close stdout
                close(1);
                // Connect stdout to the output file
                dup2(fd[1], 1);
                // Close the output file
                close(fd[1]);
            }
            // Execute the command
            execvp(cmd->args[0], cmd->args);
            // If the command is not found, print the error
            perror(cmd->args[0]);
            exit(1);
        } else if (spawnPid < 0) {
            perror("fork");
            exit(1);
        } else {
            // Check if the command is a background process
            if (cmd->background) {
                // Add the child process to the processes list
                processes[childStatus] = spawnPid;
            } else {
                // Wait for the child process to finish
                waitpid(spawnPid, &childStatus, 0);
            }
        }
    }
}

int main(void)
{
    printf("$ smallsh\n");
    fflush(stdout);

    // Loop for user input
    while (true) {
        // Initialize string for user input
        char *input = malloc(sizeof(char) * MAXLINE);

        // Initialize the command struct
        command *cmd = malloc(sizeof(command));

        // Read user input
        read_cmd(input);

        // If user input is empty or a comment, continue
        if (input[0] == '\0' || input[0] == '#') {
            continue;
        }

        // Parse the input line into a command struct
        cmd = parse_cmd(input);

        // Execute user input
        execute_cmd(cmd);

        free(cmd);
        free(input);
    }
    return 0;
}