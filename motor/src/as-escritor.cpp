/**
* @file	as-escritor.cpp
* @brief		<b>Proceso encargado de enviar un mensaje a los <br>
*				manejadores de comunicacion de los distintos dispositivos
*				para realizar operaciones de escritura</b><br>
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

#include <stdexcept>
#include <cstring>
#include <sstream>
#include <iostream>
#include <ctime>
#include <cmath>
#include <vector>
#include <algorithm>
#include <climits>
#include <map>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mqueue.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <float.h>
#include <signal.h>
#include <netdb.h>

#include "as-util.h"
#include "as-registro.h"
#include "tinyxml.h"
#include "as-shmalarma.h"


#define _MAX_BUFFER_ 16384


using namespace std;
using namespace argos;

static int	 		verbose_flag;		//!< Modo Verboso
string				mensaje_error;		//!< Mensaje de Error por Pantalla

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los argumentos
	al hilo encargado de entablar la conexion con los
	manejadores de comunicacion de los dispositivos
----------------------------------------------------------*/
typedef struct{
	Tmsgsswr	mensaje;				//!< Mensaje de Escritura
	int			sckt;					//!< Socket para conexion con manejador
}arg_hilo;


namespace wrConf{

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los atributos
	del Tag que se desea escribir en el dispositivo
----------------------------------------------------------*/
typedef struct{
	char		nombre_tag[_LONG_NOMB_TAG_];	//!< Nombre del Tag
	Tdato		tipo_tag;						//!< Tipo del Tag
	mqd_t		cola;							//!< Cola del Dispositivo asociado al Tag
	registro	destino;						//!< Registro destino en el Dispositivo
	short		bit;							//!< En el Caso de ser un BIT del Registro
}escritura;


/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			escribe los valores de los Tags en los Dispositivos</b><br>
*/
class configuracion{
public:
	vector <escritura>	tags_escritura;			//!< Vector de Tags Habilitados para Escritura
	int					puerto;					//!< Puerto para escuchar las conexiones
	int					longitud_mensaje;		//!< Tamano del Mensaje escuchado
	int					max_conexiones;			//!< Cantidad de conexiones permitidas
	map <string, mqd_t> colas_dispositivos;		//!< Tabla de Colas de los Distintos Dispositivos
	vector <string>		alarmas_escritura;		//!< Vector de Alarmas Habilitadas para Escritura
	shm_thalarma		shm_alarmas;			//!< Referencia a la SHMEM de Alarmas

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
	configuracion(){
		puerto = 16005;
		longitud_mensaje = _MAX_BUFFER_;
		max_conexiones = 20;
	}

/**
* @brief	Obtiene el Descriptor de la Cola para
*			el Dispositivo
* @param	disp	Nombre del Dispositivo
* @return	cola	Descriptor de la Cola
*/
	mqd_t obtener_cola( string disp ){
		map<string, mqd_t>::iterator ii = colas_dispositivos.find( disp );
		if( ii == colas_dispositivos.end() )
			return (mqd_t) -1;
		return (mqd_t) (*ii).second;
	}

/**
* @brief	Agregar un Tag habilitado para escritura
*			al vector
* @param	nt			Nombre del Tag
* @param	td			Tipo de Dato
* @param	dis			Nombre del Dispositivo
* @param	dir			Direccion del Registro
* @param	tdr			Tipo de Dato del Registro
* @param	bt			En el caso de ser Bit del Registro
* @return	verdadero	Inserccion Exitosa
*/
	bool agregar_tag_escritura( char * nt, Tdato td, string dis, char * dir,
			Tdato tdr, short bt ){
		escritura t;
		strcpy( t.nombre_tag, nt );
		t.tipo_tag = td;
		t.cola = obtener_cola( dis );
		strcpy( t.destino.direccion, dir );
		t.destino.tipo = tdr;
		t.bit = bt;
		tags_escritura.push_back( t );
		return true;
	}

/**
* @brief	Abre las Colas de los Distintos Dispositivos
*			de Comunicacion
* @return	verdadero	Apertura Exitosa
*/
	bool abrir_colas(){
		mqd_t 			id_cola;
		string			nombre_cola;

		for( map<string, mqd_t>::iterator ii=colas_dispositivos.begin();
				ii!=colas_dispositivos.end(); ++ii){

			nombre_cola = "/" + (*ii).first;
			id_cola = mq_open(nombre_cola.c_str(), O_WRONLY | O_NONBLOCK, 00666, NULL );
			if(id_cola == (mqd_t)-1){
				mensaje_error = "Error Abriendo Cola de Mensajes: " + nombre_cola + "\n" +
						string( strerror( errno ) );
				escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error( mensaje_error.c_str() );
			}

			struct mq_attr atributos;

			if( mq_getattr( id_cola, &atributos ) < 0 ){
				mensaje_error = "Error Obteniendo Atributos de Cola de Mensajes: " + string( strerror( errno ) );
				escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error( mensaje_error.c_str() );
			}

			(*ii).second = id_cola;

#ifdef _DEBUG_
			cout << "\t" << (*ii).first << ": " << (*ii).second << "\n";
			cout << "Abriendo Cola......... " << endl <<
				"Mensajes Actuales:\t" << atributos.mq_curmsgs << endl <<
				"Cantidad Maxima Mensajes:\t" << atributos.mq_maxmsg << endl <<
				"Longitud de Mensajes:\t" << atributos.mq_msgsize << "\n\n";
#endif
		}
		return true;
	}

/**
* @brief	Cerrar las Colas de los Distintos Dispositivos
*			de Comunicacion
* @return	verdadero	Cierre Exitoso
*/
	bool cerrar_colas(){
		for( map<string, mqd_t>::iterator ii=colas_dispositivos.begin();
				ii!=colas_dispositivos.end(); ++ii){

			close((*ii).second);
		}
		return true;
	}

};

}


