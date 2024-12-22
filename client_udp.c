#include "ip_utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define PORT1 10000
#define PORT2 20000

// Estructura de datos para pasar los datos al hilo
typedef struct {
  struct sockaddr_in client_addr;
  char message[BUFFER_SIZE];
  int server_socket;
} thread_data_t;

int *receive_array(int client_socket, struct sockaddr_in *server_addr,
                   socklen_t addr_len) {
  int size;
  // Recibir el tamaño del arreglo primero
  if (recvfrom(client_socket, &size, sizeof(size), 0,
               (struct sockaddr *)server_addr, &addr_len) < 0) {
    perror("Error recieving array size");
    return NULL;
  }

  printf("Size of array: %d\n", size);

  // Recibir el arreglo serializado
  int *arr = malloc(size * sizeof(int));

  if (recvfrom(client_socket, arr, size * sizeof(int), 0,
               (struct sockaddr *)server_addr, &addr_len) < 0) {
    perror("Error receiving array");
    free(arr);
    return NULL;
  }

  // Mostrar el arreglo deserializado
  printf("Array: ");
  for (int i = 0; i < size; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
  return arr;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s [SERVER_IP] [DHCP PORT]\n", argv[0]);
    return 1;
  }

  char *server_ip = argv[1];       // Cambiado a argv[1]
  int server_port = atoi(argv[2]); // Cambiado a argv[2]
                                   //
  // Preguntar al servidor DHCP por una IP
  char *ip = request_ip(server_ip, server_port);
  // Configurar la ip dada por el sevidor
  // NOTA: En este caso estamos haciendolo en la configuración local
  set_ip(ip, "lo");
  if (strcmp(ip, "NO_IP_AVAILABLE") == 0) {
    printf("Error assigning ip: NO_IP_AVAILABLE");
    exit(1);
  }

  printf("IP assigned: %s\n", ip);

  /* PREGUNTA AL PRIMER SERVIDOR POR LOS DATOS*/
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
  server_addr.sin_port = htons(PORT1); // Puerto del servidor
  inet_pton(AF_INET, ip,
            &server_addr.sin_addr); // Dirección del servidor

  // Enviar solicitud al servidor
  const char *request_message = "SEND_DATA";
  if (sendto(client_socket, request_message, strlen(request_message), 0,
             (struct sockaddr *)&server_addr, addr_len) < 0) {
    perror("Error sending message");
    close(client_socket);
    exit(EXIT_FAILURE);
  }
  int *arr = receive_array(client_socket, &server_addr, addr_len);
  /* FIN DEL PRIMER SERVIDOR*/

  /* INICIO DEL SEGUNDO SERVIDOR */

  int multiplication = 1;
  for (int i = 0; i < 3; i++) {
    multiplication *= arr[i];
  }
  server_addr.sin_port = htons(PORT2); // Reasignando el puerto

  // Enviar los datos del arreglo
  if (sendto(client_socket, &multiplication, sizeof(multiplication), 0,
             (struct sockaddr *)&server_addr, addr_len) < 0) {
    perror("Error sending array data");
    close(client_socket);
    exit(EXIT_FAILURE);
  } else {
    printf("Array sent successfully\n");
  }

  // Removing interface
  remove_ip(ip, "lo");
  close(client_socket);
  return 0;
}
