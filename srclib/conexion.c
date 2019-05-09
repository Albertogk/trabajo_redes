
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../includes/picohttpparser.h"
#include "../includes/conexion.h"
#include "../includes/respuesta.h"
#include <syslog.h>
#include <pthread.h>
#include <fcntl.h>

#define MAXBUF 1024
#define ERR -1
#define SUCCESS 0
#define NOT_SCRIPT -2
#define NOT_FOUND 404

#define OK 200
#define ISE 500
#define BAD_REQUEST 400
#define MAXHDR 256
#define MAXCOM 512


/************************************************************************/
/* Nombre: Abrir_Socket                                                 */
/* Descripcion: Abre un socket TCP                                      */
/*                                                                      */
/* Entrada:                                                             */
/*		nada                                                            */
/*                                                                      */
/* Salida:                                                              */
/* 		int descriptor fichero del socket en caso de ejecucion correcta */
/*			-1 en caso de error y errno toma el valor apropiado         */
/*                                                                      */
/************************************************************************/
int Abrir_Socket() {
	int socketfd;

	/* 1er parametro especifica que se va a usar protocolo de internet IPv4       */
	/* 2do, el tipo de socket, en este caso STREAM                                */
	/* y el 3o, el 0 indica que se vaya a usar el protocolo de socket por defecto */
	socketfd = socket(AF_INET, SOCK_STREAM, 0);

	return socketfd;
}

/********************************************************************/
/* Nombre: Enlazar                                                  */
/* Descripcion: Liga el socket a un puerto                          */
/*                                                                  */
/* Entrada:                                                         */
/*		int socketfd : el descriptor del socket                     */
/* 		uint16_t port : el numero de puerto                         */
/*                                                                  */
/* Salida:                                                          */
/* 		int  0 en caso de ejecuccion correcta                       */
/*			-1 en caso de error y errno toma el valor apropiado     */
/*                                                                  */
/********************************************************************/
int Enlazar(int socketfd, uint16_t port) {
	int check;
	struct sockaddr_in addr;
	//probar sizeof

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;			/* familia de dirreciones (IPv4 en este caso) */
	addr.sin_port = htons(port);		/* numero de puerto */
	addr.sin_addr.s_addr = INADDR_ANY;	/* dirreciones de internet admitidas, en este caso, todas*/

	check = bind(socketfd, (const struct sockaddr *)&addr, sizeof(addr));

	return check;
}


/**********************************************************************/
/* Nombre: Escuchar                                                   */
/* Descripcion: Indica que el socket va a aceptar conexiones          */
/*                                                                    */
/* Entrada:                                                           */
/*		int socketfd : el descriptor del socket                       */
/* 		int backlog : el tamaño de la cola de conexiones pendientes   */
/*                                                                    */
/* Salida:                                                            */
/* 		int  0 en caso de ejecuccion correcta                         */
/*			-1 en caso de error y errno toma el valor apropiado       */
/*                                                                    */
/**********************************************************************/
int Escuchar(int socketfd, int backlog) {
	int check;

	check = listen(socketfd, backlog);

	return check;
}


/*******************************************************************************/
/* Nombre: Aceptar_Conexion                                                    */
/* Descripcion: Extrae una conexion de la lista de conexiones pendientes       */
/*				y crea un nuevo socket devolviendo su descriptor               */
/*                                                                             */
/* Entrada:                                                                    */
/*		int socketfd : el descriptor del socket                                */
/* 		struct sockaddr_in client_addr : estructura                            */
/*				a rellenar con la direccion del cliente por accept (mirar eso) */
/*                                                                             */
/* Salida:                                                                     */
/* 		int  descriptor de fichero en caso de ejecucion correcta               */
/*			-1 en caso de error y errno toma el valor apropiado                */
/*                                                                             */
/*******************************************************************************/
int Aceptar_Conexion(int sockfd, struct sockaddr_in *client_addr) {
	int clientfd;
	socklen_t addrlen=sizeof(client_addr);

	clientfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen);

	return clientfd;
}


