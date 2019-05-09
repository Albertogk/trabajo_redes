#ifndef RESPUESTA_H_
#define RESPUESTA_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "../includes/respuesta.h"

#define HEADER_MAX 256

void Rellenar_Date(char * date_header);

void Rellenar_Allow(char * date_header);

void Rellenar_Server(char *server, char *server_header);

int Rellenar_Last_Modified(FILE *fp, char *last_modified_header);

int Rellenar_Content_Length(FILE *fp, char *content_length_header, int longitud);

void Rellenar_Content_Type(char *file_name, char * content_type_header);

#endif
