#include <stdio.h>
int main()
{
 int x,y,add ,sub ,mul,div,mod,choose ;
 printf ("rahat calculator .\n");
 printf ("five operation supported .\n");
printf("1. addition (+)\n2. subtraction (-)\n3. multiplication (*)\n4. division (/)\n5.modulus (%%)\n");
printf ("choose one operator - ");
scanf ("%d", & choose );
if (choose == 1 ) {
 printf ("you choosed addition.\n");
 printf ("enter 1st digit - ");
 scanf ("%d",&x);
 printf ( "enter 2nd digit - ");
 scanf("%d", & y );
 add = x+y ;
 printf("answer = %d" ,add);
}
else if (choose == 2 ) {
 printf ("you choosed subtraction.\n");
 printf ("enter 1st digit - ");
 scanf ("%d",&x) ;
 printf ( "enter 2nd digit - ");
 scanf("%d", & y );
 sub = x-y ;
 printf ("answer = %d" ,sub);
}
else if (choose == 3 ) {
 printf ("you choosed multiplcation.\n");
 printf ("enter 1st digit - ");
 scanf ("%d",& x ) ;
 printf ( "enter 2nd digit - ");
 scanf("%d", & y );
 add = x*y ;
 printf ("answer = %d ",mul);
}
 else if (choose == 4 ) {
 printf ("you choosed divison.\n");
 printf ("enter 1st digit - ");
 scanf ("%d",&x) ;
 printf ( "enter 2nd digit - ");
 scanf("%d", & y );
 sub = x-y ;
 printf ("answer = %d" ,div);
 }
else if (choose == 5 ) {
 printf ("you choosed modulas.\n");
 printf ("enter 1st digit - ");
 scanf ("%d",x);
 printf ( "enter 2nd digit - ");
 scanf("%d",&y);
 sub = x%y;
 printf("answer = %d" ,mod );
 }
 return 0;
}
    
    
    
    
    
 
