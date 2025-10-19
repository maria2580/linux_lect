#include<stdio.h>
#include<unistd.h>

int main()
{
	int pid;
	pid=fork();
	if (pid==0){
		printf("[Child] hello, pid=%d \n",getpid());
	}
	else{
		printf("[Parent]:hello,pid = %d\n",getpid());
	}
}
