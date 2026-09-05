/*
Name: Rylan Pietras
NetID: pietras1
Description: This is a shell program that is able to execute other programs as well as the exit, cd, and path built in commands.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

// Standard error message and rush message
char error_message[30] = "An error has occurred\n";
char *rush = "rush> ";

// Function for determining whitespace
unsigned int isWhitespace(char c) {
    if(c == ' ' || c == '\n' || c == '\t' || c == '\f' || c == '\v' || c == '\r') {
        return 1;
    } else {
        return 0;
    }
}

// Function for counting the arguments in a string
unsigned int countArgs(char *command) {
    if(!command) {
        fflush(NULL);
        write(STDERR_FILENO, error_message, strlen(error_message));
        fflush(NULL);
        exit(1);
    }
    unsigned int i = 0;
    unsigned int argFlag = 0;
    unsigned int args = 0;
    while(command[i] != '\0') {
        if((argFlag == 1) && (isWhitespace(command[i]))) {
            argFlag = 0;
        } else if((argFlag == 0) && (!isWhitespace(command[i]))) {
            argFlag = 1;
            args++;
        }
        i++;
    }
    return args;
}

// Function for parsing a string (the command) into an array of array pointers, with each array pointer
// being a string of an element in the original string (an argument)
char **parse(char *command, unsigned int numArgs) {
    if(!command || numArgs < 0) {
        fflush(NULL);
        write(STDERR_FILENO, error_message, strlen(error_message));
        fflush(NULL);
        exit(1);
    }

    char **pCommand = (char **)malloc(sizeof(char *) * (numArgs + 1));
    pCommand[numArgs] = NULL;
    unsigned int parsedI = 0;

    unsigned int commandI = 0;
    unsigned int argI = 0;
    unsigned int copyFlag = 0;

    while(command[commandI] != '\0') {
        if(isWhitespace(command[commandI])) {
            if(copyFlag == 1) {
                copyFlag = 0;
                argI = 0;
                parsedI++;
            }
            commandI++;
        } else {
            if(copyFlag == 0) {
                copyFlag = 1;
                unsigned int j = 0;
                while(!isWhitespace(command[commandI + j])) {
                    j++;
                }
                pCommand[parsedI] = (char *)malloc(sizeof(char) * (j + 1));
                pCommand[parsedI][j] = '\0';
                pCommand[parsedI][argI] = command[commandI];
                commandI++;
                argI++;
            } else {
                pCommand[parsedI][argI] = command[commandI];
                commandI++;
                argI++;
            }
        }
    }
    return pCommand;
}

// Frees memory from the parsed command as well as it itself
void freeParsed(char **pCommand, unsigned int numArgs) {
    for(int i = 0; i < numArgs; i++) {
        free(pCommand[i]);
    }
    free(pCommand);
}

// Replaces the first element of the parsed command with a string specified by newArg
void replaceFirstArg(char **pCommand, char *newArg) {
    if((!pCommand) || (!newArg)) {
        return;
    }
    free(pCommand[0]);
    pCommand[0] = (char *)malloc(sizeof(char) * (strlen(newArg) + 1));
    strcpy(pCommand[0], newArg);
}

// Cuts off the redirect path from the parsed command, as well as the '>'
char **cutOffDirect(char **pCommand, unsigned int numArgs) {
    if(numArgs >= 3) {
        char **cutPCommand = (char **)malloc(sizeof(char *) * (numArgs - 2));
        for(unsigned int i = 0; i < (numArgs - 2); i++) {
            cutPCommand[i] = pCommand[i];
        }
        cutPCommand[numArgs - 1] = NULL;
        return cutPCommand;
    } else {
        return pCommand;
    }
}

int main(int argc, char *argv[]) {
    // Ensures that rush is executed with no arguments
    if(argc > 1) {
        fflush(NULL);
        write(STDERR_FILENO, error_message, strlen(error_message));
        fflush(NULL);
        exit(1);
    }

    // Preparers the memory space (array of strings) for storing all paths
    unsigned int numPaths = 1;
    char **paths = parse("/bin", numPaths);

    // Loop for continuously executing until user inputs "exit"
    while(1) {
        // Writes "rush> " to terminal
        fflush(NULL);
        write(STDOUT_FILENO, rush, strlen(rush));
        fflush(NULL);

        // Gets user input (command)
        char *command = NULL;
        size_t commandLen = 0;
        getline(&command, &commandLen, stdin);

        // Counts the number of elements within the command (args) and parses the command string
        unsigned int numArgs = countArgs(command);
        char **pCommand = parse(command, numArgs);

        if((pCommand) && (numArgs > 0) && (strcmp(pCommand[0], "exit") == 0)) { // Handles the exit built-in command
            if(numArgs == 1) {
                freeParsed(pCommand, numArgs);
                freeParsed(paths, numPaths);
                exit(0);
            } else {
                fflush(NULL);
                write(STDERR_FILENO, error_message, strlen(error_message));
                fflush(NULL);
            }
        } else if ((pCommand) && (numArgs > 0) && (strcmp(pCommand[0], "path") == 0)) { // Handles the path built-in command
            freeParsed(paths, numPaths);
            paths = (char **)malloc(sizeof(char *) * (numArgs));
            numPaths = numArgs - 1;
            paths[numPaths] = NULL;
            for(unsigned int i = 1; i < numArgs; i++) {
                paths[i - 1] = (char *)malloc(sizeof(char) * (strlen(pCommand[i]) + 1));
                strcpy(paths[i - 1], pCommand[i]);
            }
        } else if ((pCommand) && (numArgs > 0) && (strcmp(pCommand[0], "cd") == 0)) { // Handles cd built-in command
            if(numArgs == 2) {
                if(chdir(pCommand[1]) != 0) {
                    fflush(NULL);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    fflush(NULL);
                }
            } else {
                fflush(NULL);
                write(STDERR_FILENO, error_message, strlen(error_message));
                fflush(NULL);
            }
        } else if ((pCommand) && (numArgs > 0)) { // Executes a file given the paths specified
            // Determines if output redirection is present and valid
            int redirFlag = 0;
            unsigned int redirSymI;
            for(redirSymI = 0; redirSymI < numArgs; redirSymI++) {
                if(strcmp(pCommand[redirSymI], ">") == 0) {
                    redirFlag = 1;
                    if((numArgs - redirSymI) != 2) {
                        redirFlag = -1;
                        fflush(NULL);
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        fflush(NULL);
                    }
                    break;
                }
            }

            if(redirFlag != -1) {
                // For loop checks every path for the file to execute until one is found or the paths are exhausted
                pid_t pID = -1;
                unsigned int exFlag = 0;
                for(unsigned int i = 0; i < numPaths; i++) {
                    char buffer[strlen(paths[i]) + 1 + strlen(pCommand[0]) + 1];
                    strcpy(buffer, "");
                    strcat(buffer, paths[i]);
                    strcat(buffer, "/");
                    strcat(buffer, pCommand[0]);

                    char *firstArg = (char *)malloc(sizeof(char) * (strlen(pCommand[0]) + 1));
                    strcpy(firstArg, pCommand[0]);
                    replaceFirstArg(pCommand, buffer);

                    // If the file is found in the path, execute
                    if(access(buffer, F_OK) == 0) {
                        exFlag = 1;
                        pID = fork();
                        if(pID < 0) { // For a failed fork
                            replaceFirstArg(pCommand, firstArg);
                            free(firstArg);
                            fflush(NULL);
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            fflush(NULL); 
                            break;
                        } else if(pID == 0) { // For the child process
                            if(redirFlag == 1) { // If output redirection
                                int stdout_fileno = dup(STDOUT_FILENO);
                                close(STDOUT_FILENO);
                                if(open(pCommand[redirSymI + 1], O_CREAT | O_RDWR | O_TRUNC, 0644) == -1) {
                                    replaceFirstArg(pCommand, firstArg);
                                    free(firstArg);
                                    dup2(stdout_fileno, STDOUT_FILENO);
                                    fflush(NULL);
                                    write(STDERR_FILENO, error_message, strlen(error_message)); // NOT
                                    fflush(NULL);
                                    break;
                                } else {
                                    char **cutPCommand = cutOffDirect(pCommand, numArgs);
                                    execv(buffer, cutPCommand);
                                    free(cutPCommand);
                                    replaceFirstArg(pCommand, firstArg);
                                    free(firstArg);
                                    fflush(NULL);
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    fflush(NULL); 
                                    break;
                                }
                            } else { // If no output redirection
                                execv(buffer, pCommand);
                                replaceFirstArg(pCommand, firstArg);
                                free(firstArg);
                                fflush(NULL);
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                fflush(NULL); 
                                break;
                            }
                        } else { // For the parent process
                            wait(NULL);
                            replaceFirstArg(pCommand, firstArg);
                            free(firstArg);
                            break;
                        }
                    }
                    replaceFirstArg(pCommand, firstArg);
                    free(firstArg);
                }
                if(exFlag == 0) {
                    fflush(NULL);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    fflush(NULL); 
                }
            }
        }
    }
    return 0;
}
