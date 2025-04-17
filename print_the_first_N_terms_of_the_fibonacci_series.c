#include <stdio.h>
int main() {
 int n, p1= 0, p2= 1, next;
 printf("enter the number of terms: ");
 scanf("%d",&n);
 for
 (int i = 1; i <= n; i++)
 {
 printf("%d ", p1);
 next = p1 + p2;
 p1 = p2;
 p2 = next;
 }
 return 0;
}

