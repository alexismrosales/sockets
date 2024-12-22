#!/bin/bash

# Compilando archivos importantes
clang ./dhcp_server.c -o dhcp_server_example
clang ./server1_udp.c -o server1_udp
clang ./server2_udp.c -o server2_udp
clang client_udp.c ip_utils.c -o client_udp

# Iniciar el DHCP server
# Inicia en el puerto 15000, y genera IPS a partir de la dirección
# 127.0.0.10 con un máximo de 10 usuarios
# stdbuf fuerza el guardado del log
echo "Iniciando server DHCP en el puerto: 15000"
stdbuf -oL -eL ./dhcp_server_example 15000 127.0.0.10 10 > ./logs/dhcp_server.log &

# Inicio del servidor 1
# Inicia en el puerto 10000
echo "Iniciando server 1 en el puerto: 10000"
stdbuf -oL -eL ./server1_udp 10000 > ./logs/server1_udp.log & 

# Inicio del servidor 2
# Inicia en el puerto 20000
echo "Iniciando server 2 en el puerto: 20000"
stdbuf -oL -eL ./server2_udp 20000 > ./logs/server2_udp.log &

# Verifico si existe el log de clientes y si existe lo elimino
if [ -f ./logs/clients_udp.log ]; then
    rm ./logs/clients_udp.log
fi

# Creo 10 clientes diferentes
for ((i = 1; i <= 10; i++)); do
echo "=== Cliente $i ==="
sudo ./client_udp 127.0.0.1 15000 
echo "" 
done

# Cerrando los procesos de los servidores
echo "Cerrando servidores..."
pkill dhcp_server_exa
pkill server1_udp
pkill server2_udp


# Eliminando binarios
rm dhcp_server_example
rm server1_udp
rm server2_udp
rm client_udp