/************************************************************************/
/* Nombre: Leer                                                         */
/* Descripcion: Lee la informacion enviada por el cliente               */
/*                                                                      */
/* Entrada:                                                             */
/*		int clientfd : el descriptor del cliente                        */
/* 		void * buffer : el buffer de almacenamiento                     */
/*		size_t bufflen : el tamaño del buffer                           */
/*                                                                      */
/* Salida:                                                              */
/* 		ssize_t  numero de bytes leidos en caso de ejecucion correcta   */
/*				-1 en caso de error y errno toma el valor apropiado     */
/*                                                                      */
/************************************************************************/
ssize_t Leer(int clientfd, void * buffer, size_t bufflen) {
	ssize_t length;

	length = recv(clientfd, buffer, bufflen, 0);

	return length;
}


/**************************************************************************/
/* Nombre: Enviar                                                         */
/* Descripcion: Envia la informacion al cliente por el socket             */
/*                                                                        */
/* Entrada:                                                               */
/*		int clientfd : el descriptor del cliente                          */
/* 		void * buffer : el buffer de almacenamiento                       */
/*		size_t bufflen : el tamaño del buffer                             */
/*                                                                        */
/* Salida:                                                                */
/* 		ssize_t  numero de bytes enviados en caso de ejecucion correcta   */
/*				-1 en caso de error y errno toma el valor apropiado       */
/*                                                                        */
/**************************************************************************/
ssize_t Enviar(int clientfd, void * buffer, size_t len) {
	ssize_t length;

	length = send(clientfd, buffer, len, 0);

	return length;

}

/****************************************************************************/
/* Nombre: Crear_Comando_Script                                             */
/* Descripcion: Crea el comando que va a ejecutar el script y compruba      */
/* 		que efectivamente el archivo sea un script y que la peticion esta   */
/* 		correctamente formada                                               */
/*                                                                          */
/* Entrada:                                                                 */
/*		char * path : el path del fichero a ejecutar                        */
/* 		void * command : buffer donde se va a almacenar el comando          */
/*                                                                          */
/* Salida:                                                                  */
/* 		int 0 ejecucion correcta y archivo un script                        */
/*			-2 en caso de que no sea script                                 */
/*			-1 en caso de error (peticion mal formada)                      */
/*                                                                          */
/****************************************************************************/
int Crear_Comando_Script(char *path, char *command) {
    char *script_path, *args, *extension, path_aux[MAXBUF];

    script_path = strtok(path,"?");
	args = strtok(NULL, "\0");

	/* Crea el path del archivo */
    strcpy(path_aux, script_path);
    strtok(path_aux, ".");

    extension=strtok(NULL, ".");

    /* Caso de una peticion mal formada */
    if(extension == NULL) {

    	syslog(LOG_ERR, "Peticion mal formada");
        return ERR;
    }

    /* Caso de un script de python */
    else if(strcmp(extension, "py") == 0) {

    	syslog(LOG_INFO, "Peticion de ejecucion de un script de python");

    	/* Crea el comando con python + script_path */
        sprintf(command, "python3 %s ", script_path);

        syslog(LOG_INFO, "Comando de python creado");

    }

    /* Caso de un script de php */
    else if(strcmp(extension, "php") == 0) {

    	syslog(LOG_INFO, "Peticion de ejecucion de un script de php");

    	/* Crea el comando con php + script_path */
        sprintf(command, "php %s ", script_path);

        syslog(LOG_INFO, "Comando de php creado");
    }

    /* Caso de un archivo que no es un script */
    else {

    	syslog(LOG_INFO, "Peticion de un archivo (no script)");
        return NOT_SCRIPT;
    }

  	if(args != NULL) {

  		/* Añade al comando los argumentos */
		strcat(command, args);
	}

    return SUCCESS;

}

