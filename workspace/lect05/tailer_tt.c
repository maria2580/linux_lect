#include <stdio.h>
#include <stdlib.h> // For exit()
#include <unistd.h> // For fork(), pipe(), read(), write()
#include <sys/wait.h> // For wait()
#define _USE_MATH_DEFINES
#include <math.h>

#define N 4 // Number of elements to process

// A simple struct to send results back to the parent
// This helps keep the index and value together.
struct result_t {
    int index;
    double value;
};

void sinx_taylor_parallel(int num_elements, int terms, const double* x, double* result) {
    int pipefd[2]; // pipefd[0] is for reading, pipefd[1] is for writing

    // Create a pipe for IPC. Error check it.
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // --- Create Child Processes ---
    for (int i = 0; i < num_elements; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            // Error handling for fork failure
            perror("fork");
            exit(EXIT_FAILURE);

        } else if (pid == 0) {
            // --- This is the CHILD process ---

            // The child doesn't need to read from the pipe, so close the read end.
            close(pipefd[0]);

            // Perform the Taylor series calculation
            double value = x[i];
            double numer = x[i] * x[i] * x[i];
            double denom = 6.0; // 3!
            int sign = -1;

            for (int j = 1; j <= terms; j++) {
                value += (double)sign * numer / denom;
                numer *= x[i] * x[i]; // Use x[i], the value for this specific child
                denom *= (2.0 * j + 2.0) * (2.0 * j + 3.0);
                sign *= -1;
            }

            // Prepare the result payload
            struct result_t res = { .index = i, .value = value };

            // Write the result struct back to the parent through the pipe.
            write(pipefd[1], &res, sizeof(struct result_t));

            // Close the write end of the pipe and exit.
            close(pipefd[1]);
            exit(EXIT_SUCCESS); // Child's work is done.
        }
    }

    // --- This is the PARENT process ---

    // The parent doesn't need to write, so close the write end.
    close(pipefd[1]);

    // Read the results from all children
    for (int i = 0; i < num_elements; i++) {
        struct result_t res;
        read(pipefd[0], &res, sizeof(struct result_t)); // Read one result struct
        result[res.index] = res.value; // Place the value in the correct spot
    }

    // Close the read end of the pipe.
    close(pipefd[0]);

    // Wait for all child processes to terminate to avoid zombies.
    for (int i = 0; i < num_elements; i++) {
        wait(NULL);
    }
}

int main() {
    // Correctly sized arrays for N elements
    double x[N] = {0.0, M_PI / 6.0, M_PI / 3.0, 0.134};
    double res[N];

    // Calculate using our parallel function
    sinx_taylor_parallel(N, 29, x, res); // Using 5 terms for better accuracy

    // Print the results
    printf("--- Calculation Results ---\n");
    for (int i = 0; i < N; i++) {
        printf("Input x = %.4f\n", x[i]);
        printf("  Taylor series sin(x) = %f\n", res[i]);
        printf("  Math library  sin(x) = %f\n\n", sin(x[i]));
    }

    return 0;
}
