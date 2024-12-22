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

int *receive_array(int client_socket) {
  int size;

  // Recibir el tamaño del arreglo primero
  if (recv(client_socket, &size, sizeof(size), 0) < 0) {
    perror("Error receiving array size");
    return NULL;
  }

  printf("Size of array: %d\n", size);

  // Reservar memoria para el arreglo
  int *arr = malloc(size * sizeof(int));
  if (arr == NULL) {
    perror("Error allocating memory for array");
    return NULL;
  }

  // Recibir el arreglo serializado
  if (recv(client_socket, arr, size * sizeof(int), 0) < 0) {
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

  char *server_ip = argv[1];
  int server_port = atoi(argv[2]);

  // Preguntar al servidor DHCP por una IP
  char *ip = request_ip(server_ip, server_port);
  // Configurar la ip dada por el sevidor
  // NOTA: En este caso estamos haciendolo en la interfaz de red local
  set_ip(ip, "lo");
  if (strcmp(ip, "NO_IP_AVAILABLE") == 0) {
    printf("Error assigning ip: NO_IP_AVAILABLE");
    exit(1);
  }
  /* PREGUNTA AL PRIMER SERVIDOR POR LOS DATOS */
  int client_socket;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];

  // Crear el socket TCP
  if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating the socket");
    exit(EXIT_FAILURE);
  }

  // Configurar la dirección del servidor
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT1);           // Puerto del servidor
  inet_pton(AF_INET, ip, &server_addr.sin_addr); // Dirección del servidor

  // Conectar al servidor
  if (connect(client_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // Enviar solicitud al servidor
  const char *request_message = "SEND_DATA";
  if (send(client_socket, request_message, strlen(request_message), 0) < 0) {
    perror("Error sending message");
    close(client_socket);
    exit(EXIT_FAILURE);
  }
  printf("Message sent to server: %s\n", request_message);
  int *arr = receive_array(client_socket);
  /* FIN DEL PRIMER SERVIDOR*/

  // Calcular la multiplicación
  int multiplication = 1;
  for (int i = 0; i < 3; i++) {
    multiplication *= arr[i];
  }
  printf("Multiplication:%d\n", multiplication);

  // Cambiar el puerto para la conexión al segundo servidor
  server_addr.sin_port = htons(PORT2);

  // Cerrar la conexión actual
  close(client_socket);

  // Crear un nuevo socket para la conexión al segundo servidor
  if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket for second server");
    exit(EXIT_FAILURE);
  }

  // Conectar al segundo servidor
  if (connect(client_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("Error connecting to second server");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // Enviar los datos al segundo servidor
  if (send(client_socket, &multiplication, sizeof(multiplication), 0) < 0) {
    perror("Error sending data to second server");
    close(client_socket);
    exit(EXIT_FAILURE);
  } else {
    printf("Data sent successfully to second server\n");
  }
  printf("IP assigned: %s\n", ip);

  // Removiendo la ip asignada en la interfaz de red
  remove_ip(ip, "lo");

  return 0;
}
