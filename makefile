CC = gcc
CFLAGS = -g -Wall
EXECUTABLES = server
LIBS = -lpthread
####################################################################
OBJECT_PATH = ./objects
SRCLIB_PATH = ./srclib
SRC_PATH = ./src
INCLUDE_PATH = ./includes
LIB_PATH = ./lib
####################################################################
OBJECTS_SERVER = $(OBJECT_PATH)/respuesta.o $(OBJECT_PATH)/conexion.o $(OBJECT_PATH)/demonizar.o $(OBJECT_PATH)/server.o
####################################################################

.PHONY: all
all: objects  $(EXECUTABLES)

objects: $(shell mkdir -p objects)

$(OBJECT_PATH)/respuesta.o: $(SRCLIB_PATH)/respuesta.c $(INCLUDE_PATH)/respuesta.h
		$(CC) $(CFLAGS) -c -o $(OBJECT_PATH)/respuesta.o $(SRCLIB_PATH)/respuesta.c

$(OBJECT_PATH)/demonizar.o: $(SRCLIB_PATH)/demonizar.c $(INCLUDE_PATH)/demonizar.h
		$(CC) $(CFLAGS) -c -o $(OBJECT_PATH)/demonizar.o $(SRCLIB_PATH)/demonizar.c 


$(OBJECT_PATH)/conexion.o: $(SRCLIB_PATH)/conexion.c $(INCLUDE_PATH)/conexion.h $(INCLUDE_PATH)/respuesta.h
		$(CC) $(CFLAGS) -c -o $(OBJECT_PATH)/conexion.o $(SRCLIB_PATH)/conexion.c 


$(OBJECT_PATH)/server.o: $(SRC_PATH)/server.c $(INCLUDE_PATH)/conexion.h
		$(CC) $(CFLAGS) -c -o $(OBJECT_PATH)/server.o $(SRC_PATH)/server.c

server: $(OBJECTS_SERVER)
	 $(CC) $(CFLAGS) -o server $(OBJECTS_SERVER) $(LIBS) $(LIB_PATH)/libinih.a $(LIB_PATH)/libpicohttpparser.a

.PHONY: clear
clear:
			rm -rf ./objects

.PHONY: clean
clean: clear
			rm -rf server
