#include <stdio.h>
#define INF 99999

int main() {
    int n, src;
    printf("enter no of nodes : ");
    scanf("%d", &n);

    int visited[n], distance[n], parent[n];
    int weights[n][n];

    printf("enter weight matrix (0 = no edge)\n");
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            scanf("%d", &weights[i][j]);

    printf("enter source node : ");
    scanf("%d", &src);

    for (int i = 0; i < n; i++) {
        visited[i] = 0;
        distance[i] = INF;
        parent[i] = -1;
    }
    distance[src] = 0;
    parent[src] = src;

    for (int count = 0; count < n - 1; count++) {

        int u = -1;
        for (int i = 0; i < n; i++)
            if (!visited[i] && (u == -1 || distance[i] < distance[u]))
                u = i;

        if (u == -1 || distance[u] >= INF)
            break;
        visited[u] = 1;

        for (int v = 0; v < n; v++) {
            if (visited[v])
                continue;
            if (weights[u][v] == 0)
                continue;
            if (distance[u] + weights[u][v] < distance[v]) {
                distance[v] = distance[u] + weights[u][v];
                parent[v] = u;
            }
        }
    }

    printf("\ndest\t\tcost\t\tpath\n");
    for (int i = 0; i < n; i++) {
        if (i == src)
            continue;
        printf("%d\t\t", i);

        if (distance[i] == INF) {
            printf("INF\t\tno path\n");
            continue;
        }
        printf("%d\t\t", distance[i]);

        int path[n], len = 0, node = i;
        while (node != src) {
            path[len++] = node;
            node = parent[node];
        }
        path[len++] = src;

        for (int j = len - 1; j >= 0; j--) {
            printf("%d", path[j]);
            if (j > 0)
                printf(" -> ");
        }
        printf("\n");
    }

    return 0;
}