wrConf::configuracion conf;				//!< Objeto de Configuracion de ssESCRITOR


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool		asignar_opciones(int, char**);
bool 		leer_config_dispositivos( int & );
bool 		leer_config_tags();
bool 		leer_config_alarmas();
bool 		leer_config_servidor();
void 		enviar_a_cola(arg_hilo *);
bool 		comparacion_escritura( wrConf::escritura, wrConf::escritura );
void 		actualizar_alarma(arg_hilo *);
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char ** argv){

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-escritor) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-escritor", LOG_INFO, "Iniciando Servicio..." );

	//-----------------Opciones de Configuracion
	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }


    int			cant_dispositivos;
	leer_config_dispositivos(cant_dispositivos);

#ifdef _DEBUG_
	cout << "Cantidad de Dispositivos: " << conf.colas_dispositivos.size() << endl;
#endif

	conf.abrir_colas();
	leer_config_tags();
	sort(conf.tags_escritura.begin(), conf.tags_escritura.end(), comparacion_escritura);

#ifdef _DEBUG_
	for( size_t i=0; i < conf.tags_escritura.size(); i++ ){
		cout << "TAG: " << conf.tags_escritura[i].nombre_tag << "\tCOLA: "
			<< conf.tags_escritura[i].cola << "\tDESTINO: "
			<< conf.tags_escritura[i].destino.direccion << endl;
	}
#endif

	leer_config_alarmas();
	sort(conf.alarmas_escritura.begin(), conf.alarmas_escritura.end());

#ifdef _DEBUG_
	for( size_t i=0; i < conf.alarmas_escritura.size(); i++ ){
		cout << "ALARMA: " << conf.alarmas_escritura[i] << endl;
	}
