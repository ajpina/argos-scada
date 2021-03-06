/**
* @file	as-clx_ethip_ps.cpp
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

#include <iostream>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <signal.h>

#include "as-ethip_ps.h"
#include "tinyxml.h"
#include "as-util.h"
#include "as-registro.h"


using namespace std;
using namespace argos;

char					__OYENTE[5];		//!< Varios Dispositivos pueden asociarse a un solo Manejador
static int				verbose_flag;		//!< Modo Verboso
int 					sock;				//!< Socket para establecer las conexiones

wpConf::configuracion	conf;				//!< Objeto de Configuracion de ssCLX_ETHIP_PS


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool 		asignar_opciones(int, char **);
bool 		construir_shm_registros();
void 		actualizar_registros( arg_hilo * );
void 		manejador_senales( int  );
bool 		registrar_manejador_senales( sigset_t * );
void 		escribir_a_dispositivo();
bool 		habilitado_escritura( int, registro, int & );
bool 		enviar_mensaje_tcp( char *, int, struct sockaddr_in );
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char **argv){

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-clx_ethip_ps) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-clx_ethip_ps", LOG_INFO, "Iniciando Servicio..." );

	//-----------------Opciones de Configuracion
	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

	//Cargar las opciones desde el archivo de configuracion
	construir_shm_registros();

	//Ordenar los Registros para crear la SHMEM
	conf.ordenar_registros();

	//------------------Creacion de la Memoria Compartida para
	//------------------cada dispositivo del grupo
    conf.crear_memorias_compartidas();

    //------------------Creacion de la Cola de Mensajes para
	//------------------cada dispositivo del grupo
    conf.crear_colas();

	//--------------------Registro del Manejador de Senales
	sigset_t set, pendset;
	registrar_manejador_senales(&set);


	//------------------Comenzar a Recibir los Mensajes
	struct sockaddr_in direccion;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( sock < 0 ){
		mensaje_error = "Error Abriendo Socket: " + string( strerror( errno ) );
		escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}
	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(conf.puerto);
	direccion.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock, (struct sockaddr *) &direccion, sizeof(direccion)) < 0) {
		mensaje_error = "Error Enlazando Socket: " + string( strerror( errno ) );
		escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}

	int 			longitud = sizeof(direccion),
					cnt;
	char 			msg[_LONG_MSG_UDP_],
					*ip_msg;
	pthread_t 		hilo_mensajes,
					hilo_escritura;
	arg_hilo 		argumentos;

	conf.long_mensaje = conf.long_mensaje > _LONG_MSG_UDP_ ?
			_LONG_MSG_UDP_ : conf.long_mensaje;

	//-----------------Arrancar el hilo para la Escritura en el Dispositivo
	if( pthread_create( &hilo_escritura, NULL, (void *(*)(void *))escribir_a_dispositivo,
			NULL ) ){
		mensaje_error = "Error Creando Hilo para Escribir en Dispositivo: "
				+ string( strerror( errno ) );
		escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
	}
	pthread_detach( hilo_escritura );

	//---------------Bucle 4ever para capturar los mensajes de los
	//---------------dispositivos
	while(true){
		cnt = recvfrom(sock, &msg[0], conf.long_mensaje, 0,
					(struct sockaddr *) &direccion, (socklen_t *) &longitud);
		if( cnt < 0 ){
			mensaje_error = "Error Recibiendo  Mensaje: "
					+ string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		}
		ip_msg = inet_ntoa( direccion.sin_addr );
		argumentos.pos = conf.obtener_indice_equipo( ip_msg );
		if( argumentos.pos < 0 ){
#ifdef _DEBUG_
			cout << "Mensaje Recibido de Direccion Desconocida: " << ip_msg << endl;
#endif
			escribir_log( "as-clx_ethip_ps", LOG_WARNING,
					string( "Mensaje Recibido de Direccion Desconocida: " + string(ip_msg)).c_str() );
		}
		else{
			argumentos.n_registros = conf.mtx_registros[ argumentos.pos ].size();
			argumentos.ptr_msg = msg;

			//----------------Arrancar el Hilo para Actualizar la Memoria Compartida
			//----------------de Registros
			if( pthread_create( &hilo_mensajes, NULL, (void *(*)(void *))actualizar_registros,
					&argumentos ) ){
				mensaje_error = "Error Creando Hilo para Actualizar Registros: "
						+ string( strerror( errno ) );
				escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			}
			pthread_detach( hilo_mensajes );
		}

		//--------------Desbloquear y Manejar las Senales Pendientes
		sigpending( &pendset );
		if( sigismember(&pendset, SIGHUP ) || sigismember(&pendset, SIGINT ) ||
				sigismember(&pendset, SIGQUIT ) || /*sigismember(&pendset, SIGTERM ) ||*/
				sigismember(&pendset, SIGUSR1 ) || sigismember(&pendset, SIGUSR2 )
			){
			pthread_sigmask (SIG_UNBLOCK, &set, NULL);
		}
	}
	escribir_log( "as-clx_ethip_ps", LOG_ERR, "Finalizacion Anormal....." );
	for( int i = 0; i < conf.n_equipos; i++ ){
		if( close(conf.equipos[i].fd) == -1 ){
			mensaje_error = "Error Cerrando SHMEM: "
						+ string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_INFO, mensaje_error.c_str() );
		}
		if( close(conf.equipos[i].qd) == -1 ){
			mensaje_error = "Error Cerrando Cola: "
						+ string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_INFO, mensaje_error.c_str() );
		}
	}
	if( close(sock) == -1 ){
		mensaje_error = "Error Cerrando Socket: "
						+ string( strerror( errno ) );
		escribir_log( "as-clx_ethip_ps", LOG_INFO, mensaje_error.c_str() );
	}
	return 1;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
