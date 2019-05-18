/**
* @file	as-datos.cpp
* @brief		<b>Proceso encargado de Producir los Tags y las Alarmas que
*				son enviados a los clientes, este servidor puede correr
*				en la misma maquina que el cliente o puede estar remotamente
*				usando multicast para la comunicacion</b><br>
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

#include <ctime>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "as-util.h"
#include "as-tags.h"
#include "as-thtag.h"
#include "as-shmtag.h"
#include "as-alarma.h"
#include "as-thalarma.h"
#include "as-shmalarma.h"
#include "as-multicast.h"
#include "tinyxml.h"

using namespace std;
using namespace argos;

/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			envia los Tags y Alarmas a los distintos clientes</b><br>
*/
class configuracion{
public:
	char			tags_shmem[100];		//!< Memoria Compartida de Tags en el Servidor
	bool			tags_habilitado;		//!< Habilitacion para Lectura de Tags en el Servidor
	unsigned int	tags_x_msj;				//!< Cantidad de Tags a Enviar en el Mensaje
	unsigned int	tiempo_entre_tags;		//!< Tiempo de Adquisicion de Tags
	char			alarmas_shmem[100];		//!< Memoria Compartida de Alarmas en el Servidor
	bool			alarmas_habilitado;		//!< Habilitacion para Lectura de Alarmas en el Servidor
	unsigned int	alarmas_x_msj;			//!< Cantidad de Alarmas a Enviar en el Mensaje
	unsigned int	tiempo_entre_alarmas;	//!< Tiempo de Adquisicion de Alarmas
	ulong			tiempo_entre_msj;		//!< Tiempo entre el envio de Mensajes
	mcast			conexion;				//!< Objeto para el Envio de Mensajes MULTICAST

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
	configuracion() : conexion(){
		tags_shmem[0]			= '\0';
		tags_habilitado			= false;
		tags_x_msj				= 0;
		tiempo_entre_msj		= UINT_MAX;
		alarmas_shmem[0]		= '\0';
		alarmas_habilitado		= false;
		alarmas_x_msj			= 0;
		tiempo_entre_alarmas	= UINT_MAX;
		tiempo_entre_msj		= ULONG_MAX;
	}
};



static int	 		verbose_flag;		//!< Modo Verboso
string				mensaje_error;		//!< Mensaje de Error por Pantalla


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool		asignar_opciones(int ,char **);
bool		leer_config_servidor(configuracion *);
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal para el Envio
*			de los Tags y Alarmas a los Consumidores
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char **argv) {

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-datos) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-datos", LOG_INFO, "Iniciando Servicio..." );

	configuracion conf;

	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-datos", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }


	leer_config_servidor(&conf);

	shm_thtag shm_t( conf.tags_shmem );
	shm_thalarma shm_a( conf.alarmas_shmem );

	shm_t.obtener_shm_tablahash();
	shm_a.obtener_shm_tablahash();

	conf.conexion.hacer_servidor(true);
	conf.conexion.iniciar_mcast();


	conf.tags_x_msj = conf.tags_x_msj > _MAX_TAGS_X_MSG_ ?
						_MAX_TAGS_X_MSG_ : conf.tags_x_msj;

	Ttags_portable arreglo[_MAX_TAM_ARR_TAGS_];
	unsigned int ntags;

	conf.alarmas_x_msj = conf.alarmas_x_msj > _MAX_ALARM_X_MSG_ ?
						_MAX_ALARM_X_MSG_ : conf.alarmas_x_msj;
	Talarmas_portable arreglo2[_MAX_TAM_ARR_ALARM_];
	unsigned int nalarm;

	while(true){
		ntags = _MAX_TAM_ARR_TAGS_;
		bzero(&arreglo[0],sizeof(arreglo));
		shm_t.obtener_tags_portable(arreglo,&ntags);
		conf.conexion.enviar_tags(arreglo,ntags,conf.tags_x_msj,
				conf.tiempo_entre_tags);

		nalarm = _MAX_TAM_ARR_ALARM_;
		bzero(&arreglo2[0],sizeof(arreglo2));
		shm_a.obtener_alarmas_portable(arreglo2,&nalarm);
		conf.conexion.enviar_alarmas(arreglo2,nalarm,conf.alarmas_x_msj,
				conf.tiempo_entre_alarmas);
		usleep(conf.tiempo_entre_msj);
	}

	shm_t.cerrar_shm_tablahash();
	shm_a.cerrar_shm_tablahash();

