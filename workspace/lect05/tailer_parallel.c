#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define N 4
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

static double sin_taylor_at(double x, int terms) {
    double value = x;                  // 첫 항
    double numer = x * x * x;          // x^3
    double denom = 6.0;                // 3!
    int sign = -1;
    for (int j = 1; j <= terms; ++j) {
        value += sign * (numer / denom);
        numer *= x * x;                // x^(2k+1)
        denom *= (2.0 * j + 2.0) * (2.0 * j + 3.0);
        sign *= -1;
    }
    return value;
}
void sinx_taylor(int num_elements, int terms, double* x, double* result)
{
	int pipes[N][2];
    pid_t pids[N];
	int myidx;
	
	for (int i = 0; i < num_elements; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
	}
	for (myidx = 0; myidx < num_elements; ++myidx) {
		pids[myidx] = fork();
		

		if (pids[myidx] == 0) {
			printf("Forked process %d for index %d, PPID: %d, PID: %d\n", pids[myidx], myidx, getppid(), getpid());
			break;
		}
	}

	switch(pids[myidx]){
		case -1:
            perror("fork");
            exit(1);
			break;
		case 0:
			// Child: 계산하고 자신의 파이프에 결과 쓰기
			double value = sin_taylor_at(x[myidx], terms);
			ssize_t n = write(pipes[myidx][1], &value, sizeof(value));
			if (n != (ssize_t)sizeof(value)) {
				perror("write");
			}
			close(pipes[myidx][1]);
			_exit(0);
			break;
		default:
			// 부모는 모든 pid가 종료됐는지 순차적으로 확인
			for (int i = 0; i < num_elements; ++i) {
				waitpid(pids[i], NULL, 0);
			}
			break;
	}
	
	// Parent: 각 파이프에서 결과 읽기
	for (int i = 0; i < num_elements; ++i) {
		double value;
		read(pipes[i][0], &value, sizeof(double));
		result[i] = value;
		close(pipes[i][0]); // Close read end
	}
	
	return;
}



int main()
{
	double x[N] = {0, M_PI/6., M_PI/3., 0.134};
	double res[N];
	sinx_taylor(N, 3, x, res);
	for(int i=0; i<N; i++) {
		printf("sin(%.2f) by Taylor series = %f\n", x[i], res[i]);
		printf("sin(%.2f) = %f\n", x[i], sin(x[i]));
	}
	return 0;
}