* @param	argc		Cantidad de Argumentos
* @param	argv		Lista de Argumentos
* @return	verdadero	Asignaciones Validas
*/
bool asignar_opciones(int argc, char **argv) {
    int c;
    while (1) {
        static struct option long_options[] = {
            {"verbose", no_argument, &verbose_flag, 1},
            {"brief",   no_argument, &verbose_flag, 0},
            {"dsptvo",		required_argument,	0, 'd'},
            {"oyte",		required_argument,	0, 'o'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rs:d:o:",long_options, &option_index);
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
        case 'o':
			strcpy(__OYENTE, optarg);
#ifdef _DEBUG_
            cout << "Proceso Iniciado, Como <oyente id='" <<
				__OYENTE << "'>" << endl;
#endif
            break;
        case 'd':
            strcpy(__ARCHIVO_CONF_DEVS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Dispositivos: " << __ARCHIVO_CONF_DEVS << endl;
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
		cout << "\tProyecto Argos (as-clx_ethip_ps) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @brief	Construye la SHMEM de Registros partir de un archivo XML de
*			Configuracion
* @return	verdadero	Construccion Exitosa
*/
bool construir_shm_registros() {
    TiXmlDocument doc(__ARCHIVO_CONF_DEVS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_DEVS) + "\n" + string(strerror(errno));
        escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    //string nombre = ele->GetText();

	ele = ele->NextSiblingElement();

	char 			id[10],
					id2[10],
					modelo[100],
					nombre[100];
	bool 			habilitado;
	int 			puerto,
					longitud,
					cantidad2,
					ind;
	TiXmlElement 	*sub_ele,
					*sub_sub_ele,
					*sub3_ele,
					*sub4_ele;
	TiXmlAttribute 	*attr;

	for (int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
		if (!ele || strcmp(ele->Value(),"dispositivo") != 0) {
			escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <dispositivo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <dispositivo>");
		}
		attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
		}
		strcpy(id, attr->Value());
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
		}
		strcpy(modelo, attr->Value());
		if( strcmp(modelo,"AB") != 0 ){
			continue;
		}
		else{
			sub_ele = ele->FirstChildElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
			}
			strcpy(nombre, sub_ele->GetText());
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "marca") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <marca>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <marca>");
			}
			//strcpy(dispositivo.marca,sub_ele->GetText());
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "tipo") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
			}
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "habilitado") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <habilitado>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <habilitado>");
			}
			habilitado = (bool) atoi( sub_ele->GetText() );
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "comunicacion") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <comunicacion>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <comunicacion>");
			}
			sub_sub_ele = sub_ele->FirstChildElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "tipo") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
			}
			if(strcmp("PaS", sub_sub_ele->GetText() ) != 0){
				continue;
			}
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "direccion") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <direccion>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <direccion>");
			}
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "ruta") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <ruta>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <ruta>");
			}
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "nodo") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nodo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nodo>");
			}
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "grupos") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupos>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <grupos>");
			}
			sub_sub_ele = sub_ele->FirstChildElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "oyente") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <oyente>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <oyente>");
			}
			attr = sub_sub_ele->FirstAttribute();
			if( !attr || strcmp(attr->Name(), "id") != 0){
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente> -> id" );
				throw runtime_error("Error Obteniendo Atributos de <oyente> -> id");
			}
			strcpy(id2, attr->Value());
			if(strcmp(id2, __OYENTE) != 0 ){
				continue;
			}
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "puerto") != 0){
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente> -> puerto" );
				throw runtime_error("Error Obteniendo Atributos de <oyente> -> puerto");
			}
			puerto = atoi( attr->Value() );
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "longitud") != 0){
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente> -> longitud" );
				throw runtime_error("Error Obteniendo Atributos de <oyente> -> longitud");
			}
			longitud = atoi( attr->Value() );
			sub3_ele = sub_sub_ele->FirstChildElement();
			if (!sub3_ele || strcmp(sub3_ele->Value(), "cantidad") != 0) {
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <oyente><cantidad>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <oyente><cantidad>");
			}
			cantidad2 = atoi( sub3_ele->GetText() );
			if( cantidad2 > 0 ){
				conf.asignar_n_equipos( cantidad2 );
			}
			conf.habilitado 		= habilitado;
			conf.puerto 			= puerto;
			conf.long_mensaje 		= longitud;

			sub3_ele = sub3_ele->NextSiblingElement();
			for( int j = 0; j < cantidad2; j++, sub3_ele = sub3_ele->NextSiblingElement() ){
				if (!sub3_ele || strcmp(sub3_ele->Value(), "equipo") != 0) {
					escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Parseando Archivo de Configuracion -> <oyente><equipo>" );
					throw runtime_error("Error Parseando Archivo de Configuracion -> <oyente><equipo>");
				}
				attr = sub3_ele->FirstAttribute();
				if( !attr || strcmp(attr->Name(), "nombre") != 0){
					escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente><equipo> -> nombre" );
					throw runtime_error("Error Obteniendo Atributos de <oyente><equipo> -> nombre");
				}
				string tmp = string(modelo) + "_" + string(id) + "_" +
					string(nombre) + "_" + string(id2) + "_" + string( attr->Value() );
				strcpy( conf.equipos[j].nombre_marca_id_oyte_eqpo, tmp.c_str());
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "ip") != 0){
					escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente><equipo> -> ip" );
					throw runtime_error("Error Obteniendo Atributos de <oyente><equipo> -> ip");
				}
				strcpy( conf.equipos[j].ip, attr->Value() );
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "puerto_escritura") != 0){
					escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <oyente><equipo> -> puerto_escritura" );
					throw runtime_error("Error Obteniendo Atributos de <oyente><equipo> -> puerto_escritura");
				}
				conf.equipos[j].puerto_escritura = atoi( attr->Value() );
				sub4_ele = sub3_ele->FirstChildElement();
				ind = 0;
				registro actual;
				while( sub4_ele && strcmp(sub4_ele->Value(), "registro") == 0 ){
					if(ind > _MAX_CANT_REGS_){
						escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Superada la Cantidad Maxima de Registros Permitidos" );
						throw runtime_error("Error Superada la Cantidad Maxima de Registros Permitidos");
					}
					attr = sub4_ele->FirstAttribute();
					if( !attr || strcmp(attr->Name(), "direccion") != 0){
						escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <registro> -> direccion" );
						throw runtime_error("Error Obteniendo Atributos de <registro> -> direccion");
					}
					strcpy( conf.equipos[j].registros[ind].direccion, attr->Value());
					strcpy(actual.direccion, attr->Value());
					attr = attr->Next();
					if( !attr || strcmp(attr->Name(), "tipo") != 0){
						escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tipo" );
						throw runtime_error("Error Obteniendo Atributos de <registro> -> tipo");
					}
					TEXTO2DATO(conf.equipos[j].registros[ind].tipo, attr->Value());
					TEXTO2DATO(actual.tipo, attr->Value() );
					actual.valor.L = _ERR_DATO_;
					conf.mtx_registros[j].push_back( actual );
					attr = attr->Next();
					if( !attr || strcmp(attr->Name(), "indice") != 0){
						escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <registro> -> indice" );
						throw runtime_error("Error Obteniendo Atributos de <registro> -> indice");
					}
					conf.equipos[j].registros[ind].indice = atoi( attr->Value() );
					attr = attr->Next();
					if( !attr || strcmp(attr->Name(), "longitud") != 0){
						escribir_log( "as-clx_ethip_ps", LOG_ERR, "Error Obteniendo Atributos de <registro> -> longitud" );
						throw runtime_error("Error Obteniendo Atributos de <registro> -> longitud");
					}
					conf.equipos[j].registros[ind].longitud = atoi( attr->Value() );
					attr = attr->Next();
					if( attr && strcmp(attr->Name(), "bit") == 0){
						conf.equipos[j].registros[ind].bit = atoi( attr->Value() );
						attr = attr->Next();
					}
					if( attr && strcmp(attr->Name(), "escritura") == 0){
						conf.equipos[j].registros[ind].escritura = ( atoi( attr->Value() ) == 1 ) ?
							true : false;
					}
					ind++;
					sub4_ele = sub4_ele->NextSiblingElement();
				}
			}

			break;
		}
	}

    return true;
}

