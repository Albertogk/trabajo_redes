#ifndef CONEXION_H_
#define CONEXION_H_

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

/********************************************************************
Nombre: Abrir_socket
Descripcion: Prepara el socket para comunicar TCP con nivel de
aplicación

Entrada: //TODO Quizas hacerlo con posibilidades
Salida:
	int:	descriptor del socket en caso de ejecución correcta
		 	-1 (ERR) en caso de error

sockfd = socket(AF_INET, SOCK_STREAM, 0)
**********************************************************************/
int Abrir_Socket();

/********************************************************************
Nombre: Enlazar
Descripcion: Liga el puerto a socket especificado

Entrada:
	int 	socketfd	El descriptor del socket
	uint16_t 	port		El numero de puerto

Salida:
	int:	0 (OK)		en caso de ejecución correcta
		 	-1 (ERR)	en caso de error


**********************************************************************/
int Enlazar(int sockfd, uint16_t port);

/********************************************************************
Nombre: Esuchar
Descripcion: Indica que el socket especificado como socket pasivo,
es decir, como socket que se utilizara para aceptar la conexion entrante

Entrada:
	int 	socketfd	El descriptor del socket
	int 	backlog		El numero maximo de conexiones pendientes (en la cola)

Salida:
	int:	0 (OK)		en caso de ejecución correcta
		 	-1 (ERR)	en caso de error


**********************************************************************/
int Escuchar(int sockfd, int backlog);

/********************************************************************
Nombre: Aceptar_Conexion
Descripcion: Toma una conexion y crea un nuevo socket de comunicacion ????

Entrada:
	int 	socketfd	El descriptor del socket
	muchas cosas mas

Salida:
	int:	 clientfd	el descriptor de la conexion con el cliente???
	int	 	-1 (ERR)	en caso de error

clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
**********************************************************************/
int Aceptar_Conexion(int sockfd, struct sockaddr_in *client_addr);

/********************************************************************
Nombre: Leer
Descripcion: Lee (recibe) un mensaje desde el socket

Entrada:
	int 	clientfd	El descriptor de la conexion con el cliente
	void * 	buffer		Buffer que almacenara el mensaje recibido
	size_t	len			Tamaño del buffer

Salida:??? retorno del recv o OK/ERR
recv(clientfd, buffer, MAXBUF, 0)
**********************************************************************/
ssize_t Leer(int clientfd, void * buffer, size_t bufflen);

/********************************************************************
Nombre: Enviar
Descripcion: Envia la respuesta del servidor al cliente

Entrada:
	int 	clientfd	El descriptor de la conexion con el cliente
	void * 	buffer		Buffer que almacenara el mensaje recibido
	size_t	len			Tamaño del buffer

Salida:??? retorno del  send o OK/ERR
send(clientfd, buffer, recv(clientfd, buffer, MAXBUF, 0), 0);
**********************************************************************/
ssize_t Enviar(int clientfd, void * buffer, size_t len);

int Procesar_GET(int clientfd, char *file_path, char *server_signature,  pthread_mutex_t *sem_pipe);

int Procesar_Peticion(int clientfd, char *server_root, char *server_signature,  pthread_mutex_t *sem_pipe);

int Enviar_Respuesta(int clientfd, char *buffer, int length, FILE *fp);

int Procesar_Script(int clientfd, char *command, char *file_path, char *server_signature, char *input,  pthread_mutex_t *sem_pipe);

int Procesar_POST(int clientfd, char *file_path, char *server_signature, char *content,  pthread_mutex_t *sem_pipe);

int Procesar_OPTIONS(int clientfd, char *server_signature);

#endif
