#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

// Estructura de datos para pasar los datos al hilo
typedef struct {
  struct sockaddr_in client_addr;
  char message[BUFFER_SIZE];
  int server_socket;
} thread_data_t;

int main(int argc, char *argv[]) {
  // En caso que falten argumentos
  if (argc != 2) {
    printf("Usage: %s [PORT] \n", argv[0]);
    return 1;
  }
  // Recibiendo argumentos
  int port = atoi(argv[1]);

  printf("UDP config:\n");
  printf("Port: %d\n", port);

  int server_socket;
  int received_value;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

  // Se crea el socket UDP
  if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  // Asignando dirección y puerto al servidor
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Enlace del socket con la dirección ip
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Obtener la dirección IP real usando getsockname
  printf("Server listening in IP: 127.0.0.1, Port: %d\n", port);

  // Empezando a escuchar clientes
  while (1) {
    // Recibir un entero del cliente
    if (recvfrom(server_socket, &received_value, sizeof(received_value), 0,
                 (struct sockaddr *)&client_addr, &addr_len) < 0) {
      perror("Error receiving integer");
      close(server_socket);
      exit(EXIT_FAILURE);
    }

    // Obtener la IP del cliente
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    // Imprimiendo los datos
    printf("Received result from client IP: %s\n", client_ip);
    printf("Multiplicación recibida: %d.\n", received_value);
  }
  close(server_socket);
  return 0;
}
