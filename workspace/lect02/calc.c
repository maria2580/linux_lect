#include<stdio.h>
#include<stdlib.h>
int add(int a, int b){
	return a+b;
}
int minus(int a, int b){
	return a-b;
}
int mul(int a, int b){
        return a*b;
}
int divv(int a, int b){
        return a/b;
}

int main(int argc, char *argv[]){
	
	int result=0;
	int a = atoi(argv[1]);
	int b = atoi(argv[3]);

	if (argv[2][0]=='+')
		result = add(a,b);
	else if (argv[2][0]=='-')
		result=minus(a,b);
	else if (argv[2][0]=='x')
		result=mul(a,b);
	else if (argv[2][0]=='/')
		result=divv(a,b);

	printf("%d\n",result);
	return 0;
}
