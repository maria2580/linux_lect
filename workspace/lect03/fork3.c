#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main()
{
	int pid1,pid2;
	pid1=fork();
	if (pid1==0){
		printf("[Child1] hello, pid=%d \n",getpid());
		exit(0);
	}
	pid2=fork();
	if(pid2==0){
		printf("[Child]: hllo,pid = %d\n",getpid());
		exit(0);
	}
}