/**
* @brief	Mantiene Actualizada la Memoria Compartida de Registros
*			con los valores recibidos de los Dispositivos
* @param	arg		Estructura que encapsula los Mensajes
*/
void actualizar_registros( arg_hilo * arg ){
	char	msg_local[_LONG_MSG_UDP_];
	char	estatus;
	void	*dest, *src;
	int		pqtes_perdidos;

	memcpy( msg_local, arg->ptr_msg, _LONG_MSG_UDP_ );
	pqtes_perdidos = (short)msg_local[0] - conf.equipos[arg->pos].cuenta_pqte - 1;
	conf.equipos[arg->pos].cuenta_pqte = (int)msg_local[0];
	if( pqtes_perdidos > 0 ){
#ifdef _DEBUG_
		cout << "\nMensajes Perdidos: " << pqtes_perdidos << "\n";
#endif
		escribir_log( "as-clx_ethip_ps", LOG_WARNING, string( "Mensajes Perdidos: " +
				numero2string( pqtes_perdidos, std::dec ) ).c_str() );
	}

	for( int j = 0; j < arg->n_registros; j++){
		dest = (void *)(conf.equipos[arg->pos].shm + conf.equipos[arg->pos].registros[j].shm_pos);
		src = (void *)&msg_local[conf.equipos[arg->pos].registros[j].indice + 1];
		estatus = msg_local[conf.equipos[arg->pos].registros[j].indice];

		//Verficar que el registro a cambiado para proceder a actualizar
		//la memoria compartida
		if( !(estatus & __BIT2HEX[0]) ){
			continue;
		}

#ifdef _DEBUG_
		cout << conf.equipos[arg->pos].registros[j].direccion << "\t";
#endif
		switch(conf.equipos[arg->pos].registros[j].tipo){
				case CARAC_:
					memcpy( &(((registro *)dest)->valor.C), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << static_cast<palabra_t> (((registro *)dest)->valor.C)  << endl;
#endif
				break;
				case CORTO_:
					memcpy( &(((registro *)dest)->valor.S), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.S << endl;
#endif
				break;
				case ENTERO_:
					memcpy( &(((registro *)dest)->valor.I), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.I << endl;
#endif
				break;
				case LARGO_:
					memcpy( &(((registro *)dest)->valor.L), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.L << endl;
#endif
				break;
				case REAL_:
					memcpy( &(((registro *)dest)->valor.F), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.F << endl;
#endif
				break;
				case DREAL_:
					memcpy( &(((registro *)dest)->valor.D), src, conf.equipos[arg->pos].registros[j].longitud );
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.D << endl;
#endif
				break;
				case BIT_:
					memcpy( &(((registro *)dest)->valor.I), src, conf.equipos[arg->pos].registros[j].longitud );
					((registro *)dest)->valor.B = (bool) (((registro *)dest)->valor.I &
							__BIT2HEX[conf.equipos[arg->pos].registros[j].bit]);
#ifdef _DEBUG_
					cout << "valor " << ((registro *)dest)->valor.B << endl;
#endif
				break;
				case TERROR_:
				default:
#ifdef _DEBUG_
					cout << "valor Desconocido" << endl;
#endif
					escribir_log( "as-clx_ethip_ps", LOG_WARNING, "Valor Desconocido..." );
					continue;
		}
	}
}

/**
* @brief	Maneja las Senales que recibe el proceso
*			para cerrar correctamente la comunicacion con
*			los dispositivos
* @param	signum		Senal
*/
void manejador_senales( int signum ){
#ifdef _DEBUG_
	cout << "Senal Captura[" << signum << "]... Finalizando Proceso" << endl;
#endif
	escribir_log( "as-clx_ethip_ps", LOG_INFO, string("Finalizando Proceso. Senal Recivida: " +
			numero2string(signum, std::dec)).c_str() );
	for( int i = 0; i < conf.n_equipos; i++ ){
		if( close(conf.equipos[i].fd) == -1 ){
			mensaje_error = "Error Cerrando SHMEM: "
						+ string( strerror( errno ) );
			cout << mensaje_error.c_str() << endl;
		}
	}
	if( close(sock) == -1 ){
		mensaje_error = "Error Cerrando Socket: "
						+ string( strerror( errno ) );
		cout << mensaje_error.c_str() << endl;
	}
	escribir_log( "as-clx_ethip_ps", LOG_INFO, "Terminando Servicio..." );
	exit(EXIT_SUCCESS);
}

/**
* @brief	Hace un registro de las Senales que seran
*			Manejadas dentro del proceso
* @param	set			Conjunto de senales
* @return	verdadero	Registro Exitoso
*/
bool registrar_manejador_senales(sigset_t * set){
	struct sigaction n_accion;
	n_accion.sa_flags = 0;
	sigemptyset( &n_accion.sa_mask );
	n_accion.sa_handler = manejador_senales;
	sigaction( SIGHUP, &n_accion, NULL );
	sigaction( SIGINT, &n_accion, NULL );
	sigaction( SIGQUIT, &n_accion, NULL );
	//sigaction( SIGTERM, &n_accion, NULL );
	sigaction( SIGUSR1, &n_accion, NULL );
	sigaction( SIGUSR2, &n_accion, NULL );
	sigaction( SIGALRM, &n_accion, NULL );
	sigemptyset( set );
	sigaddset( set, SIGHUP );
	sigaddset( set, SIGINT );
	sigaddset( set, SIGQUIT );
	//sigaddset( set, SIGTERM );
	sigaddset( set, SIGUSR1 );
	sigaddset( set, SIGUSR2 );
	sigaddset( set, SIGALRM );
	//sigprocmask( SIG_BLOCK, set, NULL );
	pthread_sigmask (SIG_BLOCK, set, NULL);

	return true;
}

/**
* @brief	Tiempo Consumido en el envio del Mensaje de
*			escritura al dispositivo
* @param	sig			Senal ALARM
*/
void timeout_ack(int sig){
	cout << "Timeout. Signal: " << sig << endl;
	cout.flush();
}

/**
* @brief	Envia el Mensaje para la Escritura de un
*			valor en el dispositivo
*/
void escribir_a_dispositivo(){
	int				ind = 0,
					nbytes,
					posicion = -1,
					sckt[_MAX_CANT_DISP_X_DRVE_];
	registro		mensaje;
	unsigned int	prio;
	char			buffer[_LONG_MSG_ENV_UDP_],
					*ptr_buffer;

	struct sockaddr_in	pin,
						pin2;
	struct hostent		*servidor;

	for( int i = 0; i < _MAX_CANT_DISP_X_DRVE_ && i < conf.n_equipos; i++ ){

		if( ( servidor = gethostbyname( conf.equipos[i].ip ) ) == 0 ){
			mensaje_error = "[HILO]Obteniendo Nombre del Servidor "
				+ string( conf.equipos[i].ip )
				+ "\n" + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		}

		bzero( &pin, sizeof( pin ) );
		pin.sin_family = AF_INET;
		pin.sin_addr.s_addr = htonl( INADDR_ANY );
		pin.sin_addr.s_addr = ((struct in_addr *)(servidor->h_addr))->s_addr;
		pin.sin_port = htons( conf.equipos[i].puerto_escritura );

		bzero( &pin2, sizeof( pin2 ) );
		pin2.sin_family = AF_INET;
		pin2.sin_addr.s_addr = htonl( INADDR_ANY );
		pin2.sin_port = htons( conf.equipos[i].puerto_escritura );

		if( (sckt[i] = socket( AF_INET, SOCK_DGRAM, 0 ) ) == -1 ){
			mensaje_error = "[HILO]Obteniendo Socket\n" + string( strerror( errno ) ) +
						"\nEquipo: " + string(conf.equipos[i].ip) + "\tPuerto: " +
						numero2string<int>( conf.equipos[i].puerto_escritura, dec );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		}
		if( bind( sckt[i], ( struct sockaddr * )&pin2, sizeof( pin2 ) ) < 0 ){
			mensaje_error = "[HILO]Enlazando Socket\n" + string( strerror( errno ) ) +
						"\nEquipo: " + string(conf.equipos[i].ip) + "\tPuerto: " +
						numero2string<int>( conf.equipos[i].puerto_escritura, dec );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
		}
	}


	struct sigaction	action;
	sigset_t			set;

	action.sa_handler = timeout_ack;
	action.sa_flags = 0;
	sigemptyset(&(action.sa_mask));
	sigaction(SIGALRM,&action,NULL);
	sigemptyset( &set );
	sigaddset( &set, SIGALRM );
	pthread_sigmask (SIG_UNBLOCK, &set, NULL);

	while( true ){
		// Esperar 1 seg entre cada operacion de escritura
		usleep( 1000000 );

		if( -1 == ( nbytes = mq_receive( conf.equipos[ind].qd,
								(char *)&mensaje, 8192, &prio) ) ){
			// Si no hay mensajes en la Cola, se detiene la ejecucion
			// por 500 milisegundos y luego se inicia el siguiente ciclo
			if( errno == EAGAIN ){
				usleep( 500000 );
				ind++;
				if( ind == conf.n_equipos ){
					ind = 0;
				}
				continue;
			}
			else{
				mensaje_error = "[HILO]Error Recibiendo Mensaje de Cola: " + string( strerror( errno ) );
				escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
				usleep( 500000 );
				continue;
			}

		}
#ifdef _DEBUG_
		cout << "Escribiendo Registro:\t" << mensaje << endl;
#endif

		if( !habilitado_escritura( ind, mensaje, posicion ) ){
#ifdef _DEBUG_
			cout << "Imposible Escribir\nRegistro No esta habilitado para escritura" << endl;
#endif
			escribir_log( "as-clx_ethip_ps", LOG_ERR, "[HILO]Imposible Escribir\nRegistro No esta habilitado para escritura" );
			ind++;
			if( ind == conf.n_equipos ){
				ind = 0;
			}
			continue;
		}
		else{
			buffer[conf.equipos[ind].registros[posicion].indice] = __BIT2HEX[0];
			ptr_buffer = &buffer[conf.equipos[ind].registros[posicion].indice + 1];
			switch(conf.equipos[ind].registros[posicion].tipo){
				case CARAC_:
					memcpy( ptr_buffer, &(mensaje.valor.C), conf.equipos[ind].registros[posicion].longitud );
				break;
				case CORTO_:
					memcpy( ptr_buffer, &(mensaje.valor.S), conf.equipos[ind].registros[posicion].longitud );
				break;
				case ENTERO_:
					memcpy( ptr_buffer, &(mensaje.valor.I), conf.equipos[ind].registros[posicion].longitud );
				break;
				case LARGO_:
					memcpy( ptr_buffer, &(mensaje.valor.L), conf.equipos[ind].registros[posicion].longitud );
				break;
				case REAL_:
					memcpy( ptr_buffer, &(mensaje.valor.F), conf.equipos[ind].registros[posicion].longitud );
				break;
				case DREAL_:
					memcpy( ptr_buffer, &(mensaje.valor.D), conf.equipos[ind].registros[posicion].longitud );
				break;
				case BIT_:
					memcpy( ptr_buffer, &(mensaje.valor.B), conf.equipos[ind].registros[posicion].longitud );
				break;
				case TERROR_:
				default:
#ifdef _DEBUG_
					cout << "Imposible Escribir\nTipo de Registro Desconocido" << endl;
#endif
					escribir_log( "as-clx_ethip_ps", LOG_ERR, "[HILO]Imposible Escribir\nTipo de Registro Desconocido" );
					ind++;
					if( ind == conf.n_equipos ){
						ind = 0;
					}
					continue;
			}
			enviar_mensaje_tcp( buffer, sckt[ind], pin );
		}

		ind++;
		if( ind == conf.n_equipos ){
			ind = 0;
		}
	}

	for( int i = 0; i < _MAX_CANT_DISP_X_DRVE_ && i < conf.n_equipos; i++ ){
		close( sckt[i] );
	}

}

/**
* @brief	Verifica si el registro que se intenta escribir
*			esta habilitado para efectuar la operacion
* @param	indice		Posicion del Dispositivo en el Vector
* @param	reg			Datos del Registro
* @param	pos			Posicion del Registro dentro del Vector
* @return	verdadero	habilitado para escritura
*/
bool habilitado_escritura( int indice, registro reg, int & pos ){
	for( size_t i = 0; i < conf.mtx_registros[indice].size(); i++ ){
		if( strcmp( conf.equipos[indice].registros[i].direccion,
					reg.direccion ) == 0 ){

			pos = i;
			return conf.equipos[indice].registros[i].escritura;
		}
	}
	pos = -1;
	return false;
}

/**
* @brief	Establece la conexion con el dispositivo para
*			enviarle el dato que requiere escribir
* @param	buf			Mensaje
* @param	sckt		Socket con el dispositivo
* @param	pin			Datos de la Conexion
* @return	verdadero	Envio Exitoso
*/
bool enviar_mensaje_tcp( char * buf, int sckt, struct sockaddr_in pin ){

	bool				envio_ok = false,
						rec_ok = false;
	int					intentos = 0,
						longitud = sizeof( struct sockaddr );
	char				rec_buf[_LONG_MSG_ENV_UDP_];
	struct sockaddr_in	rec_pin;


	for( intentos = 0; intentos < _MAX_NRO_INTENTOS_ESC_; intentos++){
		envio_ok = rec_ok = true;
		if( sendto( sckt, buf, _LONG_MSG_ENV_UDP_, 0, (struct sockaddr *)&pin, sizeof(pin) ) < 0 ){
			envio_ok = false;
			mensaje_error = "ERROR Intento[" + numero2string(intentos, std::dec) + "]\nEnviando Tag\n" + string( strerror( errno ) );
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			continue;
		}

		alarm(2);
		if( recvfrom( sckt, rec_buf, _LONG_MSG_ENV_UDP_, 0, (struct sockaddr *)&rec_pin, (socklen_t *)&longitud ) < 0 ){
			rec_ok = false;
			if(errno==EINTR){
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "TimeOut Esperando Respuesta" );
			}else{
				escribir_log( "as-clx_ethip_ps", LOG_ERR, "ERROR recvfrom\n" );
			}
			mensaje_error = "Error Recibiendo Acuse: " + string( strerror( errno ) ) +
				"\nIntento[" + numero2string<int>( intentos, dec ) + "]";
			escribir_log( "as-clx_ethip_ps", LOG_ERR, mensaje_error.c_str() );
			continue;
		}
		alarm(0);
		if( envio_ok && rec_ok ){
			if( strncmp( buf, rec_buf, _LONG_MSG_ENV_UDP_ ) != 0 ){
				escribir_log( "as-clx_ethip_ps", LOG_ERR, string("Error en Transmision. Intento: " +
						numero2string(intentos+1, std::dec) ).c_str() );
				envio_ok = rec_ok = false;
				continue;
			}
#ifdef _DEBUG_
			cout << "Comunicacion OK. Intento:" <<intentos+1<< endl;
#endif
		}
		else{
			escribir_log( "as-clx_ethip_ps", LOG_ERR, string("Error Inesperado. Intento: " +
						numero2string(intentos+1, std::dec) ).c_str() );
			continue;
		}
		break;
	}
#ifdef _DEBUG_
	if( envio_ok && rec_ok ){
		cout << "Paquete Escrito Correctamente" << endl;
	}
	else{
		cout << "Imposible Completar Operacion de Escritura" << endl;
	}
#endif
	return true;
}
