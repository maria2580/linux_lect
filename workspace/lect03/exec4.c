#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

int main(){
	printf("-->before exec()\n");
	execl("/bin/echo","echo","hello",NULL);
	printf("-->after exec();\n");
	return 0;
}
