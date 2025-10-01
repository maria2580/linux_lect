#include <stdio.h>
#include <string.h>
#define MAXLINE 100
void copy(char from[], char to[]);

char line[MAXLINE];
// 입력
char longest[MAXLINE]; // 가장 긴 줄 
void copy(char from[], char to[])
{
	int i;
	i=0;
	while(to[i]=from[i]!='\0')
	++i;
}
int main(){
 int len;
 int max=0;
 while (fgets(line, MAXLINE, stdin) != NULL) {
 if (len > max) {
 max = len;
 copy(line, longest);
}
}

 if (max > 0) // 입력 줄이 있었다면 
printf("%s", longest);

return 0;
}
