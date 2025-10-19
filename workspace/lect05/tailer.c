#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define N 4

void sinx_taylor(int num_elements, int terms, double* x, double* result)
{
	for(int i=0; i<num_elements; i++) {
		double value = x[i];
		double numer = x[i] * x[i] * x[i];
		double denom = 6.; // 3!
		int sign = -1;
		for(int j=1; j<=terms; j++) {
			value += (double)sign * numer / denom;
		numer *= x[i] * x[i];
		denom *= (2.*(double)j+2.) * (2.*(double)j+3.);
		sign *= -1;
		}
	result[i] = value;
	}
}



int main()
{
	double x[N] = {0, M_PI/6., M_PI/3., 0.134};
	double res[N];
	sinx_taylor(N, 30, x, res);
	for(int i=0; i<N; i++) {
		printf("sin(%.2f) by Taylor series = %f\n", x[i], res[i]);
		printf("sin(%.2f) = %f\n", x[i], sin(x[i]));
	}
	return 0;
}
