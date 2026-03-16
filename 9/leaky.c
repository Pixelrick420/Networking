#include <stdio.h>
#include <stdlib.h>

int main() {
    int capacity, rate, n, leak;
    int bucket = 0;

    printf("enter capacity : ");
    scanf("%d", &capacity);

    printf("enter rate : ");
    scanf("%d", &rate);

    printf("enter no of packets : ");
    scanf("%d", &n);

    int *packets = (int *)malloc(sizeof(int) * n);
    printf("enter packet sizes\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &packets[i]);
    }

    for (int i = 0; i < n; i++) {
        printf("recieved packet with size %d\n", packets[i]);
        if ((packets[i] + bucket) > capacity) {
            printf("bucket overflow, dropped packet\n");
        } else {
            bucket += packets[i];
            printf("packet added to bucket\n");
        }

        leak = (bucket < rate) ? bucket : rate;
        printf("leaked amount : %d\n", leak);
        bucket -= leak;
        printf("bucket : %d\n\n", bucket);
    }

    while (bucket > 0) {
        leak = (bucket < rate) ? bucket : rate;
        printf("leaked amount : %d\n", leak);
        bucket -= leak;
        printf("bucket : %d\n\n", bucket);
    }
    return 0;
}
