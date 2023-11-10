#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_NAME_LENGTH 20
#define MAX_CHANNELS 5

struct ClientInfo {
    int socket;
    char name[MAX_NAME_LENGTH];
    int channel;
};

struct ClientInfo clients[MAX_CLIENTS];

int main() {
    int server_socket, client_sockets[MAX_CLIENTS], client_count = 0;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erreur de liaison");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Erreur d'écoute");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        int max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket > 0) {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_sd) {
                    max_sd = clients[i].socket;
                }
            }
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("Erreur lors de l'utilisation de select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &readfds)) {
            // Nouvelle connexion
            socklen_t client_addr_len = sizeof(client_addr);
            client_sockets[client_count] = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sockets[client_count] == -1) {
                perror("Erreur lors de l'acceptation de la connexion");
                continue;
            }

            // Demander au client de saisir un pseudo et un channel
            send(client_sockets[client_count], "Entrez votre pseudo : ", sizeof("Entrez votre pseudo : "), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(client_sockets[client_count], buffer, sizeof(buffer), 0);
            buffer[strcspn(buffer, "\r\n")] = 0; // Supprimer le retour à la ligne
            strncpy(clients[client_count].name, buffer, MAX_NAME_LENGTH - 1);

            // Demander au client de saisir le numéro du channel
            send(client_sockets[client_count], "Entrez le numéro du channel (0-4) : ", sizeof("Entrez le numéro du channel (0-4) : "), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(client_sockets[client_count], buffer, sizeof(buffer), 0);
            buffer[strcspn(buffer, "\r\n")] = 0; // Supprimer le retour à la ligne
            clients[client_count].channel = atoi(buffer);

            printf("Client connecté : %s (%s) sur le channel %d\n", clients[client_count].name, inet_ntoa(client_addr.sin_addr), clients[client_count].channel);

            // Enregistrement du client
            clients[client_count].socket = client_sockets[client_count];
            client_count++;
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(clients[i].socket, &readfds)) {
                // Un client a envoyé un message
                memset(buffer, 0, sizeof(buffer));
                if (recv(clients[i].socket, buffer, sizeof(buffer), 0) <= 0) {
                    printf("Client déconnecté : %s (%s) du channel %d\n", clients[i].name, inet_ntoa(client_addr.sin_addr), clients[i].channel);
                    close(clients[i].socket);

                    // Retirer le client de la liste
                    clients[i].socket = 0;
                } else {
                    // Afficher le message reçu du client
                    printf("%s (%s) du channel %d : %s", clients[i].name, inet_ntoa(client_addr.sin_addr), clients[i].channel, buffer);

                    // Diffuser le message à tous les autres clients dans le même channel avec le pseudo de l'émetteur
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].socket != 0 && j != i && clients[j].channel == clients[i].channel) {
                            char messageWithPseudo[2048];
                            snprintf(messageWithPseudo, sizeof(messageWithPseudo), "%s (%s) du channel %d : %s", clients[i].name, inet_ntoa(client_addr.sin_addr), clients[i].channel, buffer);
                            send(clients[j].socket, messageWithPseudo, strlen(messageWithPseudo), 0);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
