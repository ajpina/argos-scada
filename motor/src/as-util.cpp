/**
* @file	as-util.cpp
* @brief		<b>Utilitario usado por Argos</b><br>
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
#include "as-util.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <cstdio>

char __ARCHIVO_CONF_TAGS[512] = "\0";			//!< Archivo XML de Tags
char __ARCHIVO_CONF_ALARMAS[512] = "\0";		//!< Archivo XML de Alarmas
char __ARCHIVO_CONF_DEVS[512] = "\0";			//!< Archivo XML de Dispositivos
char __ARCHIVO_CONF_SERV_DATOS[512] = "\0";		//!< Archivo XML de Servidor
char __ARCHIVO_CONF_CLIENTE_DATOS[512] = "\0";	//!< Archivo XML de Clientes
char __ARCHIVO_CONF_HISTORICOS[512] = "\0";		//!< Archivo XML de Historicos

/*--------------------------------------------------------
	Funciones para interpolacion de muestras.
----------------------------------------------------------*/
const char *_FUNCIONES_HISTORICOS_[] = {
		"AVG_",
		"STD_",
		"VAR_",
		"PZ_",
		"NO_",
		"FERROR_",
		"\0"
};

/**
* @brief	Obtiene un numero primo mayor que el parametro
* @param	no_primo	Numero No primo
*/
void primo_mayor_que(unsigned int & no_primo){
	unsigned int divisor = 2;
	unsigned int numero = no_primo;

	while(divisor<numero){
		while( numero % divisor != 0)
			divisor++;
		if(numero == divisor){
			no_primo = numero;
			break;
		}
       else{
    	   numero++;
    	   divisor = 2;
       }
  }
}

/**
* @brief	Obtiene la cantidad de funciones disponibles
* @return	uint	Cantidad de funciones
*/
unsigned int numero_funciones_disponibles(){
	unsigned int n = 0;
	while( strcmp(_FUNCIONES_HISTORICOS_[n++],"FERROR_") != 0 );
	return n-1;
}

/**
* @brief	Registra en el LOG de sistema configurado en syslog.conf
* @param	servicio	Nombre del proceso
* @param	opcion		Opcion del log
* @param	facilidad	Facilidad del log
* @param	nivel		Nivel del mensaje
* @param	msg			Mensaje del log
*/
void escribir_log(const char * servicio, int nivel, const char * msg){
	openlog (servicio, LOG_PID, LOG_LOCAL3);
	syslog(nivel, "%s\n", msg);
	closelog();
}

/**
* @brief	Imprime el numero de espacios necesarios para cuadrar
*        	la informaciòn imprimida en el espacio en pantalla
*					deseado
* @param	os				Stream de salida
*	@param	nombre		Mensaje a imprimir
* @param	maximo		cantidad de espacios
*/
void mostrar_espaciado(ostream & os, const char * nombre,int maximo){
				int espacio;
				espacio = maximo - strlen(nombre);
				for (int i = 0; i < espacio; i++){
					os<<" ";
				}
}


string dato2texto(Tdato _tipo){
	string res;
	switch(_tipo){
		case CARAC_:
				res = "CARACTER";
				break;
		case CORTO_:
				res = "CORTO";
				break;
		case ENTERO_:
				res = "ENTERO";
				break;
		case LARGO_:
				res = "LARGO";
				break;
		case REAL_:
				res = "REAL";
				break;
		case DREAL_:
				res = "DOBLE";
				break;
		case BIT_:
				res = "BIT";
				break;
		case TERROR_:
		default:
				res = "DESCONOCIDO";
		}
		return res;
}

string log2texto(Tlog _tipo){
	string res;
	switch(_tipo){
		case EVENTO_:
				res = "EVENTO";
				break;
		case MUESTREO_:
				res = "MUESTREO";
				break;
		case LERROR_:
		default:
				res = "DESCONOCIDO";
		}
		return res;
}


string func2texto(Tfunciones _tipo){
	string res;
	switch(_tipo){
		case AVG_:
				res = "PROMEDIO";
				break;
		case STD_:
				res = "DESVIACION";
				break;
		case VAR_:
				res = "VARIANZA";
				break;
		case PZ_:
				res = "PUNTO Z";
				break;
		case NO_:
				res = "NO APLICA";
				break;
		case FERROR_:
		default:
				res = "DESCONOCIDO";
		}
		return res;

}


string valor2texto(Tvalor _valor,Tdato _tipo){
	string res;
	switch(_tipo){
		case CARAC_:
				res = numero2string<char>(_valor.C,std::dec);
				break;
		case CORTO_:
				res = numero2string<short>(_valor.S,std::dec);
				break;
		case ENTERO_:
				res = numero2string<int>(_valor.I,std::dec);
				break;
		case LARGO_:
				res = numero2string<__int64_t>(_valor.L,std::dec);
				break;
		case REAL_:
				res = numero2string<float>(_valor.F,std::dec);
				break;
		case DREAL_:
				res = numero2string<double>(_valor.D,std::dec);
				break;
		case BIT_:
				res = numero2string<bool>(_valor.B,std::dec);
				break;
		case TERROR_:
		default:
				res = "NAN";
		}
		return res;

}


char * obtener_xml_servidor_tendencia(char * _ip, unsigned int _puerto, const char * _xml_in, unsigned int _lon){
	int			 			sckt;
	struct
	sockaddr_in		pin;
	struct
	hostent				*servidor;

	if( ( servidor = gethostbyname( _ip ) ) == 0 ) {
			perror("Obteniendo Nombre del Servidor");
			return NULL;
	}

	bzero( &pin, sizeof( pin ) );
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = htonl( INADDR_ANY );
	pin.sin_addr.s_addr = ((struct in_addr *)(servidor->h_addr))->s_addr;
	pin.sin_port = htons( _puerto );

	if( (sckt = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
			perror("Obteniendo Socket");
			return NULL;
	}

	if( connect( sckt, (struct sockaddr *)&pin, sizeof( pin ) ) == -1 ) {
			perror("Conectando Socket");
			return NULL;
	}

	if( send( sckt, _xml_in, _lon, 0 ) == -1 ) {
			perror("Enviando Mensaje");
			return NULL;
	}
	else{
		char 				respuesta[600000];
		if( recv( sckt, respuesta, sizeof(respuesta), 0 ) == -1 ) {
				perror("Recibiendo Respuesta");
				return NULL;
		}
		close( sckt );
		return respuesta;
	}
	close( sckt );
	return NULL;
}
