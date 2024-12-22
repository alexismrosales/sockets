#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

int main(int argc, char *argv[]) {
  // En caso que falten argumentos
  if (argc != 2) {
    printf("Usage: %s [PORT] \n", argv[0]);
    return 1;
  }
  // Recibiendo argumentos
  int port = atoi(argv[1]);

  printf("TCP config:\n");
  printf("Port: %d\n", port);

  int server_socket, client_socket;
  int received_value;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

  // Se crea el socket TCP
  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  // Asignando dirección y puerto al servidor
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Enlace del socket con la dirección IP
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Escuchar conexiones entrantes
  if (listen(server_socket, 5) < 0) {
    perror("Error listening");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on IP: 127.0.0.1, Port: %d\n", port);

  // Empezando a aceptar clientes
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
      perror("Error accepting connection");
      continue;
    }

    // Recibir un entero del cliente
    if (recv(client_socket, &received_value, sizeof(received_value), 0) < 0) {
      perror("Error receiving integer");
      close(client_socket);
      continue;
    }

    // Obtener la IP del cliente
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    // Imprimiendo los datos
    printf("Received result from client IP: %s\n", client_ip);
    printf("Multiplicación recibida: %d.\n", received_value);

    // Cerrar conexión con el cliente
    close(client_socket);
  }

  close(server_socket);
  return 0;
}
