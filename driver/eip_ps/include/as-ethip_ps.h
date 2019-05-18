/**
* @file	as-clx_ethip_ps.h
* @brief		<b>Proceso encargado de Manejar la Comunicacion con
*				Dispositivos que soporten el envio de Mensajes UDP.<br>
*				El Formato del Paquete viene dado por un indice
*				y la longitud en bytes del dato.<br>
*				Este protocolo ha sido disenado para establecer
*				comunicacion con Modulos WEB+ de Allen Bradley (CLX).</b><br>
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


#ifndef ETHIP_PS_H_
#define ETHIP_PS_H_

#include <cstdlib>
#include <vector>

#include <mqueue.h>

#include "as-util.h"
#include "as-registro.h"


#define _MAX_CANT_REGS_			500		//!< Cantidad de Registros soportados por el Manejador
#define _MAX_CANT_DISP_X_DRVE_	5		//!< Cantidad de Equipos soportados por el Manejador
#define _LONG_MSG_UDP_			1024	//!< Tamano del Mensaje Enviado por los Dispositivos
#define _LONG_MSG_ENV_UDP_		1024	//!< Tamano del Mensaje Enviado a los Dispositivos
#define _MAX_NRO_INTENTOS_ESC_	3		//!< Cantidad de Intentos para Envio de Mensajes de Escritura


using namespace std;
using namespace argos;

string			mensaje_error;			//!< Mensaje de Error por Pantalla

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los argumentos
	al hilo encargado de entablar la conexion con los
	manejadores de comunicacion de los dispositivos
----------------------------------------------------------*/
typedef struct{
	char		*ptr_msg;				//!< Mensaje Proveniente del Dispositivo
	int			n_registros;			//!< Cantidad de Registros en el Mensaje
	int			pos;					//!< Posiscion del Registro dentro del Mensaje
}arg_hilo;

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los atributos
	de un registro que proviene de un dispositivo y se
	debe actualizar su valor en SHMEM
----------------------------------------------------------*/
typedef struct{
	char		direccion[100];			//!< Direccion en el Dispositivo
	Tdato		tipo;					//!< Tipo de Dato
	int			indice;					//!< Ubicacion del Registro en el Mensaje
	int			longitud;				//!< Tamano en Bytes del Registro
	int			bit;					//!< Posicion en caso de ser un Bit
	int			shm_pos;				//!< Ubicacion del Registro en SHMEM
	bool		escritura;				//!< Habilitado para la Escritura
}reg;

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los atributos
	de los dispositivos y los parametros de comunicacion
----------------------------------------------------------*/
typedef struct{
	char		nombre_marca_id_oyte_eqpo[200];	//!< Nombre del Dispositivo
	char		ip[20];							//!< Direccion IP del Dispositivo
	int			puerto_escritura;				//!< Puerto en el Dispositivo para Escribir
	reg			registros[_MAX_CANT_REGS_];		//!< Cantidad de Registros por Dispositivo
	int			fd;								//!< Descriptor de la SHMEM del Dispositivo
	registro	*shm;							//!< Referencia a la SHMEM
	int			cuenta_pqte;					//!< Contador de Paquetes
	mqd_t		qd;								//!< Cola donde recibe los requisitos de escritura
}eqpo;

