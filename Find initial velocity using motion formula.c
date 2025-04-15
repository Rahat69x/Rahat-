#include<stdio.h>
int main()
{
double s,t,a,u;
printf("Enter the value of s : ");
scanf("%lf ",&s);
printf("Enter the value of t : ");
scanf("%lf ",&t);
printf("Enter the value of a : ");
scanf("%lf",&a);
double m = s-(0.5*a*t*t);
u=m/t;

printf("Print the value of U = %if\n",u);
return 0;
}