#endif


	leer_config_servidor();

	//Crear Servidor TCP/IP

	//Esperar por requisito de escritura o reconocimiento de alarma

	//Cuando llega la solicitud levantar un hilo


	struct sockaddr_in		sin,
							pin;
	int						sckt;
	socklen_t				addr_size;
	pthread_t				hilo;
	arg_hilo				msj_hilo;

	sckt = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( -1 == sckt ){
		mensaje_error = "Error Abriendo Socket: " + string( strerror( errno ) );
		escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	bzero( &sin, sizeof( sin ) );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl( INADDR_ANY );
	sin.sin_port = htons( conf.puerto );

	if( bind( sckt, (struct sockaddr *)&sin, sizeof( sin )) == -1 ){
		mensaje_error = "Error Enlazando Socket: " + string( strerror( errno ) );
		escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	if( listen( sckt, conf.max_conexiones ) == -1 ){
		mensaje_error = "Error Cantidad de Conexiones por Socket: " + string( strerror( errno ) );
		throw runtime_error(mensaje_error.c_str());
	}

	while( true ){
		addr_size = sizeof( pin );
		msj_hilo.sckt = accept( sckt, (struct sockaddr *)&pin, &addr_size );
		if( -1 == msj_hilo.sckt ){
			mensaje_error = "Error Aceptando Conexiones: " + string( strerror( errno ) );
			escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}

		if( recv( msj_hilo.sckt, (char *)(&msj_hilo.mensaje), sizeof(msj_hilo.mensaje), 0 ) == -1 ){
			mensaje_error = "Error Recibiendo Datos: " + string( strerror( errno ) );
			escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}

		switch( msj_hilo.mensaje.tipo ){
				case _TIPO_ALARMA_:
#ifdef _DEBUG_
					cout << "Iniciando Escritura de Alarma" << endl;
#endif
					if( pthread_create( &hilo, NULL, (void *(*)(void *))actualizar_alarma,
						 &msj_hilo) ){
						mensaje_error = "Error Creando Hilo para Actualizar Alarma: "
								+ string( strerror( errno ) );
						escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
					}
					pthread_detach( hilo );
				break;
				case _TIPO_TAG_:
#ifdef _DEBUG_
					cout << "Iniciando Escritura de Tag" << endl;
#endif
					if( pthread_create( &hilo, NULL, (void *(*)(void *))enviar_a_cola,
						 &msj_hilo) ){
						mensaje_error = "Error Creando Hilo para Actualizar Registros: "
								+ string( strerror( errno ) );
						escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
					}
					pthread_detach( hilo );
				break;
				default:
#ifdef _DEBUG_
					cout << "Tipo Desconocido" << endl;
#endif
					escribir_log( "as-escritor", LOG_WARNING, "Tipo de Mensaje Desconocido" );
				continue;
		}



	}

	conf.cerrar_colas();

	//La cola de mensajes la debe crear el driver y debe ser no-bloqueante.


	//El hilo escribe en la cola de mensajes del driver, si fue exitoso
	//le responde al cliente por el socket TCP




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
            {"rec-all",	no_argument,		0, 'r'},
            {"dsptvo",		required_argument,	0, 'd'},
            {"tags",		required_argument,	0, 't'},
            {"alrms",		required_argument,	0, 'a'},
            {"serv",		required_argument,	0, 's'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rs:d:t:a:s:",long_options, &option_index);
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
        case 'd':
            strcpy(__ARCHIVO_CONF_DEVS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Dispositivos: " <<
				__ARCHIVO_CONF_DEVS << endl;
#endif
            break;
        case 't':
            strcpy(__ARCHIVO_CONF_TAGS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Tags: " <<
				__ARCHIVO_CONF_TAGS << endl;
#endif
            break;
		case 'a':
            strcpy(__ARCHIVO_CONF_ALARMAS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Alarmas: " <<
				__ARCHIVO_CONF_ALARMAS << endl;
#endif
            break;
		case 's':
            strcpy(__ARCHIVO_CONF_SERV_DATOS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion del Servidor: " <<
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
		cout << "\tProyecto Argos (ssescritor) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
*			de los distintos dispositivos conectados de un archivo XML
* @param	cantidad	Cantidad de Dispositivos
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config_dispositivos( int & cantidad ){
	TiXmlDocument doc(__ARCHIVO_CONF_DEVS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_DEVS) + "\n" + string(strerror(errno));
        escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
        escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad1 = atoi( ele->GetText() );
    cantidad = cantidad1;
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    //string nombre = ele->GetText();

	ele = ele->NextSiblingElement();
	TiXmlElement *sub_ele, *sub_sub_ele, *sub3_ele, *sub4_ele;
	TiXmlAttribute * attr;

	int reg_x_disp[20] = {0};				//Almacenamiento temporal (20) mientras
	string marca_id_nombre[20];			//se calcula el tamano para la
										//estructura de dispositivos

	int ind = 0, cantidad2;
	char modelo[100], id[10], id2[10], nombre[100];
	for (int i = 0; i < cantidad1; i++ , ele = ele->NextSiblingElement()) {
		if (!ele || strcmp(ele->Value(),"dispositivo") != 0) {
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <dispositivo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <dispositivo>");
		}
		attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
		}

		strcpy(id, attr->Value());
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
		}

		strcpy(modelo, attr->Value());

		sub_ele = ele->FirstChildElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre, sub_ele->GetText());

		sub_ele = sub_ele->NextSiblingElement();
		while(true){
			if( sub_ele && strcmp(sub_ele->Value(), "lectura") != 0 &&
					strcmp(sub_ele->Value(), "grupos") != 0 ){
				sub_ele = sub_ele->NextSiblingElement();
			}
			else if( sub_ele == NULL ){
				escribir_log( "as-escritor", LOG_ERR, "Error Contando numero de Registros <lectura>" );
				throw runtime_error("Error Contando numero de Registros <lectura>");
			}
			else if( strcmp(sub_ele->Value(), "lectura") == 0 ){
				sub_sub_ele = sub_ele->FirstChildElement();
				if( !sub_sub_ele ){
					sub_ele = sub_ele->NextSiblingElement();
					continue;
				}
				marca_id_nombre[ind] = string(modelo) + "_" + string(id) + "_" + string(nombre);
				reg_x_disp[ind] = 0;
				while( sub_sub_ele && strcmp(sub_sub_ele->Value(), "registro") == 0 ){
					reg_x_disp[ind]++;
					sub_sub_ele = sub_sub_ele->NextSiblingElement();
				}
				ind++;
				break;
			}
			else if( strcmp(sub_ele->Value(), "grupos") == 0 ){
				sub_sub_ele = sub_ele->FirstChildElement();
				if( !sub_sub_ele ){
					sub_ele = sub_ele->NextSiblingElement();
					continue;
				}
				cantidad -= 1;	//Elimino un dispositivo ya que despues
								//va a ser agregado por cada equipo
				while( sub_sub_ele && strcmp( sub_sub_ele->Value(), "oyente" ) == 0 ){
					attr = sub_sub_ele->FirstAttribute();
					if( !attr || strcmp(attr->Name(), "id") != 0){
						sub_sub_ele = sub_sub_ele->NextSiblingElement();
						continue;
					}
					strcpy(id2, attr->Value());
					sub3_ele = sub_sub_ele->FirstChildElement();
					if (!sub3_ele || strcmp(sub3_ele->Value(), "cantidad") != 0) {
						sub_sub_ele = sub_sub_ele->NextSiblingElement();
						continue;
					}
					else{
						cantidad2 = atoi( sub3_ele->GetText() );
						cantidad += cantidad2;
						sub3_ele = sub3_ele->NextSiblingElement();
						for( int j = 0; j < cantidad2;
								j++, sub3_ele = sub3_ele->NextSiblingElement() ){
							if (!sub3_ele || strcmp(sub3_ele->Value(), "equipo") != 0) {
								sub_sub_ele = sub_sub_ele->NextSiblingElement();
								continue;
							}

							attr = sub3_ele->FirstAttribute();
							if( !attr || strcmp(attr->Name(), "nombre") != 0){
								sub_sub_ele = sub_sub_ele->NextSiblingElement();
								continue;
							}
							marca_id_nombre[ind] = string(modelo) + "_" +
								string(id) + "_" + string(nombre) + "_" +
								string(id2) + "_" + string( attr->Value() );
							sub4_ele = sub3_ele->FirstChildElement();
							while( sub4_ele && strcmp(sub4_ele->Value(), "registro") == 0 ){
								reg_x_disp[ind]++;
								sub4_ele = sub4_ele->NextSiblingElement();
							}
							ind++;
						}
						sub_sub_ele = sub_sub_ele->NextSiblingElement();
					}
				}
				break;
			}
		}

	}

	for( int i = 0; i < cantidad; i++ ){
#ifdef _DEBUG_
		cout <<"Nombre MQUEUE: "<< marca_id_nombre[i] << "\t\tRegistros: [" <<
			reg_x_disp[i] << "]" << endl;
#endif
		conf.colas_dispositivos.insert( pair <string, mqd_t>( marca_id_nombre[i], (mqd_t)-1 ) );
	}

    return true;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
*			de los tags que se desean escribir de un archivo XML
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config_tags(){
	TiXmlDocument doc(__ARCHIVO_CONF_TAGS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_TAGS) + "\n" + string(strerror(errno));
		escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"tag") != 0){
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tag>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
	}

	TiXmlElement 		*sub_ele,
						*sub_sub_ele;
	TiXmlAttribute 		*attr;
	char 				nombre_tag[_LONG_NOMB_TAG_],
						tmp1[100],
						tmp2[100],
						direccion[_LONG_NOMB_REG_];
	string				dispositivo;
	Tdato 				tipo_tag,
						tipo_registro;
	short				bit;


	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <tag> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <tag> -> id");
		}
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "escritura") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <tag> -> escritura" );
			throw runtime_error("Error Obteniendo Atributos de <tag> -> escritura");
		}
		if( atoi( attr->Value() ) != 1 ){
			continue;		//No esta habilitado para escritura
		}
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_x_defecto>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_x_defecto>");
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
		}
		TEXTO2DATO(tipo_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(sub_ele && ( strcmp(sub_ele->Value(),"dispositivo") != 0 ) ){
			sub_ele = sub_ele->NextSiblingElement();
		}
		if(sub_ele && ( strcmp(sub_ele->Value(),"dispositivo") == 0 ) ){
			attr = sub_ele->FirstAttribute();
			if( !attr || strcmp(attr->Name(), "id") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
				throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
			}
			strcpy(tmp1, attr->Value());
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "modelo") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
				throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
			}
			strcpy(tmp2, attr->Value());
			sub_sub_ele = sub_ele->FirstChildElement();
			if(!sub_sub_ele || strcmp(sub_sub_ele->Value(),"nombre") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion <dispositivo> -> nombre" );
				throw runtime_error("Error Parseando Archivo de Configuracion <dispositivo> -> nombre");
			}
			dispositivo = string( tmp2 ) + "_" + string( tmp1 ) + "_" +
							string( sub_sub_ele->GetText() );
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if(!sub_sub_ele || strcmp(sub_sub_ele->Value(),"registro") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion <dispositivo> -> registro" );
				throw runtime_error("Error Parseando Archivo de Configuracion <dispositivo> -> registro");
			}
			attr = sub_sub_ele->FirstAttribute();
			if( !attr || strcmp(attr->Name(), "direccion") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <registro> -> direccion" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> direccion");
			}
			strcpy( direccion, attr->Value() );
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "tipo") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tipo" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> tipo");
			}
			TEXTO2DATO( tipo_registro, attr->Value());
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "bit") != 0){
				escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <registro> -> bit" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> bit");
			}
			bit = ( strcmp(attr->Value(),"") != 0 ) ? atoi( attr->Value() ) : -1;
			conf.agregar_tag_escritura( nombre_tag, tipo_tag, dispositivo, direccion,
						tipo_registro, bit );
		}
	}
    return true;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
