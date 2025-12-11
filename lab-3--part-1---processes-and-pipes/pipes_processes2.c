// pipes_processes3.c
// Implements the pipeline: cat scores | grep <arg> | sort
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

/**
 * Executes the command: cat scores | grep <arg> | sort
 * P1 (Parent): cat scores
 * P2 (Child): grep <arg>
 * P3 (Grandchild): sort
 */

#define READ_END 0
#define WRITE_END 1

int main(int argc, char **argv)
{
    // Check for the required command-line argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <grep_argument>\n", argv[0]);
        return 1;
    }

    // Two pipes are needed for a three-stage pipeline (P1->P2, P2->P3)
    int pipe1[2]; // P1 (cat) -> P2 (grep)
    int pipe2[2]; // P2 (grep) -> P3 (sort)
    
    pid_t pid_p2, pid_p3;
    
    // Commands and arguments
    char *grep_arg = argv[1];
    char *cat_args[] = {"cat", "scores", NULL};
    char *grep_args[] = {"grep", grep_arg, NULL};
    char *sort_args[] = {"sort", NULL};

    // 1. Create Pipe 1 (P1 -> P2)
    if (pipe(pipe1) == -1) {
        perror("pipe1 failed");
        return 1;
    }

    // 2. Fork P1 -> P2
    pid_p2 = fork();
    if (pid_p2 < 0) {
        perror("fork P2 failed");
        return 1;
    }

    // ******************************************************
    // CHILD PROCESS (P2): executes grep <arg> and forks P3 (sort)
    // ******************************************************
    if (pid_p2 == 0) {
        
        // 3. Create Pipe 2 (P2 -> P3)
        if (pipe(pipe2) == -1) {
            perror("pipe2 failed");
            exit(1);
        }

        // 4. Fork P2 -> P3
        pid_p3 = fork();
        if (pid_p3 < 0) {
            perror("fork P3 failed");
            exit(1);
        }
        
        // ******************************************************
        // GRANDCHILD PROCESS (P3): executes sort
        // ******************************************************
        if (pid_p3 == 0) {
            // P3 INPUT: Must read from pipe2[READ_END]
            dup2(pipe2[READ_END], STDIN_FILENO);
            
            // Close all unused pipe ends
            close(pipe1[READ_END]);
            close(pipe1[WRITE_END]);
            close(pipe2[WRITE_END]); // Done duplicating, close write end
            close(pipe2[READ_END]);  // Done duplicating, close original read end

            execvp("sort", sort_args);
            perror("execvp sort failed");
            exit(1); // Exit if execvp fails
        }
        
        // ******************************************************
        // CHILD PROCESS (P2) CONTINUES: executes grep <arg>
        // ******************************************************
        
        // P2 INPUT: Must read from pipe1[READ_END] (from P1/cat)
        dup2(pipe1[READ_END], STDIN_FILENO);
        
        // P2 OUTPUT: Must write to pipe2[WRITE_END] (to P3/sort)
        dup2(pipe2[WRITE_END], STDOUT_FILENO);

        // Close all unused pipe ends
        close(pipe1[WRITE_END]); // Only need to read from pipe1
        close(pipe1[READ_END]);  // Done duplicating
        close(pipe2[READ_END]);  // Only need to write to pipe2
        close(pipe2[WRITE_END]); // Done duplicating

        // Wait for P3 (sort) to complete before P2 terminates
        waitpid(pid_p3, NULL, 0);

        execvp("grep", grep_args);
        perror("execvp grep failed");
        exit(1); // Exit if execvp fails
    }

    // ******************************************************
    // PARENT PROCESS (P1): executes cat scores
    // ******************************************************
    
    // P1 INPUT: STDIN_FILENO (keyboard) - Keep it as is.
    
    // P1 OUTPUT: Must write to pipe1[WRITE_END] (to P2/grep)
    dup2(pipe1[WRITE_END], STDOUT_FILENO);

    // Close all unused pipe ends
    close(pipe1[READ_END]);  
    close(pipe1[WRITE_END]); // Done duplicating

    execvp("cat", cat_args);
    perror("execvp cat failed");
    return 1; // Return if execvp fails
} 