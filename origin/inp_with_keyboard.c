#include <stdio.h>
#include <stdlib.h>

int main()
{
char word[20];
fgets(word,sizeof(word),stdin);
printf("%s",word);
return 0;
}