return 1;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
* @param	argc		Cantidad de Argumentos
* @param	argv		Lista de Argumentos
* @return	verdadero	Opciones validas
*/
bool asignar_opciones(int argc, char **argv) {
    int c;
    while (1) {
        static struct option long_options[] = {
            {"verbose", no_argument, &verbose_flag, 1},
            {"brief",   no_argument, &verbose_flag, 0},
            {"rec-all",	no_argument,		0, 'r'},
            {"conf",		required_argument,	0, 'c'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rs:c:",long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 0:
            if (long_options[option_index].flag != 0)
                break;
            cout << "Opcion " << long_options[option_index].name;
            if (optarg)
                cout << " con arg " << optarg;
            cout << endl;
            break;
        case 'r':
#ifdef _DEBUG_
            cout <<"r"<<endl;
#endif
            break;
        case 'c':
            strcpy(__ARCHIVO_CONF_SERV_DATOS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion del Servidor de Datos: " <<
				__ARCHIVO_CONF_SERV_DATOS << endl;
#endif
            break;
        case '?':
            break;
        default:
            return false;
        }
    }

    if (verbose_flag){
        cout << "Modo Verbose" << endl;
		cout << "\tProyecto Argos (as-datos) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
		cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
    }
    if (optind < argc) {
        cout << "Opciones no Reconocidas: ";
        while (optind < argc)
            cout << argv[optind++] << " ";
        cout << endl;
        return false;
    }
    return true;
}


/**
* @brief 	Construye la Configuracion a partir de un archivo XML
* @param	conf		Atibutos configurables del proceso
* @return	verdadero	Configuracion exitosa
*/
bool leer_config_servidor( configuracion * conf ){
	TiXmlDocument doc(__ARCHIVO_CONF_SERV_DATOS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " +
				string(__ARCHIVO_CONF_SERV_DATOS) + "\n" + string(strerror(errno));
		escribir_log( "as-datos", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "origen") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <origen>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <origen>");
    }

    TiXmlElement * sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "tags") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tags>");
    }
	strcpy( conf->tags_shmem, sub_ele->GetText() );
    TiXmlAttribute * attr = sub_ele->FirstAttribute();
    if( !attr || strcmp(attr->Name(), "habilitado") != 0){
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags> -> habilitado" );
		throw runtime_error("Error Parseando Archivo de Configuracion <tags> -> habilitado");
	}
	conf->tags_habilitado = ( atoi(attr->Value()) == 1 );
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"tags_x_mensaje") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags_x_mensaje>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tags_x_mensaje>");
    }
    conf->tags_x_msj = (unsigned int) atoi(sub_ele->GetText());
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"tiempo_entre_tags") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_entre_tags>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_entre_tags>");
    }
    conf->tiempo_entre_tags = (unsigned int) atoi(sub_ele->GetText());
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "alarmas") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <alarmas>");
    }
	strcpy( conf->alarmas_shmem, sub_ele->GetText() );
    attr = sub_ele->FirstAttribute();
    if( !attr || strcmp(attr->Name(), "habilitado") != 0){
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas> -> habilitado" );
		throw runtime_error("Error Parseando Archivo de Configuracion <alarmas> -> habilitado");
	}
	conf->alarmas_habilitado = ( atoi(attr->Value()) == 1 );
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"alarmas_x_mensaje") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas_x_mensaje>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <alarmas_x_mensaje>");
    }
    conf->alarmas_x_msj = (unsigned int) atoi(sub_ele->GetText());
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"tiempo_entre_alarmas") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_entre_alarmas>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_entre_alarmas>");
    }
    conf->tiempo_entre_alarmas = (unsigned int) atoi(sub_ele->GetText());
	ele = ele->NextSiblingElement();
	if (!ele || strcmp(ele->Value(),"tiempo_entre_mensajes") != 0) {
		escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_entre_mensajes>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_entre_mensajes>");
    }
    conf->tiempo_entre_msj = (ulong) atol(ele->GetText());
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(), "multicast") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <multicast>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <multicast>");
    }
    sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "puerto") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <puerto>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <puerto>");
    }
    conf->conexion.asignar_puerto( atoi(sub_ele->GetText()) );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"grupo") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupo>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
    }
	conf->conexion.asignar_grupo( sub_ele->GetText() );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"loop") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <loop>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <loop>");
    }
	conf->conexion.asignar_loop( static_cast<short> (atoi(sub_ele->GetText())) );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"ttl") != 0) {
    	escribir_log( "as-datos", LOG_ERR, "Error Parseando Archivo de Configuracion -> <ttl>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <ttl>");
    }
	conf->conexion.asignar_ttl( static_cast<short> (atoi(sub_ele->GetText())) );

    return true;
}
