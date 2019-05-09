#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "../includes/respuesta.h"

#define HEADER_MAX 256

/************************************************************************/
/* Nombre: Rellenar_Date                                                */
/* Descripcion: Crea el campo Date de la cabecera HTTP                  */
/*                                                                      */
/* Entrada:                                                             */
/*              date_header: cadena de caracteres donde almacenar Date  */
/*                                                                      */
/* Salida:                                                              */
/*              ninguna                                                 */
/*                                                                      */
/************************************************************************/
void Rellenar_Date(char* date_header) {
    char s[64];

    /* Obtenemos la hora local */
    time_t t = time(NULL);
    struct tm *ptm = gmtime(&t);

    /* Guardamos la fecha en una cadena en el formato adecuado */
    strftime(s, 64, "%a, %d %b %Y %H:%M:%S GMT", ptm);
    /* Creamos la cabecera Date: *fecha y hora* */
    strcpy(date_header, "Date: ");
    strcat(date_header, s);
    strcat(date_header, "\r\n");
}

/********************************************************************/
/* Nombre: Rellenar_Server                                                                                  */
/* Descripcion: Crea el campo Server de la cabecera HTTP                        */
/*                                                                                                                                  */
/* Entrada:                                                                                                                 */
/*              server : firma del servidor                                                             */
/*              date_header: Cadena de caracteres donde almacenar Server  */
/*                                                                                                                  */
/* Salida:                                                                                                                  */
/*              ninguna                                                                                                     */
/*                                                                                                                                  */
/********************************************************************/
void Rellenar_Server(char *server, char * server_header) {
    /* Creamos la cabecera Server: *server_signature* */
    strcpy(server_header, "Server: ");
    strcat(server_header, server);
    strcat(server_header, "\r\n");
}

/********************************************************************/
/* Nombre: Rellenar_Last_Modified                                                                       */
/* Descripcion: Crea el campo Last Modified de la cabecera HTTP         */
/*                                                                                                                                  */
/* Entrada:                                                                                                                 */
/*              fp : file pointer del fichero                                                           */
/*              last_modified_header: Cadena de caracteres donde          */
/*             almacenar Last Modified                                                          */
/*                                                                                                                  */
/* Salida:                                                                                                                  */
/*              int:    0 (OK) en caso de ejecuccion correcta                               */
/*                       -1 (ERR) en caso de error                                                      */
/*                                                                                                                                  */
/********************************************************************/
int Rellenar_Last_Modified(FILE *fp, char *last_modified_header) {
    struct stat file_stats;
    struct tm *time;
    char s[64];

    /* Control de parámetros de entrada */
    if(fp== NULL) {
        return -1;
    }

    /* Si falla fstat devolvemos error */
    if (fstat(fileno(fp), &file_stats) == -1) {
        return -1;
    }

    /* Obtenemos de fstat cuando fue modificado por ultima vez */
    time = gmtime(&file_stats.st_mtime);
    /* Guardamos la fecha en una cadena en el formato adecuado */
    strftime(s, 64, "%a, %d %b %Y %H:%M:%S GMT", time);

    /* Creamos la cabecera Last Modified: *fecha y hora* */
    strcpy(last_modified_header, "Last-Modified: ");
    strcat(last_modified_header, s);
    strcat(last_modified_header, "\r\n");

    return 0;
}

