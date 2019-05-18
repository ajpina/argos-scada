/**
* @file	as-clx_ethip.cpp
* @brief		<b>Proceso encargado de Manejar la Comunicacion con
*				Dispositivos Allen Bradley que soporten el Protocolo
*				Ethernet/IP.<br>
*				Este proceso utiliza la biblioteca de funciones:<br>
*				TuxPLC de Stephane JEANNE s.jeanne@tuxplc.net</b><br>
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

#ifndef ETHIP_H_
#define ETHIP_H_

#include "as-util.h"
#include "TuxEip.h"
#include "as-registro.h"

#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <algorithm>
#include <stdexcept>

#define space cout << "\n"
#define flush(header) _CipFlushBuffer(header,24+header->Length);
#define _MAX_CANT_REGS_ 500				//!< Cantidad de Registros soportados por el Manejador

using namespace std;
using namespace argos;

string				mensaje_error;		//!< Mensaje de Error por Pantalla

namespace ethipConf{

/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			Administra la Comunicacion con los Dispositivos de Campo
*			Allen Bradley a traves del Protocolo Ethernet/IP</b><br>
*/
class configuracion{
public:
	char			nombre_marca_id[100];		//!< Nombre del Dispositivo
	char			str_tipo[10];				//!< Tipo de Dispositivo (String)
	Plc_Type		tipo;						//!< Tipo de Dispositivo
	char			direccion[100];				//!< Direccion IP del Dispositivo
	char			ruta[100];					//!< Ruta en el Backplane del Modulo de Comunicacion
	char			nodo[10];					//!< Nodo dentro de la Red
	bool			habilitado;					//!< Habilitado para operar
	int				tiempo_muestreo;			//!< Tiempo entre cada adquisicion de datos
	vector <registro> vregistros;				//!< Vector de Registros que maneja el dispositivo
	registro		*bloque;					//!< Memoria Compartida de Registros
	mqd_t			qd;							//!< Cola de Mensajes para los requisitos de escritura
	int				fd;							//!< Descriptor de la Memoria Compartida

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los atributos
	de los registros que se pueden escribir en el
	dispositivo
----------------------------------------------------------*/
	struct{
		char		direccion[100];				//!< Direccion del Registro
		Tdato		tipo;						//!< Tipo de Dato del Registro
		bool		escritura;					//!< Habilitado para Escritura
	}registros[_MAX_CANT_REGS_];

	LGX_Read		*data;						//!< Data Transferida por Ethernet/IP
	Eip_Session 	*session;					//!< Sesion a registrar por Ethernet/IP
	Eip_Connection 	*connection;				//!< Conexion Ethernet/IP

/*--------------------------------------------------------
	Metodos Implementados para la Configuracion
	del Manejador de Comunicacion
----------------------------------------------------------*/
	configuracion();
	~configuracion();
	bool ordenar_registros();
	bool crear_memoria_compartida();
	bool crear_cola();
};

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
configuracion::configuracion(){
	nombre_marca_id[0] 	= '\0';
	str_tipo[0]			= '\0';
	tipo				= Unknow;
	direccion[0]		= '\0';
	ruta[0]				= '\0';
	nodo[0]				= '\0';
	habilitado			= false;
	tiempo_muestreo		= 1000;
	bloque				= NULL;
	qd					= -1;
	fd					= -1;
	data				= NULL;
	session				= NULL;
	connection			= NULL;
}

/**
* @brief	Destructor por Defecto, libera los recursos
*			reservados
*/
configuracion::~configuracion(){
}

/**
* @brief	Ordena los registros alfabeticamente por nombre
* @return	verdadero	Operacion Satisfactoria
*/
bool configuracion::ordenar_registros(){

	stable_sort(vregistros.begin(),vregistros.end());

#ifdef _DEBUG_
	cout << "Construido Vector de Registros\n";
	for(unsigned int i = 0; i < vregistros.size(); i++){
		cout << vregistros[i] << "\t\t\t";
	}
#endif
	return true;
}

/**
* @brief	Crea la Memoria Compartida con los Registros
*			para el dispositivo
* @return	verdadero	Creacion Satisfactoria
*/
bool configuracion::crear_memoria_compartida(){
    unsigned long int segmento_memoria = sizeof(registro) * vregistros.size();
    struct stat datos_directorio;
    lstat("/dev/shm/dispositivos", &datos_directorio);
    if ( !S_ISDIR( datos_directorio.st_mode ) ) {
        if (-1 == mkdir("/dev/shm/dispositivos",0777) ) {
            mensaje_error = "Error Creando Directorio /dev/shm/dispositivos: " + string( strerror( errno ) );
            escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error( mensaje_error.c_str() );
        }
    }
	string nombre_shm = "dispositivos/" + string(nombre_marca_id);

    fd = shm_open(nombre_shm.c_str(), O_CREAT|O_RDWR, S_IRWXU);
    if ( -1 == fd ) {
        mensaje_error = "Error Abriendo SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    if (ftruncate(fd, segmento_memoria)<0) {
        mensaje_error = "Error Truncando SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    bloque = ( registro * )mmap(0, segmento_memoria, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (long)0);
    if ( NULL == bloque ) {
        mensaje_error = "Error Mapeando SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    memset(bloque,'\0',segmento_memoria);		//Inicializar Segmento de Memoria

    //------------------Copiar Registros de Memoria Principal a Memoria Compartida
    for(unsigned int i = 0; i < vregistros.size(); i++){
    	(*(bloque + i)) = vregistros[i];
    }

	return true;
}

/**
* @brief	Crea la Cola para el Dispositivo donde recibira
*			los requisitos para efectuar operaciones de escritura
* @return	verdadero	Creacion Satisfactoria
*/
bool configuracion::crear_cola(){
	string nombre_cola;
	nombre_cola = "/" + string(nombre_marca_id);
	qd = mq_open(nombre_cola.c_str(), O_CREAT | O_RDONLY | O_NONBLOCK, 00666, NULL );
	if(qd == (mqd_t)-1){
		mensaje_error = "Error Abriendo Cola de Mensajes: " + nombre_cola + "\n" +
				string( strerror( errno ) );
		escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}
	struct mq_attr atributos;
	if( mq_getattr( qd, &atributos ) < 0 ){
		mensaje_error = "Error Obteniendo Atributos de Cola de Mensajes: " + string( strerror( errno ) );
		escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}
#ifdef _DEBUG_
	cout << "COLA: " << qd << "\tMAX MENSAJES: " << atributos.mq_maxmsg <<
			"\tTAM MENSAJES: " << atributos.mq_msgsize << "\tMENSAJES ACTUALES: " <<
			atributos.mq_curmsgs << endl;
#endif

	return true;
}


}


#endif // ETHIP_H_