/***********************************************************************/
/* Nombre: Crear_Cabecera                                              */
/* Descripcion: Crea las cabeceras de la respuesta                     */
/*		(Date, Server, Content-Lenght, Content-Type, Last-Modified)    */
/*                                                                     */
/* Entrada:                                                            */
/*                                                                     */
/*                                                                     */
/* Salida:                                                             */
/* 		int 0 ejecucion correcta y archivo un script                   */
/*			-2 en caso de que no sea script                            */
/*			-1 en caso de error (peticion mal formada)                 */
/*                                                                     */
/***********************************************************************/
int Crear_Cabecera(int code, char *server_signature, char *file_path, FILE *fp, char *buffer, int longitud) {
	char header[MAXHDR];
	int length = 0, check;

	/* Se añade al mensaje la linea de respuesta*/
	sprintf(header, "HTTP/1.1 %d\r\n", code);
	strcpy(buffer, header);

	/* Se añaden las cabeceras de fecha y el nombre del servidor*/
	Rellenar_Date(header);
	strcat(buffer,header);

    /* Se añade la firma del servidor */
	Rellenar_Server(server_signature, header);
	strcat(buffer, header);

	/* Se añade la cabecera del tipo de contenido */
	Rellenar_Content_Type(file_path, header);
	strcat(buffer, header);

	syslog(LOG_INFO, "Cabeceras creadas correctamente");

	/* Se añade la cabecera de la longitud del contenido */
	length = Rellenar_Content_Length(fp, header, longitud);
    if (length == ERR) {
        syslog(LOG_ERR, "Error en Rellenar_Content_Length");
        return ERR;
    }
	strcat(buffer, header);
    /* Si file_path es NULL, se trata de una peticion OPTIONS. Rellenamos
        Allow y salimos*/
	if(file_path == NULL) {
		Rellenar_Allow(header);
		strcat(buffer, header);
        /* se añade el /r/n que separa las cabeceras del contendido a enviar */
        strcat(buffer, "\r\n");
		return SUCCESS;
	}
	/* Se añade la cabecera de la ultima modificacion si no es un script*/
	if (fp != NULL) {

		check = Rellenar_Last_Modified(fp, header);
	    if (check == ERR) {
	        syslog(LOG_ERR, "Error en Rellenar_Last_Modified");
	        return ERR;
	    }

	    strcat(buffer, header);
	}


	/* se añade el /r/n que separa las cabeceras del contendido a enviar */
	strcat(buffer, "\r\n");
	return length;
}


/**********************************************************************/
/* Nombre: Enviar_Respuesta								              */
/* Descripcion: Envia la respuesta al cliente                         */
/*			Las cabeceras correspondientes y los datos solicitados    */
/*																	  */
/* Entrada:	                                                          */
/*		int clientfd : descriptor de çl fichero del cliente           */
/*		char * buffer : el buffer que contiene las cabeceras          */
/*		int lenght : la longitud del archivo a enviar                 */
/*		FILE * fp : puntero al archivo                                */
/*															          */
/*		                                                              */
/*																	  */
/* Salida:															  */
/* 		int 0 ejecucion correcta                                      */
/*			-1 en caso de error (peticion mal formada)	              */
/*																	  */
/**********************************************************************/
int Enviar_Respuesta(int clientfd, char *buffer, int length, FILE *fp) {
	ssize_t ret, nbytes, aux, total_bytes = 0;
	size_t bytes_used;

	/* Si length es 0 (no hay cuerpo en la respuesta)  se envian las cabeceras correspondientes*/
	if(length == 0) {
		syslog(LOG_INFO, "Enviando una respuesta de tamaño 0");
		ret = Enviar(clientfd, buffer, strlen(buffer) + 1);

		if (ret == ERR) {
			syslog(LOG_INFO, "Error al enviar respuesta: %s", strerror(errno));
			return ERR;
		}

		return SUCCESS;
	}

	/* Se calculan los bytes usados para las cabeceras*/
	bytes_used = strlen(buffer);

	/* Lectura de archivo */
	/* Se envian los bytes en grupos de 1024*/
	/* En la primera interacion se envia la cabecera, despues se va enviando solo el contenido del archivo*/
	do {

		/* Se guardan los conetnidos del archivo en la parte no ocupada del buffer */
		nbytes = fread(buffer + bytes_used, sizeof(char), MAXBUF - bytes_used, fp);
		if (nbytes < 0) {
			perror("Error al leer el archivo");
			return ERR;
		}

		total_bytes += nbytes;
		aux = bytes_used;
		bytes_used = 0;

		/* Se ha terminado de leer el archivo*/
		if (nbytes == 0) {
			syslog(LOG_INFO, "Se ha terminado de enviar los datos");
			break;
		}

		/* Se envian los datos */
		ret = Enviar(clientfd, buffer, aux + nbytes);
		if (ret == ERR) {
			syslog(LOG_ERR, "Se ha producido un error al enviar los datos");
			return ERR;
		}

	} while(total_bytes < (ssize_t) length);

	syslog(LOG_INFO, "Se ha terminado de enviar los datos");
	return SUCCESS;
}

