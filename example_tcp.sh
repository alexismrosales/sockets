#!/bin/bash

# Compilando archivos importantes
clang ./dhcp_server.c -o dhcp_server_example
clang ./server1_tcp.c -o server1_tcp
clang ./server2_tcp.c -o server2_tcp
clang client_tcp.c ip_utils.c -o client_tcp

mkdir logs
# Iniciar el DHCP server
# Inicia en el puerto 15000, y genera IPS a partir de la dirección
# 127.0.0.10 con un máximo de 10 usuarios
# stdbuf fuerza el guardado del log
echo "Iniciando server DHCP en el puerto: 15000"
stdbuf -oL -eL ./dhcp_server_example 15000 127.0.0.10 10 > ./logs/dhcp_server.log &

# Inicio del servidor 1
# Inicia en el puerto 10000
echo "Iniciando server 1 en el puerto: 10000"
stdbuf -oL -eL ./server1_tcp 10000 > ./logs/server1_tcp.log & 

# Inicio del servidor 2
# Inicia en el puerto 20000
echo "Iniciando server 2 en el puerto: 20000"
stdbuf -oL -eL ./server2_tcp 20000 > ./logs/server2_tcp.log &


# Creo 10 clientes diferentes
for ((i = 1; i <= 10; i++)); do
echo "=== Cliente $i ==="
sudo ./client_tcp 127.0.0.1 15000 
echo "" 
done

# Cerrando los procesos de los servidores
echo "Cerrando servidores..."
pkill dhcp_server_exa
pkill server1_tcp
pkill server2_tcp


# Eliminando binarios
rm dhcp_server_example
rm server1_tcp
rm server2_tcp
rm client_tcp
