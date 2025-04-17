#include <stdio.h>
int main()
{
 int n;
 int reverse = 0, digit;
 scanf("%d", &n);
 for
 (; n> 0; n/= 10) {
 digit = n% 10;
 reverse=reverse*10 + digit;
 }
 printf("reverse number: %d\n", reverse);
 return 0;
}
