#include <stdio.h>
#define INF 9999

int min(int a, int b) { return (a < b ? a : b); }

int main() {
    int n;
    printf("Enter number of nodes: ");
    scanf("%d", &n);

    printf("Enter edge weights between pairs of nodes\n");
    printf("Use -1 if they are not connected\n");
    int weights[n][n];
    int w;
    for (int i = 0; i < n; i++) {
        weights[i][i] = 0;
        for (int j = i + 1; j < n; j++) {
            weights[i][j] = INF;

            printf("Enter weight of edge %d - %d : ", i, j);
            scanf("%d", &w);

            if (w != -1) {
                weights[i][j] = w;
            }
            weights[j][i] = weights[i][j];
        }
    }

    printf("\n\n1. Initial Routing Tables\n");

    for (int i = 0; i < n; i++) {
        printf("\nNode %d\n", i);
        for (int j = 0; j < n; j++) {
            printf("Distance to node %d = %d\n", j, weights[i][j]);
        }
    }

    printf("\n\n2. Update using Bellman Ford\n");
    for (int i = 0; i < n; i++) {
        for (int j = (i + 1); j < n; j++) {
            for (int k = 0; k < n; k++) {
                weights[i][j] =
                    min(weights[i][j], weights[i][k] + weights[k][j]);
                weights[j][i] = weights[i][j];
            }
        }
    }

    printf("\n\n3. Final Routing Tables\n");
    for (int i = 0; i < n; i++) {
        printf("\nNode %d\n", i);
        for (int j = 0; j < n; j++) {
            printf("Distance to node %d = %d\n", j, weights[i][j]);
        }
    }
    return 0;
}
