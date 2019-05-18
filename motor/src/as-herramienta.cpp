/**
   \mainpage ARGOS - SCADA DE CODIGO ABIERTO
   <br>CINTAL - Centro de Innovaci&oacute;n Tecnol&oacute;gica del Aluminio
   <hr>
   \author Alejandro Pi&ntilde;a - ajpina@gmail.com
   \date Enero-2008

   <br><img src="../../imagenes/logo_argos.png"> - SCADA desarrollado en C/C++ con herramientas de Software Libre

*/

/**
* @file	as-herramienta.cpp
* @brief		<b>Utilidad para manejo de Tags y Alarmas</b><br>
* @author	    Alejandro Pina <ajpina@gmail.com><br>
* @link			http://www.cintal.com.ve/tecnologia/argos<br>
* @package      argos
* @access       public
* @version      1.0  -  01/06/09
*/

/*
Copyright (C) 2006 Alejandro Pina <ajpina@gmail.com>

Este programa es software libre. Puede redistribuirlo y/o modificarlo bajo
los terminos de la Licencia Publica General de GNU segun es publicada por
la Free Software Foundation, bien de la version 3 de dicha Licencia o bien
(segun su eleccion) de cualquier version posterior.

Este programa se distribuye con la esperanza de que sea util, pero
SIN NINGUNA GARANTIA, incluso sin la garantia MERCANTIL implicita o sin
garantizar la CONVENIENCIA PARA UN PROPOSITO PARTICULAR. Vease la Licencia
Publica General de GNU para mas detalles.

Deberia haber recibido una copia de la Licencia Publica General junto con
este programa. Si no ha sido asi, escriba a la Free Software Foundation, Inc.,
en 675 Mass Ave, Cambridge, MA 02139, EEUU.

Alejandro Pina mediante este documento renuncia a cualquier interes de derechos de
copyright con respecto al programa 'argos'.

01 de Noviembre de 2008

Por favor reporte cualquier fallo a la siguiente direccion:

	http://www.cintal.com.ve/tecnologia/argos
*/

/*
===============================================================================
			Informacion de Versión
===============================================================================
	* V 0.7		12/09  	-Brazón, Juan
  									-Castro, Manuel
  									-De Ponte, David
										-Saheli, Mohamed
										-Suarez, Alexis
										-Yegres, Jenifer

  - Opción de Ayuda (Manuel Castro/Jenifer Yegres)
  - Impresión del Listado de todos los Tags almacenados (Jenifer Yegres/Juan Brazón)
  - Impresión del Listado de todas las Alarmas almacenadas (Juan Brazón)
  - Envio de Tag al servicio Escritor (Juan Brazón)
  - Envio de Alarma al servicio Escritor (Juan Brazón)
  - Leer valor de alarma/tag en la memoria compartida ( Mohamed Saheli/Alexis Suarez)
  - Actualizar ( escribir) valor alarma/tag en la memoria compartida (Mohamed Saheli)
  - Consulta de thalarma/thtag (David De ponte/Mohamed Saheli)

===============================================================================
	* V 0.0		11/08		Alejandro Pina
	- Creacion de las primeras herramientas
===============================================================================

*/


#include "as-util.h"
#include "as-tags.h"
#include "as-thtag.h"
#include "as-shmtag.h"
#include "as-alarma.h"
#include "as-thalarma.h"
#include "as-shmalarma.h"
#include "as-shmhistorico.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>




static int	verbose_flag;
string		mensaje_error;

int 		c,
puerto = 16005;

bool 		leer = false,
				actualizar = false,
				insertar = false,
				eliminar = false,
				escribir =  false,
				observar = false,
				consultar = false,
				escribir_shm = false,
				imprimir_tendencia = false;

char		nombre_shm_tags[100] = "\0",
				nombre_shm_alarmas[100] = "\0",
				nombre_tag[_LONG_NOMB_TAG_] = "\0",
				ip_servidor[100] = "127.0.0.1",
				nombre_alarma[_LONG_NOMB_ALARM_] = "\0",
				char_valor_tag[30];

Tvalor		valor_tag;

Tdato		tipo_tag;
Ealarma		estado_alarma;


using namespace std;
using namespace argos;


bool escribir_a_dispositivo();

/* Funciones Grupo 1 */

void help(int);
void mostrar_alarmas_de_bdtr();
void mostrar_tags_de_bdtr();
bool enviar_alarma_a_escritor();
bool enviar_tag_a_escritor();
char *Estado_Alarma_String(int);
char *Tipo_Tag_String(int);

/* Funciones Grupo 2 */
void leer_th_shm();
void consultar_th();
void escribir_th_shm();

void obtener_tendencia_servidor();


