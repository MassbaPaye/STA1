#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "messages.h"
#include "cJSON.h"

#define DEFAULT_PORT 8080

// Variable globale partagée avec protection mutex
Obstacle obstacle_courant = {0};
pthread_mutex_t mutex_obstacle = PTHREAD_MUTEX_INITIALIZER;

// === Boucle infinie d’écoute des obstacles ===
void boucle_detection_obstacle(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    listen(server_fd, 3);
    printf("[Detection] écoute sur le port %d\n", port);

    char buffer[4096];
    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int client_fd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
        if (client_fd < 0) continue;

        int n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
        } else {
            close(client_fd);
            continue;
        }

        cJSON* root = cJSON_Parse(buffer);
        if (root) {
            cJSON* dist = cJSON_GetObjectItem(root, "distance");
            cJSON* type = cJSON_GetObjectItem(root, "type");
            cJSON* pg = cJSON_GetObjectItem(root, "pointg");
            cJSON* pd = cJSON_GetObjectItem(root, "pointd");

            // Verrouillage du mutex pour mise à jour thread-safe
            pthread_mutex_lock(&mutex_obstacle);
            
            if (dist) obstacle_courant.distance = dist->valueint;
            if (type) obstacle_courant.type = type->valueint;
            
            if (pg) {
                cJSON* pg_x = cJSON_GetObjectItem(pg, "x");
                cJSON* pg_y = cJSON_GetObjectItem(pg, "y");
                if (pg_x) obstacle_courant.pointg.x = pg_x->valueint;
                if (pg_y) obstacle_courant.pointg.y = pg_y->valueint;
            }
            
            if (pd) {
                cJSON* pd_x = cJSON_GetObjectItem(pd, "x");
                cJSON* pd_y = cJSON_GetObjectItem(pd, "y");
                if (pd_x) obstacle_courant.pointd.x = pd_x->valueint;
                if (pd_y) obstacle_courant.pointd.y = pd_y->valueint;
            }

            printf("Obstacle mis à jour : dist=%d, type=%d\n",
                   obstacle_courant.distance, obstacle_courant.type);
            
            pthread_mutex_unlock(&mutex_obstacle);

            cJSON_Delete(root);
        }
        close(client_fd);
    }

    close(server_fd);
}
