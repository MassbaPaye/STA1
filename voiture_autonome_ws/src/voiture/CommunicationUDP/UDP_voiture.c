#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "cJSON.h"
#include <unistd.h>
#include "messages.h"
#include "voiture_globals.h"
#include "UDP_voiture.h"
#include "cJSON.h"

#define PORT 5005
#define BUFFER_SIZE 65535

void* initialisation_communication_camera(void* arg) {
    int sockfd;
    struct sockaddr_in server_addr, sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur création socket");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("✅ Récepteur UDP prêt sur 127.0.0.1:%d\n", PORT);
    printf("En attente de messages JSON...\n\n");

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&sender_addr, &addr_len);
        if (n < 0) {
            perror("Erreur recvfrom");
            break;
        }

        buffer[n] = '\0';

        DonneesDetection *detection = parse_json_to_donnees(buffer);
        set_donnees_detection(detection);
        DonneesDetection *detection2;
        get_donnees_detection(detection2);

        if (detection2) {
            printf("Nombre d’obstacles détectés : %d\n", detection2->count);
            for (int i = 0; i < detection2->count; i++) {
                printf("→ Obstacle %d : type=%d, pointG=(%d,%d,%d), pointD=(%d,%d,%d)\n",
                    i,
                    detection2->obstacle[i].type,
                    detection2->obstacle[i].pointg.x, detection2->obstacle[i].pointg.y, detection2->obstacle[i].pointg.z,
                    detection2->obstacle[i].pointd.x, detection2->obstacle[i].pointd.y, detection2->obstacle[i].pointd.z);
            }
            free(detection2);
        }

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);
        printf("📩 Message reçu de %s:%d\n", sender_ip, ntohs(sender_addr.sin_port));
        printf("Contenu JSON (%d octets) :\n%s\n\n", n, buffer);
        fflush(stdout);
        
    }

    close(sockfd);
    pthread_exit(NULL);
}

DonneesDetection* parse_json_to_donnees(const char *json_str) {
    DonneesDetection *data = malloc(sizeof(DonneesDetection));
    if (!data) {
        fprintf(stderr, "❌ Erreur malloc DonneesDetection\n");
        return NULL;
    }
    memset(data, 0, sizeof(DonneesDetection));

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "❌ Erreur JSON invalide\n");
        free(data);
        return NULL;
    }

    cJSON *count = cJSON_GetObjectItem(root, "count");
    if (cJSON_IsNumber(count)) {
        data->count = count->valueint;
    }

    cJSON *obstacles = cJSON_GetObjectItem(root, "obstacles");
    if (cJSON_IsArray(obstacles)) {
        int nb = cJSON_GetArraySize(obstacles);
        if (nb > MAX_OBSTACLES_SIMULTANES)
            nb = MAX_OBSTACLES_SIMULTANES;

        for (int i = 0; i < nb; i++) {
            cJSON *obstacle_obj = cJSON_GetArrayItem(obstacles, i);
            cJSON *label = cJSON_GetObjectItem(obstacle_obj, "label");
            cJSON *class_id = cJSON_GetObjectItem(obstacle_obj, "class_id");

            if (cJSON_IsNumber(class_id))
                data->obstacle[i].type = (ObstacleType)class_id->valueint;
            else if (cJSON_IsString(label)) {
                if (strcmp(label->valuestring, "PONT") == 0)
                    data->obstacle[i].type = PONT;
                else
                    data->obstacle[i].type = OBSTACLE_VOITURE;
            }

            cJSON *pl = cJSON_GetObjectItem(obstacle_obj, "point_left");
            cJSON *pr = cJSON_GetObjectItem(obstacle_obj, "point_right");

            if (pl && cJSON_IsObject(pl)) {
                data->obstacle[i].pointg.x = (int)(cJSON_GetObjectItem(pl, "x")->valuedouble * 1000);
                data->obstacle[i].pointg.y = (int)(cJSON_GetObjectItem(pl, "y")->valuedouble * 1000);
                data->obstacle[i].pointg.z = (int)(cJSON_GetObjectItem(pl, "z")->valuedouble * 1000);
                data->obstacle[i].pointg.theta = (float)(cJSON_GetObjectItem(pl, "theta")->valuedouble * 180.0 / 3.14159);
            }

            if (pr && cJSON_IsObject(pr)) {
                data->obstacle[i].pointd.x = (int)(cJSON_GetObjectItem(pr, "x")->valuedouble * 1000);
                data->obstacle[i].pointd.y = (int)(cJSON_GetObjectItem(pr, "y")->valuedouble * 1000);
                data->obstacle[i].pointd.z = (int)(cJSON_GetObjectItem(pr, "z")->valuedouble * 1000);
                data->obstacle[i].pointd.theta = (float)(cJSON_GetObjectItem(pr, "theta")->valuedouble * 180.0 / 3.14159);
            }
        }
    }

    cJSON_Delete(root);
    return data;
}

