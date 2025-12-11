// C program to demonstrate use of fork() and two-way pipe() 
#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 

#define MAX_SIZE 256 // Define a maximum buffer size

int main() 
{ 
    // Pipe 1 (fd1): Parent (P1) writes to Child (P2)
    int fd1[2];  
    // Pipe 2 (fd2): Child (P2) writes to Parent (P1)
    int fd2[2];  
  
    char fixed_str1[] = "howard.edu"; 
    char fixed_str2[] = "gobison.org"; 
    char input_str[MAX_SIZE]; 
    pid_t p; 
  
    // Create first pipe (P1 -> P2)
    if (pipe(fd1) == -1) 
    { 
        perror("Pipe 1 Failed"); 
        return 1; 
    } 
    // Create second pipe (P2 -> P1)
    if (pipe(fd2) == -1) 
    { 
        perror("Pipe 2 Failed"); 
        return 1; 
    } 
  
    // Get initial input from the user in P1
    printf("P1: Enter the initial string to send to P2: ");
    // Use fgets to safely read string input
    if (fgets(input_str, MAX_SIZE, stdin) == NULL) {
        perror("P1: Error reading input");
        return 1;
    }
    // Remove the newline character added by fgets, if present
    input_str[strcspn(input_str, "\n")] = 0; 

    p = fork(); 
  
    if (p < 0) 
    { 
        perror("fork Failed"); 
        return 1; 
    } 
  
    // Parent process (P1) 
    else if (p > 0) 
    { 
        char final_str[MAX_SIZE];
  
        // 1. Setup P1's pipe ends
        close(fd1[0]);  // P1 won't read from Pipe 1
        close(fd2[1]);  // P1 won't write to Pipe 2
  
        // 2. Write input string to P2 (Pipe 1)
        printf("P1: Sending '%s' to P2...\n", input_str);
        write(fd1[1], input_str, strlen(input_str) + 1); 
        close(fd1[1]); // Done writing to Pipe 1
  
        // 3. Wait for P2 to finish and close its writing end
        wait(NULL); 
  
        // 4. Read the modified string from P2 (Pipe 2)
        printf("P1: Waiting to receive string from P2...\n");
        read(fd2[0], final_str, MAX_SIZE); 
        close(fd2[0]); // Done reading from Pipe 2

        // 5. Concatenate "gobison.org" and print
        // Get current length
        int len = strlen(final_str);
        // Ensure there's space for the new string and null terminator
        if (len + strlen(fixed_str2) < MAX_SIZE) {
            strcat(final_str, fixed_str2);
            printf("P1: Final concatenated string (received from P2 + '%s'): %s\n", 
                   fixed_str2, final_str);
        } else {
            fprintf(stderr, "P1: Buffer overflow prevented.\n");
        }
    } 
  
    // Child process (P2) 
    else
    { 
        char received_str[MAX_SIZE]; 
        char second_input[MAX_SIZE];
      
        // 1. Setup P2's pipe ends
        close(fd1[1]);  // P2 won't write to Pipe 1
        close(fd2[0]);  // P2 won't read from Pipe 2

        // 2. Read string from P1 (Pipe 1)
        read(fd1[0], received_str, MAX_SIZE); 
        close(fd1[0]); // Done reading from Pipe 1
  
        // 3. Concatenate "howard.edu"
        printf("P2: Received '%s' from P1.\n", received_str);
        // Append "howard.edu"
        if (strlen(received_str) + strlen(fixed_str1) < MAX_SIZE) {
            strcat(received_str, fixed_str1);
            printf("P2: Concatenated with '%s': %s\n", fixed_str1, received_str);
        } else {
            fprintf(stderr, "P2: Buffer overflow prevented when adding howard.edu.\n");
        }

        // 4. Prompt for second input and append
        printf("P2: Enter a second string to append: ");
        if (fgets(second_input, MAX_SIZE, stdin) != NULL) {
             // Remove the newline character
            second_input[strcspn(second_input, "\n")] = 0; 
            
            // Append the second input
            if (strlen(received_str) + strlen(second_input) < MAX_SIZE) {
                strcat(received_str, second_input);
                printf("P2: Appended second string. Result: %s\n", received_str);
            } else {
                fprintf(stderr, "P2: Buffer overflow prevented when adding second input.\n");
            }
        } else {
            perror("P2: Error reading second input");
        }

        // 5. Pass the modified string back to P1 (Pipe 2)
        printf("P2: Sending modified string back to P1...\n");
        write(fd2[1], received_str, strlen(received_str) + 1); 
        close(fd2[1]); // Done writing to Pipe 2
  
        exit(0); 
    } 
    return 0; // Should only be reached by the parent process after child exits
}