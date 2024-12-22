#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 512

char *request_ip(char *ip, int port) {
  int client_socket;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];
  socklen_t addr_len = sizeof(server_addr);

  // Crear el socket UDP
  if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Error creating the socket");
    exit(EXIT_FAILURE);
  }

  // Configurar la dirección del servidor
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port); // Puerto del servidor
  inet_pton(AF_INET, ip,
            &server_addr.sin_addr); // Dirección del servidor

  // Enviar solicitud al servidor
  const char *request_message = "DISCOVER";
  if (sendto(client_socket, request_message, strlen(request_message), 0,
             (struct sockaddr *)&server_addr, addr_len) < 0) {
    perror("Error sending message");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // Recibir respuesta del servidor
  memset(buffer, 0, BUFFER_SIZE);
  if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0,
               (struct sockaddr *)&server_addr, &addr_len) < 0) {
    perror("Error receiving response");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // Cerrar el socket
  close(client_socket);
  char *ip_assigned = strdup(buffer);

  return ip_assigned;
}

void set_ip(const char *ip, const char *interface) {
  char command[100];
  snprintf(command, sizeof(command), "sudo ip addr add %s/24 dev %s", ip,
           interface);
  int ret = system(command);
  if (ret != 0) {
    perror("Error setting IP");
    exit(EXIT_FAILURE);
  }
  // Habilitar la interfaz
  snprintf(command, sizeof(command), "sudo ip link set dev %s up", interface);
  ret = system(command);
  if (ret != 0) {
    perror("Error enabling interface");
    exit(EXIT_FAILURE);
  }
}

void remove_ip(const char *ip, const char *interface) {
  char command[256];

  // Comando para eliminar la IP
  snprintf(command, sizeof(command), "sudo ip addr del %s/24 dev %s", ip,
           interface);
  int ret = system(command);
  if (ret != 0) {
    perror("Error removing IP");
    exit(EXIT_FAILURE);
  }
  printf("IP %s removed from interface %s\n", ip, interface);
}