int main(int argc, char * argv[]) {

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-herramienta) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

    while (true) {
        static struct option long_options[] = {
            {"verbose", 		no_argument, 		&verbose_flag, 	1},
            {"brief",   		no_argument, 		&verbose_flag, 	0},
            {"leer",		no_argument,		0, 				'l'},
            {"escribir",		no_argument,		0, 				'e'},
            {"actualizar",	no_argument,		0, 				'u'},
            {"consultar",	no_argument,		0, 				'c'},
            {"valor-tag",	required_argument,	0, 				'v'},
            {"mem-tags",		required_argument,	0, 				'g'},
            {"mem-alarmas",	required_argument,	0, 				's'},
            {"tag",		required_argument,	0, 				't'},
            {"alarma",		required_argument,	0, 				'a'},
            {"servidor",		required_argument,	0, 				'i'},
            {"puerto",		required_argument,	0, 				'p'},
            {"tipo-dato",	required_argument,	0, 				'd'},
            {"estado-alarma",    required_argument,	0, 				'm'},
            {"observar",		required_argument,	0, 				'o'},
            {"help",		no_argument,	        0, 				'h'},
            {"imprime-tend",		no_argument,	        0, 				'r'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "leuchrv:g:s:t:a:i:p:d:m:o",long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 0:
            if (long_options[option_index].flag != 0)
                break;
            cout << "opcion " << long_options[option_index].name;
            if (optarg)
                cout << " con arg " << optarg;
            cout << endl;
            break;
        case 'l':
            leer = true;
#ifdef _DEBUG_
            cout <<"Opcion de Lectura"<<endl;
#endif
            break;
        case 'c':
            consultar = true;
#ifdef _DEBUG_
            cout <<"Opcion de Consulta"<<endl;
#endif
            break;
        case 'e':
            escribir = true;
#ifdef _DEBUG_
            cout <<"Opcion de Escritura"<<endl;
#endif
            break;
        case 'u':
            actualizar = true;
#ifdef _DEBUG_
            cout << "Opcion de Actualizacion" <<  endl;
#endif
            break;
				case 'r':
            imprimir_tendencia = true;
#ifdef _DEBUG_
            cout << "Opcion de Imprimir Tendencia" <<  endl;
#endif
            break;
        case 'v':
            strcpy( char_valor_tag, optarg );
#ifdef _DEBUG_
            cout <<"Valor del Tag "<<optarg<<endl;
#endif
            break;
        case 'g':
            strcpy(nombre_shm_tags, optarg);
#ifdef _DEBUG_
            cout <<"**Mostrando Información de Tag(s) Almacenado(s)**" << endl;
#endif
            if(observar) {
                mostrar_tags_de_bdtr();
            }
            break;
        case 's':
            strcpy(nombre_shm_alarmas, optarg);
#ifdef _DEBUG_
            cout <<"**Mostrando Información de Alarma(s) Almacenada(s)**" << endl;
#endif
            if(observar) {
                mostrar_alarmas_de_bdtr();
            }

            break;
        case 't':
            strcpy(nombre_tag, optarg);
#ifdef _DEBUG_
            cout << "Tag: " << optarg << endl;
#endif
            if( escribir ) {
                enviar_tag_a_escritor();
                return 1;
            }
            break;
        case 'a':
            strcpy(nombre_alarma, optarg);
#ifdef _DEBUG_
            cout << "Alarma: " << optarg << endl;
#endif
            if( escribir ) {
                enviar_alarma_a_escritor();
                return 1;
            }
            break;
        case 'i':
            strcpy(ip_servidor, optarg);
#ifdef _DEBUG_
            cout << "Servidor: " << optarg << endl;
#endif
            break;
        case 'p':
            puerto = atoi(optarg);
#ifdef _DEBUG_
            cout << "Puerto: " << optarg << endl;
#endif
            break;
        case 'd':
            switch( atoi(optarg) ) {
            case 0:
                tipo_tag = BIT_;
                break;
            case 1:
                tipo_tag = CARAC_;
                break;
            case 2:
                tipo_tag = CORTO_;
                break;
            case 3:
                tipo_tag = ENTERO_;
                break;
            case 4:
                tipo_tag = LARGO_;
                break;
            case 5:
                tipo_tag = REAL_;
                break;
            case 6:
                tipo_tag = DREAL_;
                break;
            default:
                tipo_tag = TERROR_;
            }
#ifdef _DEBUG_
            cout << "Tipo de Dato: " << Tipo_Tag_String(tipo_tag) << endl;
#endif
            break;
        case 'm':
            switch( atoi(optarg) ) {
            case 0:
                estado_alarma = BAJO_;
                break;
            case 1:
                estado_alarma = BBAJO_;
                break;
            case 2:
                estado_alarma = ALTO_;
                break;
            case 3:
                estado_alarma = AALTO_;
                break;
            case 4:
                estado_alarma = ACTIVA_;
                break;
            case 5:
                estado_alarma = DESACT_;
                break;
            case 6:
                estado_alarma = RECON_;
                break;
            default:
                estado_alarma = EERROR_;
            }
#ifdef _DEBUG_
            cout << "Estado de Alarma: " << Estado_Alarma_String(estado_alarma) << endl;
#endif
            break;
        case 'o':
            observar = true;
#ifdef _DEBUG_
            cout <<"Opcion de Observar Base de Datos de Tiempo Real"<<endl;
#endif
            break;
        case 'h':
            help(2);
            break;
        case '?':
            help(1);
            break;
        default:
            help(1);
            return false;
        }
    }

    if (verbose_flag) {
        cout << "Modo Verbose" << endl;
        cout << "\tProyecto Argos (ssescritor) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
        cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";

    }
    if (optind < argc) {
        cout << "Opciones no Reconocidas: ";
        while (optind < argc)
            cout << argv[optind++] << " ";
        cout << endl;

    }


    struct timeval cTime1, cTime2, cTime3, cTime4, cTime5;
    gettimeofday( &cTime1, (struct timezone*)0 );

    if( leer) {
        leer_th_shm();
    }

    if (consultar) {
        consultar_th();
    }

    if( actualizar) {
        escribir_th_shm();
    }

   	if( imprimir_tendencia ){
   		obtener_tendencia_servidor();
   	}

    gettimeofday( &cTime2, (struct timezone*)0 );
    if(cTime1.tv_usec > cTime2.tv_usec) {
        cTime2.tv_usec = 1000000;
        --cTime2.tv_usec;
    }


    gettimeofday( &cTime3, (struct timezone*)0 );
    if(cTime2.tv_usec > cTime3.tv_usec) {
        cTime3.tv_usec = 1000000;
        --cTime3.tv_usec;
    }

    gettimeofday( &cTime4, (struct timezone*)0 );
    if(cTime3.tv_usec > cTime4.tv_usec) {
        cTime4.tv_usec = 1000000;
        --cTime4.tv_usec;
    }


    gettimeofday( &cTime5, (struct timezone*)0 );
    if(cTime4.tv_usec > cTime5.tv_usec) {
        cTime5.tv_usec = 1000000;
        --cTime5.tv_usec;
    }

    /*
    cout << endl << "Finalizando" << endl << endl;


    printf("\nTiempo Uno: %ld s, %ld us. \n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );
    printf("\nTiempo Dos: %ld s, %ld us. \n", cTime3.tv_sec - cTime2.tv_sec, cTime3.tv_usec - cTime2.tv_usec );
    printf("\nTiempo Tres: %ld s, %ld us. \n", cTime4.tv_sec - cTime3.tv_sec, cTime4.tv_usec - cTime3.tv_usec );
    printf("\nTiempo Cuatro: %ld s, %ld us. \n", cTime5.tv_sec - cTime4.tv_sec, cTime5.tv_usec - cTime4.tv_usec );
    */

}

