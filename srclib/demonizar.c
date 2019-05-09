#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>


#define OK 0
#define ERR -1
#define BUFF_SIZE 20

/********************************************************************/
/* Nombre: Demonizar																								*/
/* Descripcion: Pasa la ejecución del proceso a segundo plano,			*/
/* desacoplandolo de la terminal																		*/
/*																																	*/
/* Entrada:																													*/
/*				ninguna																										*/
/*																								                	*/
/* Salida:																													*/
/* 				int:	0 (OK) en caso de ejecuccion correcta								*/
/*		 				 -1 (ERR) en caso de error														*/
/*																																	*/
/********************************************************************/
int Demonizar() {
	int i;
	char fd_path[BUFF_SIZE];

	int pid = 0;
	int sid = 0;
	int max_files;

	/*Creamos el hijo*/
	pid = fork();

	/*Terminamos la ejecución si ha fallado el fork*/
	if (pid == ERR) {
		syslog(LOG_ERR, "%s", strerror(errno));
		return ERR;
	}

	/* Ejecuccion del padre */
	if (pid != 0) {
		/*Terminamos su ejecución*/
		exit(OK);
	}

	/* Ejecuccion del hijo */
	else {
		/* Creacion de la nueva sesion de procesos, el proceso creador (hijo) es el lider */
		sid = setsid();

		/*Terminamos la ejecución si ha fallado el setsid*/
		if (sid == ERR) {
			syslog(LOG_ERR, "%s", strerror(errno));
			return ERR;
		}

		/* Modificamos los permisos de ficheros dando todos los posibles (mask a 0)*/
		umask(0);

		/*Obtenemos el numero máximo de descriptores de ficheros abiertos a la vez*/
		max_files = getdtablesize();

		/*Terminamos la ejecución si ha fallado el getdtablesize*/
		if (max_files == ERR) {
			syslog(LOG_ERR, "%s", strerror(errno));
			return ERR;
		}

		/*Guardamos en fd_path el path hasta el directorio con los descriptores*/
		sprintf(fd_path, "/proc/%d/fd/", getpid());

		/*Nos movemos hasta el directorio con los descriptores*/
		chdir(fd_path);
		for (i = 0; i < 3; i++) {
			/*Cerramos todos los descriptores que puedan estar abiertos*/
			/*Mirar que esto da error*/
			close(i);
		}

		
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_RDWR);
		open("/dev/null", O_RDWR);

		/*Nos volvemos a mover al directorio raiz*/
		chdir("/");

		/*Abrimos el log (NULL para que coja el nombre del programa,
		LOG_PID para que indice el id del proceso (quizas util, podemos añadir modos),
		LOG_DAEMON para indicar que el programa es un demonio)*/
		openlog (NULL, LOG_PID, LOG_DAEMON);
		// errno = 0; //ponemos esto porque lo de close ha dado error
		return OK;
	}
}