void Enviar_Internal_Server_Error(int clientfd, char *server_signature) {
	char *mensaje = "<html> <font color=\"#E0E0E0\">"
					"<body style=\"background-color:#151515;font-family:courier\">"
					"</br></br></br></br></br></br></br>"
					"<h1 style=\"text-align:center\">Error 500</h1>"
					"<h2 style=\"text-align:center\">Internal Server Error</h2>"
					"<p style=\"text-align:center\">Ha ocurrido un error en el"
					"funcionamiento interno del servidor. Intentelo mas tarde.</p></html>";
    int code = ISE;
    int length;
    char header[MAXHDR];
    char buffer[MAXBUF];

    sprintf(buffer, "HTTP/1.1 %d\r\n", code);
    length = strlen(mensaje);
    /* Se añaden las cabeceras de fecha y el nombre del servidor*/
    Rellenar_Date(header);
    strcat(buffer,header);
    Rellenar_Server(server_signature, header);
    strcat(buffer, header);
    /* Se añade la cabecera del tipo de contenido */
    Rellenar_Content_Type("texto.html", header);
    strcat(buffer, header);
    Rellenar_Content_Length(NULL, header, length);
    strcat(buffer, header);
    strcat(buffer, "\r\n");
    strcat(buffer, mensaje);

    Enviar(clientfd, buffer, strlen(buffer));
    return;
}

void Enviar_Bad_Request(int clientfd, char *server_signature) {
    char *mensaje = "<html> <font color=\"#E0E0E0\">"
					"<body style=\"background-color:#151515;font-family:courier\">"
					"</br></br></br></br></br></br></br>"
					"<h1 style=\"text-align:center\">Error 400</h1>"
					"<h2 style=\"text-align:center\">Bad Request</h2>"
					"<p style=\"text-align:center\">Su peticion no tiene el formato adecuado.</p></html>";
    int code = BAD_REQUEST;
    int length;
    char header[MAXHDR];
    char buffer[MAXBUF];

    sprintf(buffer, "HTTP/1.1 %d\r\n", code);
    length = strlen(mensaje);
    /* Se añaden las cabeceras de fecha y el nombre del servidor*/
    Rellenar_Date(header);
    strcat(buffer,header);
    Rellenar_Server(server_signature, header);
    strcat(buffer, header);
    /* Se añade la cabecera del tipo de contenido */
    Rellenar_Content_Type("texto.html", header);
    strcat(buffer, header);
    Rellenar_Content_Length(NULL, header, length);
    strcat(buffer, header);
    strcat(buffer, "\r\n");
    strcat(buffer, mensaje);
    Enviar(clientfd, buffer, strlen(buffer));
    return;
}

void Enviar_Not_Found(int clientfd, char *server_signature) {
    char *mensaje = "<html> <font color=\"#E0E0E0\">"
					"<body style=\"background-color:#151515;font-family:courier\">"
					"</br></br></br></br></br></br></br>"
					"<h1 style=\"text-align:center\">Error 404</h1>"
					"<h2 style=\"text-align:center\">Not Found</h2>"
					"<p style=\"text-align:center\">Archivo no encontrado en el servidor.</p></html>";
    int code = NOT_FOUND;
    int length;
    char header[MAXHDR];
    char buffer[MAXBUF];

    sprintf(buffer, "HTTP/1.1 %d\r\n", code);
    length = strlen(mensaje);
    /* Se añaden las cabeceras de fecha y el nombre del servidor*/
    Rellenar_Date(header);
    strcat(buffer,header);
    Rellenar_Server(server_signature, header);
    strcat(buffer, header);
    /* Se añade la cabecera del tipo de contenido */
    Rellenar_Content_Type("texto.html", header);
    strcat(buffer, header);
    Rellenar_Content_Length(NULL, header, length);
    strcat(buffer, header);
    strcat(buffer, "\r\n");
    strcat(buffer, mensaje);
    syslog(LOG_INFO, "NOT_FOUND: %s", buffer);
    int ret;
    ret = Enviar(clientfd, buffer, strlen(buffer));
    syslog(LOG_INFO, "NOT_FOUND2: %d", ret);
    return;
}