namespace wpConf{

/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			Administra la Comunicacion con los Dispositivos de Campo
*			a traves de conexiones UDP (AB - CLX -> WEB+).</b><br>
*/
class configuracion{
public:
	bool		habilitado;							//!< Dispositivo Habilitado
	int			puerto;								//!< Puerto a Escuchar las Conexiones
	int			long_mensaje;						//!< Tamano del Mensaje
	eqpo		*equipos;							//!< Arreglo de Dispositivos
	int			n_equipos;							//!< Cantidad de Dispositivos a escuchar
	vector < vector <registro> >  mtx_registros;	//!< Tabla de Registros por cada Dispositivo

/*--------------------------------------------------------
	Metodos Implementados para la Configuracion
	del Manejador de Comunicacion
----------------------------------------------------------*/
	configuracion();
	~configuracion();
	bool asignar_n_equipos( int );
	int obtener_indice_equipo( char * );
	bool ordenar_registros();
	bool crear_memorias_compartidas();
	bool crear_colas();

};

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
configuracion::configuracion(){
	mtx_registros.resize( _MAX_CANT_DISP_X_DRVE_ );
}

/**
* @brief	Destructor por Defecto, libera los recursos
*			reservados
*/
configuracion::~configuracion(){
	if( equipos ){
		free( equipos );
	}
}

/**
* @brief	Asigna la cantidad de equipos que van
*			a ser atendidos por el Manejador de Comunicacion
* @param	n			Cantidad de dispositivos
* @return	verdadero	Creacion Satisfactoria
*/
bool configuracion::asignar_n_equipos( int n ){
	equipos = ( eqpo * ) malloc( sizeof(eqpo) * n);
	if( !equipos )
		return false;
	n_equipos  = n;
	return true;
}

/**
* @brief	Buscar por Nombre un Dispositivo en la Tabla
*			de Equipos - Registros
* @param	ip_src		Nombre del dispositivos
* @return	i			Posicion dentro de la Taba
*/
int configuracion::obtener_indice_equipo( char * ip_src ){
	for( int i = 0; i < n_equipos; i ++ ){
		if( strcmp( ip_src, (equipos + i)->ip ) == 0 )
			return i;
	}
	return -1;
}

/**
* @brief	Ordena los Registros por Equipo dentro
*			de la Tabla de Dispositivos
* @return	verdadero	Operacion Satisfactoria
*/
bool configuracion::ordenar_registros(){

	for( unsigned int i = 0; i < mtx_registros.size(); i++ ){
		stable_sort(mtx_registros[i].begin(), mtx_registros[i].end());
	}

#ifdef _DEBUG_
	cout << "Construido Vector de Registros\n";
	for( unsigned int i = 0; i < mtx_registros.size(); i++){
		cout << "Vector " << i << " :" <<endl;
		for( unsigned int j = 0; j < mtx_registros[i].size(); j++ ){
			cout << mtx_registros[i][j] << "\n";
		}
		cout << endl;
	}
#endif
	return true;
}

/**
* @brief	Crea la Memoria Compartida con los Registros
*			para cada uno de los dispositivos que se desean
*			atender
* @return	verdadero	Creacion Satisfactoria
*/
bool configuracion::crear_memorias_compartidas(){
	unsigned long int segmento_memoria;
    struct stat datos_directorio;
    lstat("/dev/shm/dispositivos", &datos_directorio);
    if ( !S_ISDIR( datos_directorio.st_mode ) ) {
        if (-1 == mkdir("/dev/shm/dispositivos",0777) ) {
            mensaje_error = "Error Creando Directorio /dev/shm/dispositivos: " + string( strerror( errno ) );
            escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error( mensaje_error.c_str() );
        }
    }
	string nombre_shm;
	for( int i = 0; i < n_equipos; i++){
		nombre_shm = "dispositivos/" + string(equipos[i].nombre_marca_id_oyte_eqpo);
		equipos[i].fd = shm_open(nombre_shm.c_str(), O_CREAT|O_RDWR, S_IRWXU);
		if ( -1 == equipos[i].fd ) {
			mensaje_error = "Error Abriendo SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		segmento_memoria = sizeof(registro) * mtx_registros[i].size();
		if (ftruncate(equipos[i].fd, segmento_memoria)<0) {
			mensaje_error = "Error Truncando SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		equipos[i].shm = ( registro * )mmap(0, segmento_memoria,
				PROT_READ|PROT_WRITE, MAP_SHARED, equipos[i].fd, (long)0);
		if ( NULL == equipos[i].shm ) {
			mensaje_error = "Error Mapeando SHMEM /dev/shm/" + nombre_shm + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		memset(equipos[i].shm,'\0',segmento_memoria);		//Inicializar Segmento de Memoria

		//------------------Copiar Registros de Memoria Principal a Memoria Compartida
		for(unsigned int j = 0; j < mtx_registros[i].size(); j++){
			(*(equipos[i].shm + j)) = mtx_registros[i][j];
			for( unsigned int w = 0; w < mtx_registros[i].size(); w++ ){
				if( strcmp( mtx_registros[i][j].direccion,
						equipos[i].registros[w].direccion ) == 0 ){
					equipos[i].registros[w].shm_pos = j;
					break;
				}
			}
		}

	}
	return true;
}

/**
* @brief	Crea las Colas para cada Dispositivo donde recibiran
*			los requisitos para efectuar operaciones de escritura
* @return	verdadero	Creacion Satisfactoria
*/
bool configuracion::crear_colas(){
	string nombre_cola;

	for( int i = 0; i < n_equipos; i++){
		nombre_cola = "/" + string(equipos[i].nombre_marca_id_oyte_eqpo);
		equipos[i].qd = mq_open(nombre_cola.c_str(), O_CREAT | O_RDONLY | O_NONBLOCK, 00666, NULL );
		if(equipos[i].qd == (mqd_t)-1){
			mensaje_error = "Error Abriendo Cola de Mensajes: " + nombre_cola + "\n" +
					string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		struct mq_attr atributos;
		if( mq_getattr( equipos[i].qd, &atributos ) < 0 ){
			mensaje_error = "Error Obteniendo Atributos de Cola de Mensajes: " + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
#ifdef _DEBUG_
		cout << "COLA: " << equipos[i].qd << "\tMAX MENSAJES: " << atributos.mq_maxmsg <<
				"\tTAM MENSAJES: " << atributos.mq_msgsize << "\tMENSAJES ACTUALES: " <<
				atributos.mq_curmsgs << endl;
#endif
	}
	return true;
}



}

#endif // ETHIP_PS_H_
