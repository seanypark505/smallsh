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
// int processes[MAXJOBS];  // List of processes
int exit_status;  // Exit status of last process
int childStatus;  // Status of last child process
int fg_only_mode;  // Flag for foreground mode (1 = on, 0 = off)
int bg_jobs[MAXJOBS];  // List of background jobs
int bg_jobs_count;  // Number of background jobs

// Struct for commands
typedef struct command {
    char *args[MAXARGS];
    int argc;
    char *infile;
    char *outfile;
    bool background;
    bool infile_redirect;
    bool outfile_redirect;
} command;

// SIGINT sigaction handler
void handle_SIGINT(int signo)
{
    char message[30];
    sprintf(message,  "Terminated by signal %d\n", signo);
    // Concatenate message and signal
    write(STDOUT_FILENO, message, 30);
    fflush(stdout);
}

// SIGTSTP sigaction handler
void handle_SIGTSTP(int signo)
{
    char *fg_on = "Entering foreground-only mode (& is now ignored)\n";
    char *fg_off = "Exiting foreground-only mode\n";

    // If foreground mode is on, turn off and print message
    if (fg_only_mode == 1) 
    {
        fg_only_mode = 0;
        write(STDOUT_FILENO, fg_off, 30);
        fflush(stdout);
    } else
    {
        // If foreground mode is off, turn on and print message
        fg_only_mode = 1;
        write(STDOUT_FILENO, fg_on, 50);
        fflush(stdout);
    }
}



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
    cmd->infile_redirect = false;
    cmd->outfile_redirect = false;

    // Split the input line into tokens
    token = strtok(line, " ");
    while (token != NULL) {
        // Check for background, if found and it is the last token, set background to true
        if (strcmp(token, "&") == 0 && i == strlen(line) - 1) {
            cmd->background = true;
            token = strtok(NULL, " ");
        } else if (strcmp(token, ">") == 0) {
            cmd->outfile_redirect = true;
            // Change token to the next token and set outfile to the token
            token = strtok(NULL, " ");
            // Set the output file
            cmd->outfile = token;
            // Set the next token to NULL
            token = strtok(NULL, " ");
            continue;
        } else if (strcmp(token, "<") == 0) {
            cmd->infile_redirect = true;
            // Change token to the next token and set infile to the token
            token = strtok(NULL, " ");
            // Set the input file
            cmd->infile = token;
            // Set the next token to NULL
            token = strtok(NULL, " ");
            continue;
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

void expand_variable(struct command *cmd) {
    // Check if any arguments has a "$$" in it
    for (int i = 0; i < cmd->argc; i++) {
        // Change all instances of "$$" with the pid of the current process until no more instances are found
        while (strstr(cmd->args[i], "$$") != NULL) {
            // Get the pid of the current process
            char pid[10];
            sprintf(pid, "%d", getpid());

            // Replace all instances of "$$" with the pid
            char *temp = cmd->args[i];  // Store the current argument
            char *temp2 = strstr(cmd->args[i], "$$");  // Store the location of the "$$"
            char *temp3 = malloc(sizeof(char) * (strlen(temp) - 2));  // Store the new argument
            strncpy(temp3, temp, temp2 - temp);  // Copy the first part of the argument
            strcat(temp3, pid);  // Add the pid to the new argument
            strcat(temp3, temp2 + 2);  // Add the rest of the argument  
            strcpy(cmd->args[i], temp3);  // Copy the new argument into the old argument
            free(temp3);  // Free the new argument
        }
    }
}

/*
*  Execute the command arguments
*/
void execute_cmd(command *cmd) {
    int fd[2];  // 0 is read, 1 is write
    pid_t spawnPid = -5;  // PID of child process

    // Check all arguments for "$$" substring and replace with the PID of the current process
    expand_variable(cmd);

    // Check for foreground only mode
    if (fg_only_mode == 1) {
        // If foreground only mode is on, set the process to be run in the foreground
        cmd->background = false;
    }

    if (cmd->background == false) {
        // SIGINT terminates the foreground process
        signal(SIGINT, handle_SIGINT);

        // SIGTSTP is ignored
        signal(SIGTSTP, SIG_IGN);
    }

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
            fflush(stdout);
        } else if (WIFSIGNALED(childStatus)) {
            printf("terminated by signal %d\n", WTERMSIG(childStatus));
            fflush(stdout);
        }
    } else {
        // Fork a child process
        spawnPid = fork();

        if (spawnPid == -1)
        {
            perror("fork() failed!");
            fflush(stdout);
            exit(1);
        } else if (spawnPid == 0)
        {
            // Child process
            // Check if background mode is on
            if (cmd->background == false)
            {
                // Check if the command has an input file
                if (cmd->infile != NULL) {
                    // Open the input file
                    fd[0] = open(cmd->infile, O_RDONLY);
                    // Check if the file was opened successfully
                    if (fd[0] == -1) {
                        printf("Error: cannot open %s for input\n", cmd->infile);
                        fflush(stdout);
                        exit(1);
                    }
                    dup2(fd[0], 0);
                    close(fd[0]);
                }
                // Check if the command has an output file
                if (cmd->outfile != NULL) {
                    // Open the output file
                    fd[1] = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    // Check if the file was opened successfully
                    if (fd[1] == -1) {
                        printf("Error: cannot open %s for output\n", cmd->outfile);
                        fflush(stdout);
                        exit(1);
                    }
                    dup2(fd[1], 1);
                    close(fd[1]);
                }
                // Execute the command
                execvp(cmd->args[0], cmd->args);
                perror(cmd->args[0]);
                fflush(stdout);
                exit(1);
            } else if (cmd->background == true) {  // If command contained '&' at the end, set the child process to run in the background
                // Check for inputfile redirection
                if (cmd->infile_redirect == true) {
                    if (cmd->infile != NULL) {
                        // Open the input file
                        fd[0] = open(cmd->infile, O_RDONLY);
                        // Check if the file was opened successfully
                        if (fd[0] == -1) {
                            printf("Error: cannot open %s for input\n", cmd->infile);
                            fflush(stdout);
                            exit(1);
                        }
                        dup2(fd[0], 0);
                        close(fd[0]);
                    } else {
                        // Use "/dev/null" as the input file
                        fd[0] = open("/dev/null", O_RDONLY);
                        // Check if the file was opened successfully
                        if (fd[0] == -1) {
                            printf("Error: cannot open /dev/null for input\n");
                            fflush(stdout);
                            exit(1);
                        }
                    }
                }

                // Check for outputfile redirection
                if (cmd->outfile_redirect == true) {
                    if (cmd->outfile != NULL) {
                        // Open the output file
                        fd[1] = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        // Check if the file was opened successfully
                        if (fd[1] == -1) {
                            printf("Error: cannot open %s for output\n", cmd->outfile);
                            fflush(stdout);
                            exit(1);
                        }
                        dup2(fd[1], 1);
                        close(fd[1]);
                    } else {
                        // Use "/dev/null" as the output file
                        fd[1] = open("/dev/null", O_WRONLY);
                        // Check if the file was opened successfully
                        if (fd[1] == -1) {
                            printf("Error: cannot open /dev/null for output\n");
                            fflush(stdout);
                            exit(1);
                        }
                    }
                }
                // Execute the command
                execvp(cmd->args[0], cmd->args);
                perror(cmd->args[0]);
                fflush(stdout);
                exit(1);
            }
        } else
        {
            // Parent process
            // Check if background mode is on
            if (cmd->background == false)
            {
                // Wait for the child process to finish
                waitpid(spawnPid, &childStatus, 0);
            } else if (cmd->background == true) {
                // Continue executing the shell
                waitpid(spawnPid, &childStatus, WNOHANG);
                // Print the PID of the child process
                printf("Background command with pid %d has begun\n", spawnPid);
                fflush(stdout);
            }
        }
        // Print the pid and status of any background processes that have finished
        while (spawnPid = waitpid(-1, &childStatus, WNOHANG) > 0) {
            printf("Background pid %d is done: ", spawnPid);
            if (WIFEXITED(childStatus)) {
                printf("exit value %d\n", WEXITSTATUS(childStatus));
                fflush(stdout);
            } else if (WIFSIGNALED(childStatus)) {
                printf("terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);
            }
        }
    }
}

/*
*  Adds the background process to the list of background processes
*/
void add_background_process(pid_t pid) {
    // Add the child process to bg_jobs
    bg_jobs[bg_jobs_count] = pid;
    bg_jobs_count++;
}

int main(void)
{
    // Initialize the count of background jobs
    bg_jobs_count = 0;
    // Initialize signal structs
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};
    // SIGINT
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;

    // SIGTSTP
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;

    // Register the signal handlers
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
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
            free(input);
            free(cmd);
            continue;
        } else {
            // Parse the input line into a command struct
            cmd = parse_cmd(input);

            // Execute user input
            execute_cmd(cmd);
        }
        // Free the command struct and the input string
        free(cmd);
        free(input);
    }
    return 0;
}