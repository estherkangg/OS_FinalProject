#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define TRUE 1

int His = -1;
char Path[200];
char PWD[200];
char Help[200];
char ShellName[200];
char *History[100];

void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void executeCommand(char *args[], char *inputFile, char *outputFile, int background) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        // Child process

        // Input redirection
        if (inputFile != NULL) {
            freopen(inputFile, "r", stdin);
        }

        // Output redirection
        if (outputFile != NULL) {
            freopen(outputFile, "w", stdout);
            freopen(outputFile, "w", stderr);
        }

        // Execute command
        if (execvp(args[0], args) == -1) {
            printError();
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Fork error
        printError();
    } else {
        // Parent process
        if (!background) {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}

void changeDirectory(char *args[]) {
    if (args[1] == NULL || args[2] != NULL) {
        printError();
        return;
    }

    if (chdir(args[1]) != 0) {
        printError();
    }
}

void setPath(char *args[], char *path[]) {
    int i = 1;
    while (args[i] != NULL) {
        path[i - 1] = args[i];
        i++;
    }
    path[i - 1] = NULL;
}

void createFile(char *filename) {
    int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1) {
        printError();
    } else {
        close(fd);
    }
}

void removeFile(char *filename) {
    if (remove(filename) != 0) {
        printError();
    }
}

void createDirectory(char *dirname) {
    if (mkdir(dirname, 0755) != 0) {
        printError();
    }
}

void removeDirectory(char *dirname) {
    if (rmdir(dirname) != 0) {
        printError();
    }
}

void copyFile(char *source, char *destination) {
    FILE *sourceFile, *destFile;
    char ch;

    sourceFile = fopen(source, "r");
    destFile = fopen(destination, "w");

    if (sourceFile == NULL || destFile == NULL) {
        printError();
        return;
    }

    while ((ch = fgetc(sourceFile)) != EOF) {
        fputc(ch, destFile);
    }

    fclose(sourceFile);
    fclose(destFile);
}

void copyDirectory(char *source, char *destination) {
    // Implement directory copy logic here (not provided in this example)
    printError();
}

void printWelcomeMessage() {
    printf("=============================\n");
    printf("    WELCOME TO ESTHER SHELL\n");
    printf("=============================\n\n");
}

void displayHelp() {
    printf("=== Esther Shell Help ===\n");
    printf("Available Commands:\n");
    printf("  cd [directory]      - Change current working directory\n");
    printf("  exit                - Exit the shell\n");
    printf("  path [directory...] - Set the search path for executables\n");
    printf("  ls [-all | -tree]   - List files and directories\n");
    printf("  createfile [file]   - Create a new file\n");
    printf("  removefile [file]   - Remove a file\n");
    printf("  createdir [dir]     - Create a new directory\n");
    printf("  removedir [dir]     - Remove a directory\n");
    printf("  copyfile [src] [dest] - Copy a file to a destination\n");
    printf("  copydir [src] [dest]  - Copy a directory to a destination\n");
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    char *path[MAX_ARG_SIZE];
    char *inputFile = NULL;
    char *outputFile = NULL;
    int background = 0;

    // Set default path
    path[0] = "/bin";
    path[1] = NULL;

    printWelcomeMessage();

    while (1) {
        printf("es> ");

        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            exit(0); // Exit gracefully on EOF
        }

        // Remove trailing newline character
        input[strcspn(input, "\n")] = '\0';

        // Tokenize the input
        char *token = strtok(input, " ");
        int i = 0;

        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            if (strcmp(token, "&") == 0) {
                background = 1;
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL && outputFile == NULL) {
                    outputFile = token;
                } else {
                    printError();
                    break;
                }
            } else if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL && inputFile == NULL) {
                    inputFile = token;
                } else {
                    printError();
                    break;
                }
            } else {
                args[i++] = token;
            }

            token = strtok(NULL, " ");
        }

        args[i] = NULL;

        if (i > 0) {
            if (strcmp(args[0], "exit") == 0) {
                if (args[1] == NULL || (args[1] != NULL && strcmp(args[1], "&") == 0)) {
                    exit(0);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "help") == 0) {
                if (args[1] == NULL) {
                    displayHelp();
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "cd") == 0) {
                changeDirectory(args);
            } else if (strcmp(args[0], "path") == 0) {
                setPath(args, path);
            } else if (strcmp(args[0], "ls") == 0) {
                // Handle ls commands
                if (args[1] != NULL && strcmp(args[1], "-all") == 0) {
                    args[0] = "/bin/ls";
                    executeCommand(args, inputFile, outputFile, background);
                } else if (args[1] != NULL && strcmp(args[1], "-tree") == 0) {
                    args[0] = "/bin/ls";
                    args[1] = "-R";
                    args[2] = NULL;
                    executeCommand(args, inputFile, outputFile, background);
                } else {
                    // Execute command
                    int isBuiltIn = 0;
                    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "cd") == 0 || strcmp(args[0], "path") == 0) {
                        isBuiltIn = 1;
                    }

                    if (!isBuiltIn) {
                        // Construct the full path to the executable
                        char *executable = args[0];
                        int j = 0;
                        while (path[j] != NULL) {
                            char *fullPath = malloc(strlen(path[j]) + strlen(executable) + 2);
                            strcpy(fullPath, path[j]);
                            strcat(fullPath, "/");
                            strcat(fullPath, executable);

                            // Check if the executable exists and is executable
                            if (access(fullPath, X_OK) == 0) {
                                args[0] = fullPath;
                                executeCommand(args, inputFile, outputFile, background);
                                break;
                            }

                            free(fullPath);
                            j++;
                        }

                        if (path[j] == NULL) {
                            printError();
                        }
                    }
                }
            } else if (strcmp(args[0], "createfile") == 0) {
                if (args[1] != NULL && args[2] == NULL) {
                    createFile(args[1]);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "removefile") == 0) {
                if (args[1] != NULL && args[2] == NULL) {
                    removeFile(args[1]);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "createdir") == 0) {
                if (args[1] != NULL && args[2] == NULL) {
                    createDirectory(args[1]);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "removedir") == 0) {
                if (args[1] != NULL && args[2] == NULL) {
                    removeDirectory(args[1]);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "copyfile") == 0) {
                if (args[1] != NULL && args[2] != NULL && args[3] == NULL) {
                    copyFile(args[1], args[2]);
                } else {
                    printError();
                }
            } else if (strcmp(args[0], "copydir") == 0) {
                if (args[1] != NULL && args[2] != NULL && args[3] == NULL) {
                    // Implement directory copy logic here
                    printError();
                } else {
                    printError();
                }
            } else {
                printError();
            }
        }

        // Reset variables for the next iteration
        memset(input, 0, sizeof(input));
        memset(args, 0, sizeof(args));
        inputFile = NULL;
        outputFile = NULL;
        background = 0;
    }

    return 0;
}
