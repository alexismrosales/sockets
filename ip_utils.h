// client.h
#ifndef IP_UTILS_H
#define IP_UTILS_H
// Declaración de la función
char *request_ip(const char *ip, int port);
void set_ip(const char *ip, const char *interface);
void remove_ip(const char *ip, const char *interface);
#endif // IP_UTILS_H
