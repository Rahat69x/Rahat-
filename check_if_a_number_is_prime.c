#include <stdio.h>
int main() {
 int input,count= 0 ;
 printf("enter a number:");
 scanf("%d",&input);
 for (int i = 1; i<= input ; i++)
 {
 if (input %i == 0)
{
count ++ ;
 }
}
 if (count==2)
 {
 printf("prime number\n");
 }
 else
 {
 printf("not a prime number\n");
 }
 return 0;
} 
