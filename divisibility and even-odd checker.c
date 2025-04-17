#include <stdio.h>
int main()
{
    int number;
    printf("Enter a number: ");
    scanf("%d", &number);

    if (number % 5 == 0 && number % 11 == 0) {
        printf("The number is divisible by 5 and 11");
        if (number % 2 == 0)
            printf(", and it is even.\n");
        else
            printf(", and it is odd.\n");
    } else {
        number *= 2;
        printf("The number is not divisible by 5 and 11. Result after multiplication: %d\n", number);
    }

    return 0;
}

