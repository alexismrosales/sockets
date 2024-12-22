#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

int matrix[10][3] = {
    {11, 12, 13}, {21, 22, 23}, {31, 32, 33}, {41, 42, 43}, {51, 52, 53},
    {61, 62, 63}, {71, 72, 73}, {81, 82, 83}, {91, 92, 93}, {101, 102, 103},
};

int row_size = 10;
int col_size = 3;
int current_row = 0;

// MUTEX es necesario para que no intervengan los hilos en caso que se esten
// modificando a la vez
pthread_mutex_t row_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estructura de datos para pasar los datos al hilo
typedef struct {
  struct sockaddr_in client_addr;
  char message[BUFFER_SIZE];
  int server_socket;
} thread_data_t;

// Se reciben los datos del cliente
void *handle_client(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  int buffer_size = col_size * sizeof(int);
  char *buffer = malloc(buffer_size);
  if (!buffer) {
    perror("Error allocating memory for buffer");
    free(data);
    pthread_exit(NULL);
  }

  printf("Processing request...\n");

  int row_to_send = -1;

  // Bloquear acceso a `current_row` para evitar conflictos
  pthread_mutex_lock(&row_mutex);
  if (current_row < row_size) {
    row_to_send = current_row; // Asignar la fila actual
    current_row++;             // Incrementar la fila para el siguiente cliente
  }
  pthread_mutex_unlock(&row_mutex);

  if (row_to_send != -1) {
    // Enviar el tamaño de la fila primero
    int size = col_size;
    if (send(data->server_socket, &size, sizeof(size), 0) < 0) {
      perror("Error sending size to client");
      free(buffer);
      free(data);
      close(data->server_socket);
      pthread_exit(NULL);
    }

    // Copiar los datos de la fila al buffer
    memcpy(buffer, matrix[row_to_send], buffer_size);

    // Enviar la fila serializada
    if (send(data->server_socket, buffer, buffer_size, 0) < 0) {
      perror("Error sending row to client");
    } else {
      printf("Row %d sent to client.\n", row_to_send);
    }
  } else {
    // Enviar mensaje indicando que no hay más datos disponibles
    const char *no_data_msg = "NO_DATA_AVAILABLE";
    if (send(data->server_socket, no_data_msg, strlen(no_data_msg), 0) < 0) {
      perror("Error sending availability message");
    } else {
      printf("Message sent to client: NO_DATA_AVAILABLE\n");
    }
  }

  // Liberar recursos
  free(buffer);
  free(data);
  close(data->server_socket); // Cerrar el socket del cliente
  pthread_exit(NULL);
}

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
  struct sockaddr_in server_addr, client_addr;
  char buffer[BUFFER_SIZE];
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

    // Recibir mensaje del cliente
    memset(buffer, 0, BUFFER_SIZE); // Resetear el buffer
    if (recv(client_socket, buffer, BUFFER_SIZE, 0) < 0) {
      perror("Error receiving data");
      close(client_socket);
      continue;
    }

    // Removiendo el salto de línea
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strcmp(buffer, "SEND_DATA") != 0) {
      printf("Not a valid request: %s is not a valid message.\n", buffer);
      close(client_socket);
      continue;
    } else {
      printf("Request received successfully.\n");
    }

    // Creando los datos con el struct y asignándolos
    thread_data_t *data = malloc(sizeof(thread_data_t));
    if (data == NULL) {
      perror("Error assigning memory");
      close(client_socket);
      continue;
    }

    memcpy(data->message, buffer, BUFFER_SIZE);
    data->server_socket = client_socket; // Usar el socket del cliente
    data->client_addr = client_addr;

    // Obtener la IP del cliente
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    printf("Received result from client IP: %s\n", client_ip);

    // Creando el hilo
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, (void *)data) != 0) {
      perror("Error creating thread");
      free(data);
      close(client_socket);
      continue;
    }

    // Desvinculando el hilo
    pthread_detach(thread_id);
  }

  close(server_socket);
  return 0;
}
