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

// Create struct to hold command
struct command
{
    char *argv[MAXARGS];  // arguments
    int argc;  // number of arguments
    char *input;  // Input file
    char *output; // Output file
    bool background;  // Boolean to indicate background execution
};

int main(void)
{


    // Create struct to hold command
    struct command user_cmd;

    // Prompt user for input
    printf(": ");
    fflush(stdout);
    fgets(user_cmd.argv[0], MAXLINE, stdin);

    // Remove newline character
    user_cmd.argv[0][strcspn(user_cmd.argv[0], "\n")] = 0;

    // If user enters nothing, update struct with empty string
    if (strlen(user_cmd.argv[0]) == 0 || user_cmd.argv[0][0] == '#')
    {
        user_cmd.argv[0] = "";
        return 0;
    }

    // Parse input into struct
    char *token = strtok(user_cmd.argv[0], " ");
    // Parse first token
    user_cmd.argv[user_cmd.argc] = token;
    user_cmd.argc++;
    token = strtok(NULL, " ");

    // Parse remaining tokens
    while (token != NULL)
    {
        user_cmd.argv[user_cmd.argc] = token;
        user_cmd.argc++;
        token = strtok(NULL, " ");
    }

    // Check for background execution
    if (user_cmd.argv[user_cmd.argc - 1][strlen(user_cmd.argv[user_cmd.argc - 1]) - 1] == '&')
    {
        user_cmd.argv[user_cmd.argc - 1][strlen(user_cmd.argv[user_cmd.argc - 1]) - 1] = '\0';
        user_cmd.background = true;
    }

    //
}