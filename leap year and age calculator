#include <stdio.h>
int main() 
{
int year;
printf("Enter a year: ");
scanf("%d", &year);
if ((year % 400 == 0) && ((year % 100 != 0) || (year % 4 == 0))) {
printf("The year %d is a leap year\n", year);
int age = 2025 - (year + 1);
printf("The person born in %d is at atlest 18 years old (Age %d)", year, age);
} else {
printf("The year %d is not a leap year\n", year);

year = year + 10;
printf("%d", year);
}

return 0;
}


