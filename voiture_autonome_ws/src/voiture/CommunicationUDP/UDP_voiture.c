#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5005
#define BUFFER_SIZE 65535

int initialisation_communication_camera(void) {
    int sockfd;
    struct sockaddr_in server_addr, sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];

    // Création du socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur création socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur (récepteur)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // écoute sur localhost
    server_addr.sin_port = htons(PORT);

    // Liaison du socket à l'adresse locale
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("✅ Récepteur UDP prêt sur 127.0.0.1:%d\n", PORT);
    printf("En attente de messages JSON...\n\n");

    // Boucle de réception
    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&sender_addr, &addr_len);
        if (n < 0) {
            perror("Erreur recvfrom");
            break;
        }

        buffer[n] = '\0';  // Assure la terminaison de chaîne

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);

        printf("📩 Message reçu de %s:%d\n", sender_ip, ntohs(sender_addr.sin_port));
        printf("Contenu JSON (%d octets) :\n%s\n\n", n, buffer);
        fflush(stdout);
    }

    close(sockfd);
    return 0;
}