*			de las Alarmas que se desean escribir de un archivo XML
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config_alarmas(){
	TiXmlDocument doc(__ARCHIVO_CONF_ALARMAS);
	if( !doc.LoadFile() ){
		mensaje_error = "Error Cargando Archivo de Configuracion: " +
			string(__ARCHIVO_CONF_ALARMAS) + "\n" + string(strerror(errno));
        escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}
	TiXmlHandle hdle(&doc);
	TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
	}
	int cantidad = atoi( ele->GetText() );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	string nombre_alarmas = "escaner/" + string(ele->GetText());
	conf.shm_alarmas.inicializar_shm_thalarma( nombre_alarmas );
	conf.shm_alarmas.obtener_shm_tablahash();

	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"alarma") != 0){
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarma>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <alarma>");
	}

	TiXmlElement 		*sub_ele;
	TiXmlAttribute 		*attr;

	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <alarma> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <alarma> -> id");
		}
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "escritura") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Obteniendo Atributos de <alarma> -> escritura" );
			throw runtime_error("Error Obteniendo Atributos de <alarma> -> escritura");
		}
		if( atoi( attr->Value() ) != 1 ){
			continue;		//No esta habilitado para escritura
		}
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "grupo") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"subgrupo") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <subgrupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <subgrupo>");
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		conf.alarmas_escritura.push_back( string(sub_ele->GetText()) );

	}
	return true;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
