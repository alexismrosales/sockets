#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

char **ip_pool_global = NULL;
int ip_pool_count = 0;
int max_ips = 0;

// Estructura de datos para pasar los datos al hilo
typedef struct {
  struct sockaddr_in client_addr;
  char message[BUFFER_SIZE];
  int server_socket;
} thread_data_t;

// Genera las ips de forma dinámica dependiendo de la ip inicial y el máximo de
// usuarios.
char **assign_ip(const char *ip_initial, int max_users) {
  char **ip_pool = malloc(max_users * sizeof(char *));
  int ip_size = strlen(ip_initial);
  int initial_range = atoi(&ip_initial[ip_size - 1]);
  char c_number[4]; // Hasta 3 dígitos más '\0'

  // Crear una copia de ip_initial para trabajar
  char *ip_temp = strdup(ip_initial);
  // Iterar sobre max_users
  for (int i = 0; i < max_users; i++) {
    // Convertir el número a cadena
    sprintf(c_number, "%d", initial_range + i);

    // Ajustar tamaño de ip_temp si es necesario
    int new_size = ip_size - 1 + strlen(c_number) + 1;
    char *temp = realloc(ip_temp, new_size);
    ip_temp = temp;

    // Construir la nueva dirección IP
    snprintf(ip_temp, new_size, "%.*s%s", ip_size - 1, ip_initial, c_number);

    // Almacenar en el pool
    ip_pool[i] = strdup(ip_temp);
  }

  free(ip_temp); // Liberar la copia temporal
  return ip_pool;
}

// Manejo del cliente donde se le asignas las ips correspondientes
void *handle_client(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  printf("\nProcessing request...\n");
  // En caso que ya se hayan superado el máximo de ips disponibles
  if (ip_pool_count < max_ips) {
    const char *response = ip_pool_global[max_ips - ip_pool_count - 1];
    if (sendto(data->server_socket, response, strlen(response), 0,
               (struct sockaddr *)&data->client_addr,
               sizeof(data->client_addr)) < 0) {
      perror("Error sending response to client.");
    } else {
      printf("IP sent to client: %s generated.\n", response);
    }
    // Liberar el espacio
    free(ip_pool_global[max_ips - ip_pool_count - 1]);
    ip_pool_count += 1;
  } else {
    printf("IP pool has assigned all IP's\n");
    const char *no_ip_message = "NO_IP_AVAILABLE";
    // Enviar mensaje de error al cliente
    if (sendto(data->server_socket, no_ip_message, strlen(no_ip_message), 0,
               (struct sockaddr *)&data->client_addr,
               sizeof(data->client_addr)) < 0) {
      perror("Error sending response of no avalaibility");
    } else {
      printf("No IP available. Message sent to client.\n");
    }

    // Liberar datos del hilo
    free(data);
    pthread_exit(NULL);
  }
  // Liberando la memoria
  free(data);
  // Cerrando hilo
  pthread_exit(NULL);
}

// Estos son los argumentos requeridos para ejecutar el server
// ./exec [PUERTO] [IP INICIAL] [USUARIOS MÁXIMOS]
// NOTA IMPORTANTE: Para fines prácticos se
// usara la máscara de subred 255.255.255.0
int main(int argc, char *argv[]) {
  // En caso que falten argumentos
  if (argc != 4) {
    printf("Usage: %s [PORT] [INITIAL IP] [MAX USERS]\n", argv[0]);
    return 1;
  }
  // Recibiendo argumentos
  int port = atoi(argv[1]);
  char *ip_initial = argv[2];
  int max_users = atoi(argv[3]);

  printf("DHCP config:\n");
  printf("Port: %d\n", port);
  printf("Start IP address: %s\n", ip_initial);
  printf("Max users: %d\n", max_users);

  int server_socket;
  struct sockaddr_in server_addr, client_addr;
  char buffer[BUFFER_SIZE];
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

  // Generar rango de ips
  ip_pool_global = assign_ip(ip_initial, max_users);
  max_ips = max_users;
  // Empezando a escuchar clientes
  while (1) {
    // Recibir mensajes de clientes
    memset(buffer, 0, BUFFER_SIZE); // Resetear el buffer
    if (recvfrom(server_socket, buffer, BUFFER_SIZE, 0,
                 (struct sockaddr *)&client_addr, &addr_len) < 0) {
      perror("Error recieving data");
      continue;
    }

    // Removiendo el salto de linea
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strcmp(buffer, "DISCOVER") != 0) {
      printf("Not a valid request: %s is not a valid message.\n", buffer);
      continue;
    } else {
      printf("Request recieved succesfully.\n");
    }

    // Creando los datos con el struct y asignandolos
    thread_data_t *data = malloc(sizeof(thread_data_t));
    if (data == NULL) {
      perror("Error assignning memory");
      continue;
    }

    memcpy(data->message, buffer, BUFFER_SIZE);
    data->client_addr = client_addr;
    data->server_socket = server_socket;

    // Creando el hilo
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, (void *)data) != 0) {
      perror("Error creating thread");
      free(data);
      continue;
    }

    // Desvinculando el hilo
    pthread_detach(thread_id);
  }
  close(server_socket);
  return 0;
}
