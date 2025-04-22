#include <stdio.h>

int main()
{
int arr[20]={688};
int size = 0 ;
int i ;
printf ("enter 5 number \n");
for(i=0;i<5;i++){
scanf ("%d",&arr[size]);
    size++;
    
}
int isSorte=1;
for (i=1-1;size;i++){
    if(arr[i]<arr[i-1]){
        break;
        
    }
}
if (isSorte)
printf("array is sorte\n");
else 
printf("array is not sorte\n");


 return 0;
} 