*			del Servidor de un archivo XML
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config_servidor(){
	TiXmlDocument doc(__ARCHIVO_CONF_SERV_DATOS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " +
				string(__ARCHIVO_CONF_SERV_DATOS) + "\n" + string(strerror(errno));
		escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "origen") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <origen>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <origen>");
    }
	ele = ele->NextSiblingElement();
	if (!ele || strcmp(ele->Value(),"tiempo_entre_mensajes") != 0) {
		escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_entre_mensajes>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_entre_mensajes>");
    }
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(), "multicast") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <multicast>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <multicast>");
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(), "escritura") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <escritura>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <escritura>");
    }
	TiXmlElement * sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "puerto") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <puerto>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <puerto>");
    }
    conf.puerto = atoi( sub_ele->GetText() );
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"longitud_mensaje") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <longitud_mensaje>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <longitud_mensaje>");
    }
    conf.longitud_mensaje = atoi(sub_ele->GetText());
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"max_conexiones") != 0) {
    	escribir_log( "as-escritor", LOG_ERR, "Error Parseando Archivo de Configuracion -> <max_conexiones>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <max_conexiones>");
    }
    conf.max_conexiones = atoi(sub_ele->GetText());

    return true;
}

/**
* @brief	Envia los Datos para la Escritura a la Cola
*			de Mensajes del Dispositivo
* @param	rec		Datos de la Conexion y el Mensaje Recibido
*/
void enviar_a_cola(arg_hilo * rec){
		int 		sock = rec->sckt;
		Tmsgsswr	msj_recibido;

		strcpy(msj_recibido.nombre_tag, rec->mensaje.nombre_tag);
		msj_recibido.tipo_dato = rec->mensaje.tipo_dato;
		msj_recibido.valor = rec->mensaje.valor;

		wrConf::escritura		te;

		strcpy( te.nombre_tag, msj_recibido.nombre_tag );

		vector <wrConf::escritura>::iterator it;

		it = lower_bound(conf.tags_escritura.begin(), conf.tags_escritura.end(),
			te, comparacion_escritura);

		int         respuesta;

		if( (it == conf.tags_escritura.end()) || strcmp((*it).nombre_tag, te.nombre_tag) != 0 ){
			escribir_log( "as-escritor", LOG_WARNING, string("Tag No Encontrado o No Habilitado para Escritura [" +
							string(te.nombre_tag) + "]").c_str() );
            respuesta = _ESCR_ERROR_;
			if( send( sock, &respuesta, sizeof(_ESCR_ERROR_), 0 ) == -1 ){
				mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
				escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
			}
		}
		else{
			(*it).destino.valor = msj_recibido.valor;
#ifdef _DEBUG_
			cout << "Escribiendo Mensaje: " << (*it).destino.direccion << "\nTAG: " <<
				(*it).nombre_tag << "\tCOLA: " << (*it).cola << "\tVALOR: 0x" << std::hex << std::uppercase <<
				(*it).destino.valor.L << "\n";
#endif

			if( mq_send( (*it).cola, (const char *)(&((*it).destino)), sizeof(registro), 0) == -1 ){
				if( errno == EAGAIN ){
					escribir_log( "as-escritor", LOG_WARNING, "Cola de Mensajes Llena..." );
					respuesta = _ESCR_ERROR_;
					if( send( sock, &respuesta, sizeof(_ESCR_ERROR_), 0 ) == -1 ){
						mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
						escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
						throw runtime_error(mensaje_error.c_str());
					}

				}
				else{
					mensaje_error = "Error Enviando Mensaje a Cola: " + string( strerror( errno ) );
					escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
					throw runtime_error( mensaje_error.c_str() );
				}
			}

			respuesta = _ESCR_OK_;
			if( send( sock, &respuesta, sizeof(_ESCR_OK_), 0 ) == -1 ){
				mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
				escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
			}
		}
		close( sock );
}

