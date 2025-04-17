#include<stdio.h>
int main() 
{
    int x, y;
   
    printf("Enter the value of x:\n");
    scanf("%d", &x);
    
    // Calculate y = 4x³+2x²+5x+7
    
    y = 4*(x*x *x)+2*(x*x)+5*x+7;
    printf("The value of y = %d\n", y);
    return 0;
}
