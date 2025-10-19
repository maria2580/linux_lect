#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>

int main(int argc,char *argv[]){
	int child, pid,status;
	pid= fork();
	if(pid==0){
		printf("PGRP of Child =%d \n",getpgrp());
	
		while(1){
			printf("child is waiting..\n ");
			sleep(1);
		}
	}else{
		printf("[%d] child %d is terminated \nwith status %d ",getpid(),pid,status>>8);
		
		printf("Parent is sleeping..\n");
		sleep(5);
	}
	return 0;

}