int Procesar_Script(int clientfd, char *command, char *file_path, char *server_signature, char *input, pthread_mutex_t *sem_pipe) {
	int pipe_in[2], pipe_out[2], ret, cabecera_length;
	char buffer[MAXBUF];
	char paquete[MAXBUF];
	pid_t childpid;




	//memset(buffer, 0, MAXBUF);
	//memset(paquete, 0, MAXBUF);

	if(pipe(pipe_out) < 0) {
		Enviar_Internal_Server_Error(clientfd, server_signature);
		syslog(LOG_ERR, "Error al crear el pipe del output: %s", strerror(errno));
		return ERR;
	}
	if (input != NULL) {
		if(pipe(pipe_in) < 0) {
			Enviar_Internal_Server_Error(clientfd, server_signature);
			syslog(LOG_ERR, "Error al crear el pipe del input %s", strerror(errno));
			return ERR;
		}
	}

	pthread_mutex_lock(sem_pipe);
	childpid = fork();
	if (childpid == -1) {
		pthread_mutex_unlock(sem_pipe);
		Enviar_Internal_Server_Error(clientfd, server_signature);
		syslog(LOG_ERR, "Error al crear el hijo %s", strerror(errno));
		return ERR;
	}

	else if(childpid == 0) { /* Hijo */
		syslog(LOG_ERR, "syslog antes del closse %s", strerror(errno));

		close(pipe_out[0]); /* El hijo no lee por la del output, solo escribe*/

		if (input != NULL) {
			close(pipe_in[1]); /* El hijo no escribe por la del input, solo lee*/
			dup2(pipe_in[0], 0); /* Redirigimos la pipe al stdin*/
		}

		dup2(pipe_out[1], 1);  /* Redirigimos STDOUT a la pipe*/
        syslog(LOG_INFO, "Se ejecuta %s", command);

		char * paramsList[] = {"/bin/bash", "-c", command ,NULL};

		ret = execv("/bin/bash", paramsList);
		if (ret == ERR) {
			syslog(LOG_INFO, "execv ha fallado %d", ret);
			pthread_mutex_unlock(sem_pipe);
			Enviar_Internal_Server_Error(clientfd, server_signature);
			return SUCCESS;
		}
	}


	else { /* Padre */

		close(pipe_out[1]);

		if(input != NULL) {
			close(pipe_in[0]);
			write(pipe_in[1], input, strlen(input));
		}
		close(pipe_in[1]);
		wait(NULL);

		ret = read(pipe_out[0], buffer, MAXBUF);
		if (ret == ERR) {
			pthread_mutex_unlock(sem_pipe);
			Enviar_Internal_Server_Error(clientfd, server_signature);
			return SUCCESS;
		}
		pthread_mutex_unlock(sem_pipe);
		syslog(LOG_INFO, "Lectura de la pipe %d", ret);
		syslog(LOG_INFO, "A ver que pasa aqui %s", buffer);

		cabecera_length = Crear_Cabecera(200, server_signature, file_path, NULL, paquete, ret);
		syslog(LOG_INFO, "Cabecera long: %d",cabecera_length);

		strcat(paquete, buffer);
		syslog(LOG_INFO, "Paquete : %s", paquete);
		syslog(LOG_INFO, "Long paquete: %lu", strlen(paquete));
		ret = Enviar(clientfd, paquete, strlen(paquete));
		syslog(LOG_INFO, "%d", ret);
		if(ret == ERR) {
			return ERR;
		}
	}

	close(0);
	close(1);
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	//dup2(1, 0);
	//dup2(0, 0);
	return SUCCESS;

}
int Procesar_GET(int clientfd, char *file_path, char *server_signature, pthread_mutex_t *sem_pipe) {
	FILE *fp = NULL;
	int code, is_script	, check;
	int length;
	char buffer[MAXBUF], command[MAXBUF];

    syslog(LOG_INFO, "Comprobando tipo de petición GET.");
    /* Comprobamos si es un script y si lo es formamos la llamada en el formato correcto */
	is_script = Crear_Comando_Script(file_path, command);
 	if(is_script == NOT_SCRIPT) {
        syslog(LOG_INFO, "No es un script. Abriendo fichero en modo lectura: %s", strerror(errno));
        /* Si no es un script, nos preparamos para leer de archivo */
		fp = fopen(file_path, "r");
        if(fp == NULL) {
            syslog(LOG_ERR, "Archivo no encontrado. Enviando NOT FOUND: %s", strerror(errno));
            Enviar_Not_Found(clientfd, server_signature);
            return SUCCESS;
        }
        else {
            syslog(LOG_INFO, "Fichero abierto en modo lectura: %s", strerror(errno));
        }

	}
	if (is_script == SUCCESS){
        syslog(LOG_INFO, "Es un script. Empezamos a procesar script: %s", strerror(errno));
        /* Si es un script, lo procesamos */
		Procesar_Script(clientfd, command, file_path, server_signature, NULL, sem_pipe);
        syslog(LOG_INFO, "Script terminado: %s", strerror(errno));
		return SUCCESS;
	}
	else if(is_script == ERR) {
        syslog(LOG_ERR, "Peticion incorrecta. Enviando BAD REQUEST: %s", strerror(errno));
		Enviar_Bad_Request(clientfd, server_signature);
        return SUCCESS;
	}
	else {
        syslog(LOG_INFO, "Todo correcto. Creando cabeceras: %s", strerror(errno));
		code = OK;
	}
    /* Creamos las cabeceras. Si falla, enviamos internal server error */
	length = Crear_Cabecera(code, server_signature, file_path, fp, buffer, -1);
	if(length == ERR) {
        syslog(LOG_ERR, "Error al crear las cabeceras. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
		return SUCCESS;
	}
    syslog(LOG_INFO, "Envio de cabeceras terminado. Enviando respuesta: %s", strerror(errno));

    /* Enviamos la respuesta */
	check = Enviar_Respuesta(clientfd, buffer, length, fp);
    /* Si falla enviamos Internal Server Error */
	if(check == ERR) {
        syslog(LOG_ERR, "Error al enviar la respuesta. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
        return SUCCESS;
	}

    syslog(LOG_INFO, "Respuesta enviada: %s", strerror(errno));
    /* Cerramos el fichero*/
    fclose(fp);

    syslog(LOG_INFO, "Terminando procesar GET: %s", strerror(errno));
	return SUCCESS;

}

int Procesar_POST(int clientfd, char *file_path, char *server_signature, char *content, pthread_mutex_t *sem_pipe) {

	int is_script, code, bytes_used = 0;
	int nbytes;
	int length;
	int check;
	char buffer[MAXBUF];
	char command[MAXBUF];

	FILE *fp = NULL;

    syslog(LOG_INFO, "Comprobando tipo de petición POST.");
    /* Comprobamos si es un script y si lo es formamos la llamada en el formato correcto */
	is_script = Crear_Comando_Script(file_path, command);
	if(is_script == NOT_SCRIPT) {
        syslog(LOG_INFO, "No es un script. Abriendo fichero en modo escritura: %s", strerror(errno));
        /* Si no es un script, nos preparamos para escribir en archivo */
		fp = fopen(file_path, "w");
        if(fp == NULL) {
            syslog(LOG_ERR, "Error al abrir el archivo. Enviando ISE: %s", strerror(errno));
            Enviar_Internal_Server_Error(clientfd, server_signature);
            return SUCCESS;
        }
        else {
            syslog(LOG_INFO, "Fichero abierto en modo escritura: %s", strerror(errno));
        }
	}
	else if(is_script == SUCCESS) {
        syslog(LOG_INFO, "Es un script. Empezamos a procesar script: %s", strerror(errno));
        Procesar_Script(clientfd, command, file_path, server_signature, content, sem_pipe);
        syslog(LOG_INFO, "Script terminado: %s", strerror(errno));
        return SUCCESS;
	}

	else if(is_script == ERR) {
        syslog(LOG_ERR, "Peticion incorrecta. Enviando BAD REQUEST: %s", strerror(errno));
		Enviar_Bad_Request(clientfd, server_signature);
        return SUCCESS;
	}
	else {
        syslog(LOG_INFO, "Todo correcto. Creando cabeceras: %s", strerror(errno));
		code = OK;
	}

    /* Escribimos en el path especificado en la peticion */
    syslog(LOG_INFO, "Comenzando la escritura: %s", strerror(errno));
	do {
        /* Escribimos en archivo en contenido del body */
		nbytes = fwrite(content + bytes_used, sizeof(char), MAXBUF , fp);
		if (nbytes < 0) {
            syslog(LOG_ERR, "Error al escribir. Enviando ISE: %s", strerror(errno));
			Enviar_Internal_Server_Error(clientfd, server_signature);
            return SUCCESS;
		}

        syslog(LOG_INFO, "Fragmento escrito: %s", strerror(errno));
		bytes_used += nbytes;

		if (nbytes == 0) {
            /* Cuando dejamos de escribir bytes hemos terminado */
			break;
		}
	} while(bytes_used < strlen(content));
    syslog(LOG_INFO, "Escritura terminada: %s", strerror(errno));

    /* Creamos las cabeceras. Si falla, enviamos internal server error */
    length = Crear_Cabecera(code, server_signature, file_path, fp, buffer, -1);
    if(length == ERR) {
        syslog(LOG_ERR, "Error al crear las cabeceras. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
        return SUCCESS;
    }
    syslog(LOG_INFO, "Envio de cabeceras terminado. Enviando respuesta: %s", strerror(errno));

    /* Enviamos la respuesta */
    check = Enviar_Respuesta(clientfd, buffer, length, fp);
    /* Si falla enviamos Internal Server Error */
    if(check == ERR) {
        syslog(LOG_ERR, "Error al enviar la respuesta. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
        return SUCCESS;
    }

    syslog(LOG_INFO, "Respuesta enviada: %s", strerror(errno));
    /* Cerramos el fichero*/
    fclose(fp);

    syslog(LOG_INFO, "Terminando procesar POST: %s", strerror(errno));
    return SUCCESS;
}

int Procesar_OPTIONS(int clientfd, char *server_signature) {

	char buffer[MAXBUF];
	int code, length, check;

	code = OK;

    /* Creamos las cabeceras. Si falla, enviamos internal server error */
    length = Crear_Cabecera(code, server_signature, NULL, NULL, buffer, -1);
    if(length != 0) {
        syslog(LOG_ERR, "Error al crear las cabeceras. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
        return SUCCESS;
    }
    syslog(LOG_INFO, "Envio de cabeceras terminado. Enviando respuesta: %s", strerror(errno));

    /* Enviamos la respuesta */
    check = Enviar_Respuesta(clientfd, buffer, 0, NULL);
    /* Si falla enviamos Internal Server Error */
    if(check == ERR) {
        syslog(LOG_ERR, "Error al enviar la respuesta. Enviando ISE: %s", strerror(errno));
        Enviar_Internal_Server_Error(clientfd, server_signature);
        return SUCCESS;
    }

    syslog(LOG_INFO, "Respuesta enviada: %s", strerror(errno));

    syslog(LOG_INFO, "Terminando procesar OPTIONS: %s", strerror(errno));
    return SUCCESS;
}

//Cambiar cosas y mirar cosas y entender cosas
int Procesar_Peticion(int clientfd, char *server_root, char *server_signature, pthread_mutex_t *sem_pipe) {

	char buffer[MAXBUF];
	char true_path[MAXBUF];
	char true_method[MAXBUF];
	char file_path[MAXBUF];
	const char *method, *path;
	int pret, minor_version;
	struct phr_header headers[100];
	size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
	ssize_t ret;

	int persistencia = 0;
	int i;

    syslog(LOG_INFO, "Comienza la lectura de la petición");
	while(1) {
		/* Se lee la peticion del cliente */
		ret = Leer(clientfd, buffer + buflen, sizeof(buffer) - buflen);

		syslog(LOG_ERR, "%s", buffer);

        /* Error al leer la petición */
		if(ret < 0) {
			syslog(LOG_INFO, "Error al leer la petición: %s", strerror(errno));
			Enviar_Internal_Server_Error(clientfd, server_signature);
			return 0;
		}

		if (ret == 0) {
			return 0;
		}

		prevbuflen = buflen;
		buflen += ret;
		/* Parseamos la petición */
		num_headers = sizeof(headers) / sizeof(headers[0]);
		pret = phr_parse_request(buffer, buflen, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, prevbuflen);

        /* La petición se ha parseado entera */

        if (pret > 0) {
        	break;
        }
        /* Ha ocurrido un error al parsear */
        else if (pret != -2) {
            syslog(LOG_INFO, "Error al parsear la petición: %s", strerror(errno));
            Enviar_Bad_Request(clientfd, server_signature);
    		return 0;
    	}

		/* Si pret = -2 la petición esta incompleta y se sigue parseando */
        /* La petición es demasiado grande */
		if (buflen == sizeof(buffer)) {
			Enviar_Bad_Request(clientfd, server_signature);
			return 0;
		}

	}

	char buf_aux[128];

	memset(buf_aux, 0, 128);

	persistencia = (short)minor_version;

	for (i = 0; i < num_headers; i++) {
		memcpy(buf_aux, headers[i].name, (int)headers[i].name_len);


		if (!strcmp(buf_aux, "Connection")) {
			syslog(LOG_INFO, "%s", buf_aux);
			break;
		}

		memset(buf_aux, 0, 128);
	}

	memset(buf_aux, 0, 128);
	memcpy(buf_aux, headers[i].value, (int)headers[i].value_len);
	if (!strcmp(buf_aux, "close")) {
		syslog(LOG_INFO, "%s", buf_aux);
		persistencia = 0;
	}

	else if (!strcmp(buf_aux, "keep-alive")) {
		syslog(LOG_INFO, "%s", buf_aux);
		persistencia = 1;
	}


    syslog(LOG_INFO, "Petición parseada correctamente: %s", strerror(errno));
	/* Se separan las cadenas devueltas por phr_parse_requiest */
	sprintf(true_method, "%.*s", (int)method_len, method);
	sprintf(true_path, "%.*s", (int)path_len, path);

	syslog(LOG_INFO, "%s\n",true_method);
	/* Se copia a una variable local el argumento server_root*/
	strcpy(file_path, server_root);

	/* Si en el request no se indica ningun archivo se devuelve el index */
	if (strcmp(true_path, "/")==0 ) {
		strcat(file_path, "/index.html");
	}

	else {
		strcat(file_path, true_path);
	}

	/* Si el metodo es Get */
	if(strcmp(true_method, "GET") == 0) {
        syslog(LOG_INFO, "Comienzo de procesar petición GET: %s", strerror(errno));
		Procesar_GET(clientfd, file_path, server_signature, sem_pipe);
	}

	/* SI el metodo es POST*/
	else if(strcmp(true_method, "POST") == 0) {
        syslog(LOG_INFO, "Comienzo de procesar petición POST: %s", strerror(errno));
		Procesar_POST(clientfd, file_path, server_signature, buffer+pret, sem_pipe);
	}

	/*Si el metodo es OPTIONS*/
	else if(strcmp(true_method, "OPTIONS") == 0){
        syslog(LOG_INFO, "Comienzo de procesar petición OPTIONS: %s", strerror(errno));
		Procesar_OPTIONS(clientfd, server_signature);
	}

	/*BAD REQUEST*/
	else {
        syslog(LOG_INFO, "Metodo desconocido. Enviando BAD REQUEST: %s", strerror(errno));
        Enviar_Bad_Request(clientfd, server_signature);
	}
	return persistencia;
}
