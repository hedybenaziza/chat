#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080

int client_socket;
char buffer[1024];

void *receiveMessages(void *arg) {
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
            perror("Erreur lors de la réception du message du serveur");
            break;
        }

        // Afficher le message reçu du serveur de manière lisible sur une nouvelle ligne
        printf("\n%s", buffer);

        // Réafficher le message d'invitation à la saisie
        printf("Entrez un message : ");
        fflush(stdout);  // Vider le tampon de sortie pour éviter des problèmes d'affichage
    }

    // Fermer le socket et terminer le programme en cas de problème de réception
    close(client_socket);
    exit(EXIT_FAILURE);
}

int main() {
    struct sockaddr_in server_addr;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erreur de connexion au serveur");
        exit(EXIT_FAILURE);
    }

    printf("Connecté au serveur.\n");

    // Demander au client de saisir un pseudo
    printf("Entrez votre pseudo : ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\r\n")] = 0; // Supprimer le retour à la ligne
    send(client_socket, buffer, strlen(buffer), 0);

    // Demander au client de saisir le numéro du channel
    printf("Entrez le numéro du channel (0-4) : ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\r\n")] = 0; // Supprimer le retour à la ligne
    send(client_socket, buffer, strlen(buffer), 0);

    // Créer un thread pour la réception des messages
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receiveMessages, NULL) != 0) {
        perror("Erreur lors de la création du thread de réception");
        exit(EXIT_FAILURE);
    }

    // Envoyer et recevoir des messages dans le thread principal
    while (1) {
        printf("Entrez un message : ");
        fgets(buffer, sizeof(buffer), stdin);

        // Vérifier si la longueur du message est suffisante
        if (strlen(buffer) > 1) {
            // Envoyer le message au serveur
            send(client_socket, buffer, strlen(buffer), 0);
        } else {
            printf("Le message doit contenir au moins un caractère.\n");
        }
    }

    // Attendre la fin du thread de réception
    pthread_join(receive_thread, NULL);

    close(client_socket);

    return 0;
}
