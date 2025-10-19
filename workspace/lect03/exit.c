#include<stdio.h>
#include<stdlib.h>

void cleanup1(){
	printf("clean up 1 is called\n");
}

void cleanup2(){
	printf("clean up 2 is called\n");
}
int main()
{
	atexit(cleanup1);
	atexit(cleanup2);
	exit(0);
	
}
