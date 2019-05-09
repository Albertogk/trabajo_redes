#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "../includes/conexion.h"
#include "../includes/demonizar.h"
#include <pthread.h>
#include "../includes/ini.h"
#include <signal.h>

#define MAXBUF 1024
#define CONF_OPT 4
#define ERR -1

int flag = 1;

/*Mirar lo que heredan los hijos*/
void handle_sigint(int sig)
{
    printf("algo\n");
    flag = 0;
}

void handle_sigalrm(int sig)
{
    pthread_exit(NULL);
}


int num_threads = 0;


/* Estructura usada en la configuracion del servidor a partir del archivo */
typedef struct
{
    char server_root[MAXBUF];
    int max_clients;
    int max_threads;
    int listen_port;
    char server_signature[MAXBUF];

} configuration;

/* Estructura de los argumentos de los hilos*/
struct thread_args {
    int sockfd;

    char server_root[MAXBUF];
    char server_signature[MAXBUF];

    pthread_mutex_t *sem;
    pthread_mutex_t *sem_pipe;
};


/* Funcion usada para parsear el fichero de configuracion*/
static int handler(void* server, const char* section, const char* taget, const char* value)
{
    configuration* pconfig = (configuration*)server;

    #define MATCH(s) strcmp(taget, s)==0

    if (MATCH("server_root")) {
        strcpy(pconfig->server_root, value); // malloc

    } else if (MATCH("max_clients")) {
        pconfig->max_clients = atoi(value);

    } else if (MATCH("max_threads")) {
        pconfig->max_threads = atoi(value);

    } else if (MATCH("listen_port")) {
        pconfig->listen_port = atoi(value);

    } else if (MATCH("server_signature")) {
        strcpy(pconfig->server_signature, value); //malloc

    } else {
        return 0;  //Mirar a ver si se puede poner -1, 0
    }

    return 1;
}


/* Funcion que parsea el fichero de configuracion */
int conf(char *file_name, configuration* config) {

    if (ini_parse(file_name, handler, config) < 0) {
        printf("Can't load %s\n", file_name);
        return -1;
    }

    syslog(LOG_INFO, "Config loaded from %s:\n server_root = '%s'\n max_clients = %d\n max_threads = %d\n listen_port = %d\n server_signature = '%s'\n\n", file_name, config->server_root, config->max_clients, config->max_threads, config->listen_port, config->server_signature);

    return 0;
}

/* La funcion que ejecutan los hilos   */
/* Se encarga de procesar la peticion  */
void * thread_function(void *args) {

    int clientfd;
    struct sockaddr_in client_addr;
    int persistente = 1;

    //struct timeval tv;
    int sockfd = ((struct thread_args *)args)->sockfd;


    pthread_mutex_t *sem = ((struct thread_args *)args)->sem;
    pthread_mutex_t *sem_pipe = ((struct thread_args *)args)->sem_pipe;
    //tv.tv_sec = 30;
    //tv.tv_usec = 0;

    while(flag) {

        /* Se protege con semaforo aceptar conexion para evitar posibles conflictos */
        pthread_mutex_lock(sem);

        /* Se acepta una conexion de un cliente */
        clientfd = Aceptar_Conexion(sockfd, &client_addr);

        if (clientfd < 0) {
            syslog(LOG_ERR, "Error al aceptar conexion: %s", strerror(errno));
            close(sockfd); /*mandar una señal al resto de los hilos*/
            return NULL;
        }

        //setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        syslog(LOG_INFO, "Conexion aceptada: %s", strerror(errno)); //eso dentro del semaforo no es muy eficiente

        pthread_mutex_unlock(sem);
        /* Se acaba la zona critica */

        syslog(LOG_INFO, "Empezando a procesar peticion: %s", strerror(errno));

        while(persistente) {
            /* Se empieza a procesar la peticion */
            persistente = Procesar_Peticion(clientfd, ((struct thread_args *)args)->server_root, ((struct thread_args *)args)->server_signature, sem_pipe);
        }

        syslog(LOG_ERR, "Peticion procesada: %s", strerror(errno));
        persistente = 1;
        /* Se cierra el descriptor de cliente, si no se cierra, va mal*/
        close(clientfd);
        syslog(LOG_ERR, "Descrptor de cliente cerrado: %s", strerror(errno));
    }

    return NULL;
}

