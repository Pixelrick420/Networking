#include <limits.h>
#include <stdio.h>

#define MAX 10

int main() {
    int n, src;
    int graph[MAX][MAX];

    printf("No of routers : ");
    scanf("%d", &n);

    printf("Enter cost matrix: \n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            scanf("%d", &graph[i][j]);
        }
    }

    printf("Enter source router : ");
    scanf("%d", &src);

    int dist[MAX], visited[MAX] = {0}, parent[MAX];
    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        parent[i] = -1;
    }

    dist[src] = 0;
    parent[src] = src;

    for (int count = 0; count < n - 1; count++) {
        int min = INT_MAX;
        int u = -1;

        for (int i = 0; i < n; i++) {
            if (!visited[i] && dist[i] < min) {
                min = dist[i];
                u = i;
            }
        }

        visited[u] = 1;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && graph[u][v] && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                parent[v] = u;
            }
        }
    }

    printf("Dest\tCost\tPath\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t", i, dist[i]);

        int path[MAX], k = 0, j = i;
        // recompute the path from parent
        while (j != src) {
            path[k++] = j;
            j = parent[j];
        }
        path[k++] = src;

        for (int p = k - 1; p >= 0; p--) {
            printf("%d ", path[p]);
        }
        printf("\n");
    }
    return 0;
}