/*----------------------------------------------------------
  Escribir a Dispositivo, es una función existente en una
  versión anterior. Se encarga de enviar al escritor la
  información del tag o la alarma que se deseaba modifica.
  Actualmente esta función fue separada en dos, las cuales
  son: enviar_alarma_a_escritor() y enviar_tag_a_escritor()
-----------------------------------------------------------*/

bool escribir_a_dispositivo() {
    int			 		sckt;
    struct sockaddr_in	pin;
    struct hostent		*servidor;
    Tmsgsswr 			mensaje;
    char				buf[sizeof(_ESCR_OK_)];

    if( ( servidor = gethostbyname( ip_servidor ) ) == 0 ) {
        mensaje_error = "Obteniendo Nombre del Servidor " + string( ip_servidor )
                        + "\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    bzero( &pin, sizeof( pin ) );
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = htonl( INADDR_ANY );
    pin.sin_addr.s_addr = ((struct in_addr *)(servidor->h_addr))->s_addr;
    pin.sin_port = htons( puerto );

    if( (sckt = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
        mensaje_error = "Obteniendo Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( connect( sckt, (struct sockaddr *)&pin, sizeof( pin ) ) == -1 ) {
        mensaje_error = "Conectando Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }


    if( nombre_tag[0] != '\0' ) {
        mensaje.tipo = _TIPO_TAG_;
        strcpy( mensaje.nombre_tag, nombre_tag );
        mensaje.tipo_dato = tipo_tag;

        switch(tipo_tag) {
        case CARAC_:
            mensaje.valor.C = static_cast<char> (atoi(char_valor_tag));
            cout << "Enviando valor: [" << mensaje.valor.C << "]" << endl;
            break;
        case CORTO_:
            mensaje.valor.S = static_cast<short> (atoi(char_valor_tag));
            cout << "Enviando valor: [" << mensaje.valor.S << "]" << endl;
            break;
        case ENTERO_:
            mensaje.valor.I = atoi(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.I << "]" << endl;
            break;
        case LARGO_:
            mensaje.valor.L = atoll(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.L << "]" << endl;
            break;
        case REAL_:
            mensaje.valor.F = (float) atof(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.F << "]" << endl;
            break;
        case DREAL_:
            mensaje.valor.D = atof(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.D << "]" << endl;
            break;
        case BIT_:
            mensaje.valor.B = (atoi(char_valor_tag) == 1) ? true : false;
            cout << "Enviando valor: [" << mensaje.valor.B << "]" << endl;
            break;
        case TERROR_:
        default:
            mensaje.valor.L = LONG_MIN;
            cout << "Tipo de Dato Desconocido" << endl;
        }
    } else if( nombre_alarma[0] != '\0' ) {
        mensaje.tipo = _TIPO_ALARMA_;
        strcpy( mensaje.nombre_alarma, nombre_alarma );
        mensaje.tipo_dato = TERROR_;
        mensaje.estado = estado_alarma;
    }

    if( send( sckt, (char *)&mensaje, sizeof( mensaje ), 0 ) == -1 ) {
        mensaje_error = "Enviando Tag\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( recv( sckt, buf, sizeof(_ESCR_OK_), 0 ) == -1 ) {
        mensaje_error = "Recibiendo Respuesta\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    int         *res = reinterpret_cast<int *>(&buf[0]);

    switch( *res ) {
    case _ESCR_OK_:
        cout << "Tag Enviado Exitosamente" << endl;
        break;
    case _ESCR_ERROR_:
        cout << "Error Escribiendo Tag" << endl;
        break;
    default:
        cout << "Respuesta Desconocida" << endl;
    }

    close( sckt );
    return true;
}

/*----------------------------------------------------------
  Estado_Alarma_String(int estado_alarma)
  Dado un entero que representa el estado de una alarma,
  esta función se encarga de retornar la cadena de caracteres
  equivalente a dicho estado.
-----------------------------------------------------------*/

char *Estado_Alarma_String(int estado_alarma) {

    switch(estado_alarma) {
    case 0:
        return (char *)"BAJO_";
    case 1:
        return (char *)"BBAJO_";
    case 2:
        return (char *)"ALTO_";
    case 3:
        return (char *)"AALTO_";
    case 4:
        return (char *)"ACTIVA_";
    case 5:
        return (char *)"DESACT_";
    case 6:
        return (char *) "RECON_";
    default:
        return (char *) "EERROR_";
    }
    return (char *)" ";
}

/*----------------------------------------------------------
  Tipo_Tag_String(int tipo)
  Dado un entero que representa el tipo de dato de un tag,
  esta función se encarga de retornar la cadena de caracteres
  equivalente a dicho tipo.
-----------------------------------------------------------*/

char *Tipo_Tag_String(int tipo) {

    switch(tipo) {
    case 0:
        return (char *) "Bit";
    case 1:
        return (char *)"Caracter";
    case 2:
        return (char *)"Corto";
    case 3:
        return (char *)"Entero";
    case 4:
        return (char *)"Largo";
    case 5:
        return (char *)"Real";
    case 6:
        return (char *)"Bajo";
    default:
        return (char *)"TERROR";
    }
    return (char *)" ";
}

/*----------------------------------------------------------
  la función help esta encargada de mostrar la información
  relacionada con la ayuda del programa as-herramienta. Recibe
  un entero que indicará el tipo de mensaje de ayuda que se
  mostrará, ya sea 1 para cuando existe un error en la llamada
  al programa, o 2 para cuando el usuario solicite la ayuda
  haciendo uso de la opción "-h" o "--help"
-----------------------------------------------------------*/
void help(int tipo) {
    if(tipo == 1) {
        fputs(("Intente as-herramienta --help o as-herramienta -h para mayor informacion.\n"),stdout);

    } else	if (tipo == 2) {
        fputs (("\
  Este es un utilitario que permite hacer operaciones de lectura y escritura de Tags y Alarmas en los dispositivos y Base de Datos en Tiempo Real del SCADA.\n\n\
 "), stdout);

        fputs (("\
  Los argumentos obligatorios para las opciones largas son también obligatorios para las opciones cortas.\n\n\
 "), stdout);

        fputs (("\
  -l, --leer                 Leer la informacion de un elemento \n\
                             almacenado en la Base de Datos Tiempo \n\
                             Real correspondiente.\n\n\
 "), stdout);

        fputs (("\
                            Para leer un tag, agregar -t seguido \n\
                             del nombre del Tag a consultar ademas \n\
                             agregar -g seguido de la ubicacion de \n\
                             la memoria compartida en el equipo. \n\n\
  "), stdout);

        fputs (("\
                           Para leer una alarma, agregar -a seguido \n\
                             del nombre de la alarma a consultar ademas \n\
                             agregar -s seguido de la ubicacion de \n\
                             la memoria compartida en el equipo. \n\n\
  "), stdout);

        fputs (("\
-e, --escribir             Enviar datos al escritor para que los \n\
                             envie a los dispositivos correspondientes. \n\n\
 "), stdout);
        fputs (("\
                            Para enviar datos de un Tag se necesita \n\
                             las siguientes especificaciones: \n\n\
                  -i         Para indicarle la ip del escritor. \n\
                  -p         Para indicarle el puerto. \n\
                  -d         Para indicarle el tipo de dato. \n\
                  -t         Para indicarle el nombre del Tag. \n\
                  -v         Para indicarle el valor del Tag. \n\n\
 "), stdout);

        fputs (("\
                            Para enviar datos de una alarma se \n\
                             necesita las siguientes especificaciones: \n\n\
                  -i         Para indicarle la ip del escritor. \n\
                  -p         Para indicarle el puerto. \n\
                  -a         Para indicarle nombre de la alarma. \n\
                  -m         Para indicarle el estado de alarma. \n\n\
 "), stdout);

        fputs (("\
 -o, --observar             Muestra la informacion contenida en la \n\
                             Base de Datos Tiempo Real.\n\n\
 "), stdout);

        fputs (("\
                            Para observar la informacion de los Tags \n\
                             se realiza la siguiente especificacion: \n\n\
                  -g         Y agregando la ubicacion de la memoria \n\
                             compartida de los Tags. \n\n\
 "), stdout);

        fputs (("\
                            Para observar la informacion de las Alarmas \n\
                             se realiza la siguiente especificacion: \n\n\
                  -s         Y agregando la ubicacion de la memoria \n\
                             compartida de las Alarmas. \n\n\
 "), stdout);

        fputs (("\
 -c, --consultar            Muestra el tamaño de la tabla de Hash de la \n\
                             memoria compartida, la cantidad de elementos \n\
                             (Tags o Alarmas) asi como el numero de colisiones \n\
                             asociadas a dicha tabla. \n\n\
 "), stdout);
        fputs (("\
                            Para observar la informacion de los Tags \n\
                             se realiza la siguiente especificacion: \n\n\
                  -g         Y agregando la ubicacion de la memoria \n\
                             compartida de los Tags. \n\n\
 "), stdout);

        fputs (("\
                            Para observar la informacion de las Alarmas \n\
                             se realiza la siguiente especificacion: \n\n\
                  -s         Y agregando la ubicacion de la memoria \n\
                             compartida de las Alarmas. \n\n\
 "), stdout);

        fputs (("\
 -u, --actualizar           Actualiza el valor del Tag o Alarma en la Base \n\
                             de Datos Tiempo Real. \n\n\
 "), stdout);

        fputs (("\
                            Para actualizar datos de un Tag se necesita \n\
                             las siguientes especificaciones: \n\n\
                  -g         Para indicarle ubicacion de la memoria \n\
                             compartida de los Tags. \n\
                  -t         Para indicarle el nombre del Tag. \n\
                  -d         Para indicarle el tipo de dato. \n\
                  -v         Para indicarle el valor del Tag. \n\n\
 "), stdout);

        fputs (("\
                            Para actualizar datos de una alarma se \n\
                             necesita las siguientes especificaciones: \n\n\
                  -s         Para indicarle ubicacion de la memoria \n\
                             compartida de las Alarmas. \n\
                  -a         Para indicarle nombre de la alarma. \n\
                  -m         Para indicarle el estado de alarma. \n\n\
 "), stdout);



    }
}

void mostrar_alarmas_de_bdtr() {
    if( nombre_shm_alarmas[0] != '\0' ) {
        shm_thalarma shm_th2(nombre_shm_alarmas);

        shm_th2.obtener_shm_tablahash();

        shm_th2.imprimir_shm_tablahash();

        shm_th2.cerrar_shm_tablahash();
    }

}
void mostrar_tags_de_bdtr() {
    if( nombre_shm_tags[0] != '\0' ) {
        shm_thtag shm_th(nombre_shm_tags);

        shm_th.obtener_shm_tablahash();

        shm_th.imprimir_shm_tablahash();

        shm_th.cerrar_shm_tablahash();
    }
}


/*----------------------------------------------------------
  enviar_alarma_a_escritor() se encarga de enviar la nueva
  información de una alarma suministrada por el usuario, al
  servicio"escritor", de tal forma que este ultimo lo pase
  a el dispositivo correspondiente y los cambios sean realizados.
-----------------------------------------------------------*/
bool enviar_alarma_a_escritor() {
    struct timeval cTime1, cTime2;
    gettimeofday( &cTime1, (struct timezone*)0 );

    int sckt;
    struct sockaddr_in pin;
    struct hostent	*servidor;
    Tmsgsswr mensaje;
    char buf[sizeof(_ESCR_OK_)];

    if( ( servidor = gethostbyname( ip_servidor ) ) == 0 ) {
        mensaje_error = "Obteniendo Nombre del Servidor " + string( ip_servidor )
                        + "\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    bzero( &pin, sizeof( pin ) );
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = htonl( INADDR_ANY );
    pin.sin_addr.s_addr = ((struct in_addr *)(servidor->h_addr))->s_addr;
    pin.sin_port = htons( puerto );

    if( (sckt = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
        mensaje_error = "Obteniendo Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( connect( sckt, (struct sockaddr *)&pin, sizeof( pin ) ) == -1 ) {
        mensaje_error = "Conectando Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( nombre_alarma[0] != '\0' ) {
        mensaje.tipo = _TIPO_ALARMA_;
        strcpy( mensaje.nombre_alarma, nombre_alarma );
        mensaje.tipo_dato = TERROR_;
        mensaje.estado = estado_alarma;
    }

    if( send( sckt, (char *)&mensaje, sizeof( mensaje ), 0 ) == -1 ) {
        mensaje_error = "Enviando Alarma\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( recv( sckt, buf, sizeof(_ESCR_OK_), 0 ) == -1 ) {
        mensaje_error = "Recibiendo Respuesta\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    int *res = reinterpret_cast<int *>(&buf[0]);

    switch( *res ) {
    case _ESCR_OK_:
        cout << "Alarma Enviada Exitosamente" << endl;

        gettimeofday( &cTime2, (struct timezone*)0 );
        if(cTime1.tv_usec > cTime2.tv_usec) {
            cTime2.tv_usec = 1000000;
            --cTime2.tv_usec;
        }
        printf("\nTiempo que tardo: %ld s, %ld us. \n\n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );
        break;
    case _ESCR_ERROR_:
        cout << "Error Escribiendo Alarma" << endl << endl;
        break;
    default:
        cout << "Respuesta Desconocida" << endl << endl;
    }

    close( sckt );

    return true;
}

/*----------------------------------------------------------
  enviar_tag_a_escritor() se encarga de enviar la nueva
  información de un tag suministrada por el usuario, al
  servicio"escritor", de tal forma que este ultimo lo pase
  a el dispositivo correspondiente y los cambios sean realizados.
-----------------------------------------------------------*/

bool enviar_tag_a_escritor() {
    struct timeval cTime1, cTime2;
    gettimeofday( &cTime1, (struct timezone*)0 );

    int sckt;
    struct sockaddr_in pin;
    struct hostent	*servidor;
    Tmsgsswr mensaje;
    char buf[sizeof(_ESCR_OK_)];

    if( ( servidor = gethostbyname( ip_servidor ) ) == 0 ) {
        mensaje_error = "Obteniendo Nombre del Servidor " + string( ip_servidor )
                        + "\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    bzero( &pin, sizeof( pin ) );
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = htonl( INADDR_ANY );
    pin.sin_addr.s_addr = ((struct in_addr *)(servidor->h_addr))->s_addr;
    pin.sin_port = htons( puerto );

    if( (sckt = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
        mensaje_error = "Obteniendo Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( connect( sckt, (struct sockaddr *)&pin, sizeof( pin ) ) == -1 ) {
        mensaje_error = "Conectando Socket\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }


    if( nombre_tag[0] != '\0' ) {
        mensaje.tipo = _TIPO_TAG_;
        strcpy( mensaje.nombre_tag, nombre_tag );
        mensaje.tipo_dato = tipo_tag;

        switch(tipo_tag) {
        case CARAC_:
            mensaje.valor.C = static_cast<char> (atoi(char_valor_tag));
            cout << "Enviando valor: [" << mensaje.valor.C << "]" << endl;
            break;
        case CORTO_:
            mensaje.valor.S = static_cast<short> (atoi(char_valor_tag));
            cout << "Enviando valor: [" << mensaje.valor.S << "]" << endl;
            break;
        case ENTERO_:
            mensaje.valor.I = atoi(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.I << "]" << endl;
            break;
        case LARGO_:
            mensaje.valor.L = atoll(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.L << "]" << endl;
            break;
        case REAL_:
            mensaje.valor.F = (float) atof(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.F << "]" << endl;
            break;
        case DREAL_:
            mensaje.valor.D = atof(char_valor_tag);
            cout << "Enviando valor: [" << mensaje.valor.D << "]" << endl;
            break;
        case BIT_:
            mensaje.valor.B = (atoi(char_valor_tag) == 1) ? true : false;
            cout << "Enviando valor: [" << mensaje.valor.B << "]" << endl;
            break;
        case TERROR_:
        default:
            mensaje.valor.L = LONG_MIN;
            cout << "Tipo de Dato Desconocido" << endl;
        }
    }


    if( send( sckt, (char *)&mensaje, sizeof( mensaje ), 0 ) == -1 ) {
        mensaje_error = "Enviando Tag\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    if( recv( sckt, buf, sizeof(_ESCR_OK_), 0 ) == -1 ) {
        mensaje_error = "Recibiendo Respuesta\n" + string( strerror( errno ) );
        throw runtime_error( mensaje_error.c_str() );
    }

    int *res = reinterpret_cast<int *>(&buf[0]);

    switch( *res ) {
    case _ESCR_OK_:
        cout << "Tag Enviado Exitosamente" << endl;

        gettimeofday( &cTime2, (struct timezone*)0 );
        if(cTime1.tv_usec > cTime2.tv_usec) {
            cTime2.tv_usec = 1000000;
            --cTime2.tv_usec;
        }
        printf("\nTiempo que tardo: %ld s, %ld us. \n\n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );
        break;
    case _ESCR_ERROR_:
        cout << "Error Escribiendo Tag" << endl << endl;
        break;
    default:
        cout << "Respuesta Desconocida" << endl << endl;
    }

    close( sckt );

    gettimeofday( &cTime2, (struct timezone*)0 );
    if(cTime1.tv_usec > cTime2.tv_usec) {
        cTime2.tv_usec = 1000000;
        --cTime2.tv_usec;
    }

    return true;
}

/*
*	@Nombre: 	leer_th_shm()
*	@Descripcion:	Lee el estado/valor una alarma/tag de la tabla hash en la memoria compartida
*	@Parametro:	void
*	@Retorna:	void
*/
void leer_th_shm() {

    //lee alarama de la th del segmento de memoria compartida escaner/alarmas
    //
    if ( nombre_shm_alarmas[0] != '\0' ) {
        shm_thalarma shm_th(nombre_shm_alarmas);
        shm_th.obtener_shm_tablahash();
        alarma a;
        Tvalor val;

        if ( nombre_alarma[0] != '\0' ) {
            a.asignar_propiedades(0,"singrupo",0,"sinsubgrupo",nombre_alarma,BAJO_,val,MAYOR_QUE_,ENTERO_);
            if(shm_th.buscar_alarma(&a)) {
                cout<< "———————————————————————————————————————";
                cout<< endl << "\033[1;" << 34 << "mNombre                         Estado\033[0m" << endl;
                cout<< "———————————————————————————————————————";
                cout << endl << a << endl << endl;
            } else {
                cout << endl << "\033[0;" << 31 << "mNo se pudo ubicar Alarma: [" << nombre_alarma << "] en la Tabla " << nombre_shm_alarmas << "\033[0m" << endl << endl;
            }

        } else {
            cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, falta nombre de la alarma...\033[0m" << endl << endl;
        }

        shm_th.cerrar_shm_tablahash();
    }
    //lee tag de la th del segmento de memoria compartida escaner/tags
    //
    else if ( nombre_shm_tags[0] != '\0') {
        shm_thtag shm_th(nombre_shm_tags);
        shm_th.obtener_shm_tablahash();
        tags t;
        Tvalor val;

        if ( nombre_tag[0] != '\0' ) {
            t.asignar_nombre( nombre_tag);
            if( shm_th.buscar_tag(&t) ) {
                cout<< "————————————————————————————————————————————————————————————";
                cout<< endl <<"\033[1;" << 34 << "mNombre                            Valor               Tipo\033[0m" << endl;
                cout<< "————————————————————————————————————————————————————————————";
                cout << endl << t << endl<< endl;
            } else {
                cout << endl << "\033[0;" << 31 << "mNo se pudo ubicar Tag: [" << nombre_tag << "] en la Tabla " << nombre_shm_tags << "\033[0m" << endl << endl;
            }

        } else {
            cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, falta nombre de Tag...\033[0m" << endl << endl;
        }

        shm_th.cerrar_shm_tablahash();
    }

    else {
        cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, falta nombre BDTR ...\033[0m" << endl << endl;
    }


}

/*
*	@Nombre: 	Consultar_th()
*	@Descripcion:	Consultar informacion de la tabla hash (cantidad elementos, colisiones, tamaño tabla)
*	@Parametro:	void
*	@Retorna:	void
*/
void consultar_th() {
    //Informacion th alarmas
    //
    if ( nombre_shm_alarmas[0] != '\0') {
        shm_thalarma shm_th(nombre_shm_alarmas);
        shm_th.obtener_shm_tablahash();
        thalarma * th;

        th = shm_th.obtener_tablahash();
        cout << endl << "\033[1;" << 34 << "mTamaño th alarma:\033[0m\t"<< th->obtener_tamano_tabla() << endl;
        cout << "\033[1;" << 34 << "mCantidad alarmas:\033[0m\t" << th->obtener_cantidad_alarmas() << endl;
        cout << "\033[1;" << 34 << "mColisiones:\033[0m\t\t" << th->obtener_colisiones() << endl << endl;

        shm_th.cerrar_shm_tablahash();

    }

    //Informacion th tags
    //
    else if ( nombre_shm_tags[0] != '\0') {
        shm_thtag shm_th2(nombre_shm_tags);
        shm_th2.obtener_shm_tablahash();
        thtag * th2;

        th2 = shm_th2.obtener_tablahash();

        cout << endl << "\033[1;" << 34 << "mTamaño th tags:\033[0m\t"<< th2->obtener_tamano_tabla() << endl;
        cout << "\033[1;" << 34 << "mCantidad tags:\033[0m\t" << th2->obtener_cantidad_tags() << endl;
        cout << "\033[1;" << 34 << "mColisiones:\033[0m\t" << th2->obtener_colisiones() << endl << endl;

        shm_th2.cerrar_shm_tablahash();
    }

    else {
        cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, falta nombre BDTR...\033[0m" << endl << endl;
    }

}

/*
*	@Nombre: 	escribir_th_shm()
*	@Descripcion:	Escribir (actualizar) en la tabla hash de la memoria compartida el estado/valor de una alarma/tag
*	@Parametro:	void
*	@Retorna:	void
*/
void escribir_th_shm() {

    if ( nombre_shm_alarmas[0] != '\0') {
        shm_thalarma shm_th(nombre_shm_alarmas);
        shm_th.obtener_shm_tablahash();
        alarma a;
        Tvalor val;

        a.asignar_propiedades(nombre_alarma, BAJO_, TERROR_);
        if(shm_th.buscar_alarma(&a)) {
            a.asignar_propiedades(nombre_alarma, estado_alarma, TERROR_);
            shm_th.actualizar_alarma(&a);
            cout << endl  << "\033[1;" << 34 << "m Se actualizo alarma satisfactoriamente\033[0m\t" << endl << endl;
        } else {
            cout << endl << "\033[0;" << 31 << "mNo se pudo ubicar alarma: [" << nombre_alarma << "] en la Tabla " << nombre_shm_alarmas << "\033[0m" << endl << endl;
        }

        shm_th.cerrar_shm_tablahash();

    } else if( nombre_shm_tags[0] != '\0' ) {
        shm_thtag shm_th2(nombre_shm_tags);
        shm_th2.obtener_shm_tablahash();
        tags t;
        Tvalor d;

        t.asignar_nombre( nombre_tag);
        if( shm_th2.buscar_tag(&t) ) {
            switch(tipo_tag) {
            case CARAC_:
                d.C = static_cast<char> (atoi(char_valor_tag));
                break;
            case CORTO_:
                d.S = static_cast<short> (atoi(char_valor_tag));
                break;
            case ENTERO_:
                d.I = atoi(char_valor_tag);
                break;
            case LARGO_:
                d.L = atoll(char_valor_tag);
                break;
            case REAL_:
                d.F = (float) atof(char_valor_tag);
                break;
            case DREAL_:
                d.D = atof(char_valor_tag);
                break;
            case BIT_:
                d.B = (atoi(char_valor_tag) == 1) ? true : false;
                break;
            case TERROR_:
            default:
                cout << "\033[0;" << 31 << "mTipo de Dato Desconocido\033[0m" << endl << endl;
            }

            //Comparo que el tipo de dato sea el mismo.
            if ( t.tipo_dato() == tipo_tag) {
                t.asignar_propiedades( nombre_tag, d, tipo_tag);
                shm_th2.actualizar_tag(&t);
                cout << endl  << "\033[1;" << 34 << "m Se actualizo tag satisfactoriamente\033[0m\t" << endl << endl;

            } else {
                cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, el tipo de dato ingresado no corresponde al tag\033[0m" << endl << endl;
            }
        } else {
            cout << endl << "\033[0;" << 31 << "mNo se pudo ubicar Tag: [" << nombre_tag << "] en la Tabla " << nombre_shm_tags << "\033[0m" << endl << endl;
        }

        shm_th2.cerrar_shm_tablahash();

    } else {
        cout << endl << "\033[0;" << 31 << "mNo se puede llevar a cabo Operacion, falta nombre BDTR...\033[0m" << endl << endl;
    }
}




void obtener_tendencia_servidor(){
	string xml_in =
		"babbbbab"
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
		"<tendencias_xml>"
		"<cantidad_variables>1</cantidad_variables>"
		"<tipo>bd</tipo>"
		"<variable id=\"1\" nombre=\"rot_ms_Tiempo_Uso_Motor\" >"
		"<desde>120000</desde>"
		"<hasta>125000000000</hasta>"
		"</variable>"
		"</tendencias_xml>";

	cout << "Mensaje Enviado:\n" << xml_in.c_str() << "\nLongitud: " << xml_in.size() << endl;

	char * respuesta = obtener_xml_servidor_tendencia("127.0.0.1", 16009, xml_in.c_str(), xml_in.size() );
	if(respuesta)
		cout << respuesta << endl;


}