/********************************************************************/
/* Nombre: Rellenar_Content_Length                                                                  */
/* Descripcion: Crea el campo Content Length de la cabecera HTTP        */
/*                                                                                                                                  */
/* Entrada:                                                                                                                 */
/*              fp : file pointer del fichero                                                           */
/*              content_length_header: Cadena de caracteres donde         */
/*             almacenar Content Length                                                         */
/*        longitud : en caso de script, longitud del contenido.         */
/*                                   en otro caso, -1                                                           */
/* Salida:                                                                                                                  */
/*              int:    length  en caso de ejecuccion correcta                          */
/*                       -1 (ERR) en caso de error                                                      */
/*                                                                                                                                  */
/********************************************************************/
int Rellenar_Content_Length(FILE *fp, char *content_length_header, int longitud) {
    struct stat file_stats;
    char n_bytes[32];
    int length;

    /* Si el campo longitud es un numero natural, rellenamos Content Length con ese numero*/
    if(longitud >= 0) {
        length = longitud;
    }
    
    else {
        /* Sino, leemos del fichero la longitud de su contenido */
        /* Control de parámetros de entrada */
        if (fp == NULL) {
            return -1;
        }
        /* Si falla fstat devolvemos error */
        if (fstat(fileno(fp), &file_stats) == -1) {
            return -1;
        }
        /* Leemos la longitud del contenido del fichero */
        length = (int)file_stats.st_size;
    }

    /* Creamos la cabecera Content Length: *tamanio* */
    sprintf(n_bytes, "%lld", (long long) length);
    strcpy(content_length_header, "Content-Length: ");
    strcat(content_length_header, n_bytes);
    strcat(content_length_header, "\r\n");

    /* Si todo ha ido bien, devolvemos la longitud escrita en la cabecera */
    return length;
}

/********************************************************************/
/* Nombre: Rellenar_Content_Type                                                                        */
/* Descripcion: Crea el campo Content Type de la cabecera HTTP          */
/*                                                                                                                                  */
/* Entrada:                                                                                                                 */
/*              file_name : nombre del fichero                                                      */
/*              content_type_header: Cadena de caracteres donde           */
/*             almacenar Content Type                                                           */
/*                                                                                          */
/* Salida:                                                                                                                  */
/*              ninguna                                                                             */
/*                                                                                                                                  */
/********************************************************************/
void Rellenar_Content_Type(char *file_name, char *content_type_header) {
    char *ext;
    char name_aux[HEADER_MAX];

    strcpy(content_type_header, "Content-Type: ");

    /* Dividimos el nombre del fichero en nombre y extensión */
    strcpy(name_aux, file_name);
    strtok(name_aux, ".");

    /* Extensión del fichero */
    ext = strtok(NULL, ".");
    /* Si es .txt */
    if (strcmp(ext, "txt") == 0) {
        strcat(content_type_header, "text/plain");
    }
    /* Si es .html o .htm */
    else if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) {
        strcat(content_type_header, "text/html");
    }
    /* Si es .gif */
    else if (strcmp(ext, "gif") == 0) {
        strcat(content_type_header, "image/gif");
    }
    /* Si es .jpeg o .jpg */
    else if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) {
        strcat(content_type_header, "image/jpeg");
    }
    /* Si es .mpeg o .mpg */
    else if (strcmp(ext, "mpeg") == 0 || strcmp(ext, "mpg") == 0) {
        strcat(content_type_header, "video/mpeg");
    }
    /* Si es .doc o .docx */
    else if (strcmp(ext, "doc") == 0 || strcmp(ext, "docx") == 0) {
        strcat(content_type_header, "application/msword");
    }
    /* Si es .pdf */
    else if (strcmp(ext, "pdf") == 0) {
        strcat(content_type_header, "application/pdf");
    }
    /* Si no es ninguno de los anteriores, es un script cuyo contenido es html */
    else {
        strcat(content_type_header, "text/html");
    }
    strcat(content_type_header, "\r\n");
}

/********************************************************************/
/* Nombre: Rellenar_Allow                                                                               */
/* Descripcion: Crea el campo Allow de la cabecera HTTP, en caso    */
/*                    de peticion OPTIONS                                                                   */
/* Entrada:                                                                                                                 */
/*              allow_header: Cadena de caracteres donde almacenar Allow  */
/*                                                                                          */
/* Salida:                                                                                                                  */
/*              ninguna                                                                             */
/*                                                                                                                                  */
/********************************************************************/
void Rellenar_Allow(char *allow_header) {
    /* Creamos la cabecera Allow: *metodos permitidos* */
    strcat(allow_header, "Allow: GET, POST, OPTIONS");
}