int main() {
    int ret;
    int i;
    int sockfd;

    char cwd[MAXBUF];

    pthread_t *threads;
    struct thread_args targs;

    configuration config;

    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_sigalrm);
    /* Se obtienen los datos del arvhivo de configuracion*/
    ret = conf("server.conf", &config);

    /* Se obtiene la direccion absoluta de los archivos de la pagina web*/
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, config.server_root);

    pthread_mutex_t sem;
    pthread_mutex_t sem_pipe;

    /* Se demoniza el proceso, todos los mensajes de error o de informacion se escriben en syslog */
    if (Demonizar() < 0) {
        syslog(LOG_INFO, "Error en demonizar: %s", strerror(errno));
        exit(ERR);
    }

    syslog(LOG_INFO, "Demonizar completado: %s", strerror(errno));

    /* Se resetea el errno a 0 */
    errno = 0;

    /* Se abre el socket tipo TCP (socket) */
    if ((sockfd = Abrir_Socket()) < 0) {
        syslog(LOG_ERR, "Error en la creación del socket: %s", strerror(errno));
        exit(ERR);
    }

    syslog(LOG_INFO, "Socket abierto: %s", strerror(errno));

    /* Se enlaza un puerto al socket (bind) */
    if (Enlazar(sockfd, config.listen_port) != 0)   {
        syslog(LOG_ERR, "Error al enlazar: %s", strerror(errno));
        close(sockfd);
        exit(ERR);
    }

    syslog(LOG_INFO, "Socket enlazado al puerto %d: %s", config.listen_port, strerror(errno));

    /* Se empieza a escuchar el el socket con el tamaño de la cola max_clients*/
    if (Escuchar(sockfd, config.max_clients) != 0) {
        syslog(LOG_ERR, "listen %s", strerror(errno));
        close(sockfd);
        exit(ERR);
    }

    syslog(LOG_INFO, "Escuchando en el puerto %d: %s", config.listen_port, strerror(errno));

    /* Se crea el semaforo para los hilos */
    if (pthread_mutex_init(&sem, NULL) != 0) {
        syslog(LOG_ERR, "Error al crear el semaforo %s", strerror(errno));
        close(sockfd);
        exit(ERR);
    }

     if (pthread_mutex_init(&sem_pipe, NULL) != 0) {
        syslog(LOG_ERR, "Error al crear el semaforo %s", strerror(errno));
        close(sockfd);
        exit(ERR);
    }

    syslog(LOG_INFO, "Semaforo creado: %s", strerror(errno));

    /* Se crea el array de hilos de tamanio max_clients */
    threads = (pthread_t *)malloc(config.max_threads * sizeof(pthread_t));
    if (threads == NULL) {
        syslog(LOG_ERR, "Error en la creacion de array de hilos: %s", strerror(errno));
        close(sockfd);
        exit(ERR);
    }

    syslog(LOG_INFO, "Array de hilos creado: %s", strerror(errno));


    /* Se crean los hilos para procesar las peticiones */
    targs.sockfd = sockfd;
    strcpy(targs.server_root, cwd);
    syslog(LOG_INFO, "Ruta del servidor: %s", targs.server_root);
    strcpy(targs.server_signature, config.server_signature);
    targs.sem = &sem;
    targs.sem_pipe = &sem_pipe;

    for (i = 0; i < config.max_threads; i++) {

        ret = pthread_create(&threads[i], NULL, thread_function, (void *) &targs);
        if (ret < 0) {
            syslog(LOG_ERR, "Error a la hora crear hilos: %s", strerror(errno));
            close(sockfd);
            exit(ERR);
        }

        /* Indicamos que el sistema libere los recursos de un hilo en cuanto este acabe */
        //ret = pthread_detach(threads[i]);
        if (ret != 0) {
            syslog(LOG_ERR, "AAAA %s", strerror(errno));
            close(sockfd);
            exit(ERR);
        }
    }

    syslog(LOG_INFO, "Creados todos los hilos %s", strerror(errno));

    /* Se pausa el programa principal ya que son los hilos los que se encargan de procesar peticiones */
    pause();
    syslog(LOG_INFO, "Salido de pause, preparandose para liberar recursos");
    for (i = 0; i < config.max_threads; i++) {
        pthread_kill(threads[i], SIGALRM);
        pthread_join(threads[i], NULL);
        syslog(LOG_INFO, "Hilo %d acabado", i);
    }
    free(threads);

    pthread_mutex_destroy(&sem);
    pthread_mutex_destroy(&sem_pipe);

    shutdown(sockfd, SHUT_RD);

    syslog(LOG_INFO, "Todo acabado");
    exit(0);
}