/**
* @brief	Comparacion de los Tags para Escritura
* @param	e1			Estructura que contiene el Tag a Escribir
* @param	e2			Estructura que contiene el Tag a Escribir
* @return	verdadero	Si el Nombre Tag1 es menor al del Tag2
*/
bool comparacion_escritura( wrConf::escritura e1, wrConf::escritura e2){
	if( strcmp( e1.nombre_tag, e2.nombre_tag ) < 0 )
		return true;
	return false;
}

/**
* @brief	Escribe los Datos de la Alarma en la
*			Memoria Compartida de Alarmas del Servidor
* @param	rec		Datos de la Conexion y el Mensaje Recibido
*/
void actualizar_alarma(arg_hilo * rec){
	int 		sock = rec->sckt;
	alarma		a;

	a.asignar_propiedades( rec->mensaje.nombre_alarma, rec->mensaje.estado,
			rec->mensaje.tipo_dato );


	vector <string>::iterator it;

	it = lower_bound( conf.alarmas_escritura.begin(), conf.alarmas_escritura.end(),
		string(a.nombre) );

	int         respuesta;

	if( (it == conf.alarmas_escritura.end()) || (*it) != string(a.nombre) ){
		escribir_log( "as-escritor", LOG_WARNING, string("Alarma No Encontrada o No Habilitada para Escritura [" +
						string(a.nombre) + "]").c_str() );
		respuesta = _ESCR_ERROR_;
		if( send( sock, &respuesta, sizeof(_ESCR_ERROR_), 0 ) == -1 ){
			mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
			escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
	}
	else{
		conf.shm_alarmas.actualizar_alarma(&a);
		respuesta = _ESCR_OK_;
		if( send( sock, &respuesta, sizeof(_ESCR_OK_), 0 ) == -1 ){
			mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
			escribir_log( "as-escritor", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
	}
	close( sock );
}
