/**
* @file	as-cliente.cpp
* @brief		<b>Proceso encargado de Consumir los Tags y las Alarmas que
*				son enviados por el servidor, este cliente puede correr
*				en la misma maquina que el servidor o puede estar remotamente
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
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>

#include "tinyxml.h"
#include "as-util.h"
#include "as-tags.h"
#include "as-shmtag.h"
#include "as-alarma.h"
#include "as-shmalarma.h"
#include "as-multicast.h"

using namespace std;
using namespace argos;

/*--------------------------------------------------------
	Estructura de dato usada para encapsular un paquete
	de alarmas y una referencia a la memoria compartida
	de alarmas
----------------------------------------------------------*/
typedef struct{
	paquete_alarmas		*pqte;			//!< Estructura que contiene un Arreglo de Alarmas
	unsigned int		alarmas_x_msj;	//!< Cantidad de Alarmas que vienen en el Arreglo
	shm_thalarma		shm;			//!< Memoria Compartida de Alarmas en el Cliente
}pqte_hilo_alarmas;

/*--------------------------------------------------------
	Estructura de dato usada para encapsular un paquete
	de tags y una referencia a la memoria compartida
	de tags
----------------------------------------------------------*/
typedef struct{
	paquete_tags		*pqte;			//!< estructura que contiene un Arreglo de Tags
	unsigned int		tags_x_msj;		//!< Cantidad de Tags que vienen en el Arreglo
	shm_thtag			shm;			//!< Memoria Compartida de Tags en el Cliente
}pqte_hilo_tags;


/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			recibe los Tags y Alarmas enviados por el servidor</b><br>
*/
class configuracion{
public:
	char				directorio[100];		//!< Directorio donde reside la SHMEM del Cliente
	char				tags_shmem[100];		//!< Memoria Compartida de Tags en el Cliente
	bool				tags_habilitado;		//!< Habilitacion para Lectura de Tags en el Cliente
	char				alarmas_shmem[100];		//!< Memoria Compartida de Alarmas en el Cliente
	bool				alarmas_habilitado;		//!< Habilitacion para Lectura de Alarmas en el Cliente
	mcast				conexion;				//!< Objeto para la Recepcion de Mensajes MULTICAST
	pqte_hilo_tags  	pht;					//!< Referencia al Paquete de Tags
	pqte_hilo_alarmas  	pha;					//!< Referencia al Paquete de Alarmas

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
	configuracion() : conexion(){
		directorio[0]			= '\0';
		tags_shmem[0]			= '\0';
		tags_habilitado			= false;
		alarmas_shmem[0]		= '\0';
		alarmas_habilitado		= false;
		pht.pqte				= NULL;
		pht.tags_x_msj			= 0;
		//pht.shm					= NULL;
		pha.pqte				= NULL;
		pha.alarmas_x_msj		= 0;
		//pha.shm					= NULL;
	}
};

static int	 		verbose_flag;		//!< Modo Verboso
string				mensaje_error;		//!< Mensaje de Error por Pantalla
pthread_t 			hilo_tags;			//!< Recepcion Concurrente de Tags
pthread_t 			hilo_alarmas;		//!< Recepcion Concurrente de Alarmas

/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool 		asignar_opciones(int, char**);
void * 		procesar_tags(pqte_hilo_tags *);
void * 		procesar_alarmas(pqte_hilo_alarmas *);
bool 		leer_config_cliente(configuracion *);
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal para la Recepcion
*			de los Tags y Alarmas Enviados por el Productor
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc,char **argv) {

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-cliente) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-cliente", LOG_INFO, "Iniciando Servicio..." );

	configuracion conf;

	if( !asignar_opciones(argc,argv) ){
		mensaje_error = "Opcion Invalida";
		escribir_log( "as-cliente", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}

    leer_config_cliente( &conf );
	conf.conexion.hacer_servidor(false);
	conf.conexion.iniciar_mcast();

	conf.pht.tags_x_msj = conf.pht.tags_x_msj > _MAX_TAGS_X_MSG_ ?
				_MAX_TAGS_X_MSG_ : conf.pht.tags_x_msj;

	conf.pha.alarmas_x_msj = conf.pha.alarmas_x_msj > _MAX_ALARM_X_MSG_ ?
				_MAX_ALARM_X_MSG_ : conf.pha.alarmas_x_msj;


    int                 cnt = 0;
	char                msje[5000];
	char                msje1[5000];
	char                msje2[5000];
	__uint64_t          *tipo;

	while(true){
		bzero(&msje[0],sizeof(msje));
		cnt = conf.conexion.recibir_msg(msje, 5000);
		tipo = reinterpret_cast<__uint64_t *>(&msje[0]);
		switch(*tipo){
			case _TIPO_TAG_: // 'T' de tags
#ifdef _DEBUG_
				cout << "Paquete de Tags" << endl;
#endif
				bzero(msje1,sizeof(msje1));
				memcpy(msje1, msje, sizeof(msje1));
				//conf.pht.pqte = (paquete_tags *) msje1;
				conf.pht.pqte = reinterpret_cast<paquete_tags *> (&msje1[0]);
				pthread_create(&hilo_tags, NULL, (void *(*)(void *))procesar_tags, &(conf.pht) );
				pthread_detach(hilo_tags);
			break;
			case _TIPO_ALARMA_: // 'A' de alarmas
#ifdef _DEBUG_
				cout << "Paquete de Alarmas" << endl;
#endif
				bzero(msje2, sizeof(msje2));
				memcpy(msje2, msje, sizeof(msje2));
				conf.pha.pqte = (paquete_alarmas *) msje2;
				pthread_create(&hilo_alarmas, NULL, (void *(*)(void *))procesar_alarmas, &(conf.pha) );
				pthread_detach(hilo_alarmas);
			break;
			default:
#ifdef _DEBUG_
				cout << "Paquete Desconocido" << endl;
#endif
				escribir_log( "as-cliente", LOG_WARNING, "Paquete Recibido de Tipo Desconocido" );
				continue;
		}
	}

	return 1;
}

/**
* @brief	Lee las opciones pasadas al programa como parametros
*			en la linea de comandos
* @param	argc		Cantidad de Argumentos
* @param	argv		Lista de Argumentos
* @return	verdadero	Lectura Satisfactoria
*/
bool asignar_opciones(int argc, char **argv){
     int c;
     while (1){
    	 static struct option long_options[] =
             {
               {"verbose", no_argument, &verbose_flag, 1},
               {"brief",   no_argument, &verbose_flag, 0},
               {"rec-all",	no_argument,		0, 'r'},
               {"conf",		required_argument,	0, 'c'},
               {0, 0, 0, 0}
             };
           int option_index = 0;
           c = getopt_long (argc, argv, "rc:",long_options, &option_index);
           if (c == -1)
             break;
           switch (c){
             case 0:
               if (long_options[option_index].flag != 0)
                 break;
               cout << "option " << long_options[option_index].name;
               if (optarg)
                 cout << " with arg " << optarg;
               cout << endl;
               break;
             case 'r':
#ifdef _DEBUG_
             cout <<"r"<<endl;
#endif
               break;
             case 'c':
             strcpy(__ARCHIVO_CONF_CLIENTE_DATOS, optarg);
#ifdef _DEBUG_
             cout << "Archivo de Configuracion del Cliente: " <<
				__ARCHIVO_CONF_CLIENTE_DATOS << endl;
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
            cout << "\tProyecto Argos (as-cliente) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
			cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
       }
       if (optind < argc){
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
* @param	c			Atibutos configurables del proceso
* @return	verdadero	Configuracion exitosa
*/
bool leer_config_cliente(configuracion * c){
	TiXmlDocument doc(__ARCHIVO_CONF_CLIENTE_DATOS);
	if( !doc.LoadFile() ){
		mensaje_error = "Error Cargando Archivo de Configuracion: " +
				string(__ARCHIVO_CONF_CLIENTE_DATOS) + "\n" + string(strerror(errno));
		escribir_log( "as-cliente", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}
	TiXmlHandle hdle(&doc);
	TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "directorio") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <directorio>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <directorio>");
	}
	strcpy( c->directorio, ele->GetText() );
	char tmp[500];
	sprintf( tmp, "/dev/shm/%s", c->directorio );
	struct stat datos_directorio;
	lstat(tmp, &datos_directorio);
	if( !S_ISDIR( datos_directorio.st_mode ) ){
		if(-1 == mkdir(tmp,0777) ){
			mensaje_error = "Error Creando Directorio para SHMEM: " + string(strerror(errno));
			escribir_log( "as-cliente", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
	}
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "destino_tags") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <destino_tags>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <destino_tags>");
	}
	strcpy( c->tags_shmem, ele->GetText() );
	TiXmlAttribute * attr = ele->FirstAttribute();
    if( !attr || strcmp(attr->Name(), "habilitado") != 0){
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion <destino_tags> -> habilitado" );
		throw runtime_error("Error Parseando Archivo de Configuracion <destino_tags> -> habilitado");
	}
	c->tags_habilitado = ( atoi(attr->Value()) == 1 );

	//---------Inicializar la Memoria Compartida de Tags para el Cliente
	sprintf( tmp, "%s/%s", c->directorio, c->tags_shmem );
	c->pht.shm.inicializar_shm_thtag( tmp );


	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "tags_x_mensaje") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags_x_mensaje>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tags_x_mensaje>");
	}
	c->pht.tags_x_msj = (unsigned int) atoi(ele->GetText());
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "destino_alarmas") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <destino_alarmas>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <destino_alarmas>");
	}
	strcpy( c->alarmas_shmem, ele->GetText() );
	attr = ele->FirstAttribute();
    if( !attr || strcmp(attr->Name(), "habilitado") != 0){
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion <destino_alarmas> -> habilitado" );
		throw runtime_error("Error Parseando Archivo de Configuracion <destino_alarmas> -> habilitado");
	}
	c->alarmas_habilitado = ( atoi(attr->Value()) == 1 );

	//---------Inicializar la Memoria Compartida de Alarmas para el Cliente
	sprintf( tmp, "%s/%s", c->directorio, c->alarmas_shmem );
	c->pha.shm.inicializar_shm_thalarma( tmp );

	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "alarmas_x_mensaje") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas_x_mensaje>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <alarmas_x_mensaje>");
	}
	c->pha.alarmas_x_msj = (unsigned int) atoi(ele->GetText());
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(), "multicast") != 0) {
        escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <multicast>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <multicast>");
    }
    TiXmlElement *sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "puerto") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <puerto>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <puerto>");
    }
    c->conexion.asignar_puerto( atoi(sub_ele->GetText()) );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"grupo") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupo>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
    }
	c->conexion.asignar_grupo( sub_ele->GetText() );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"loop") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <loop>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <loop>");
    }
	c->conexion.asignar_loop( static_cast<short> (atoi(sub_ele->GetText())) );
	sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(),"ttl") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <ttl>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <ttl>");
    }
	c->conexion.asignar_ttl( static_cast<short> (atoi(sub_ele->GetText())) );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "tags_cliente") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags_cliente>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tags_cliente>");
	}
	sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
	int cantidad = atoi( sub_ele->GetText() );

	//----------Crear la Memoria Compartida para Tags
	c->pht.shm.crear_shm_tablahash( cantidad );

	sub_ele = sub_ele->NextSiblingElement();
	if(!sub_ele || strcmp(sub_ele->Value(),"tag") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tag>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
	}
	TiXmlElement 		*sub2_ele;
	char 				nombre_tag[100], valor_char[100];
	Tdato 				tipo_tag;
	Tvalor 				valor_tag;
	tags 				t;
	for(int i = 0; i < cantidad; i++ , sub_ele = sub_ele->NextSiblingElement()){
		sub2_ele = sub_ele->FirstChildElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(), "nombre") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre_tag,sub2_ele->GetText());
		sub2_ele = sub2_ele->NextSiblingElement();
		//GRUPO

		sub2_ele = sub2_ele->NextSiblingElement();
		//DESCRIPCION

		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"valor_x_defecto") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_x_defecto>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_x_defecto>");
		}
		strcpy(valor_char,sub2_ele->GetText());
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"tipo") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
		}
		TEXTO2DATO(tipo_tag,sub2_ele->GetText());
		switch(tipo_tag){
			case ENTERO_:
				valor_tag.I = atoi(valor_char);
        	break;
        	case REAL_:
				valor_tag.F = (float) atof(valor_char);
        	break;
        	case DREAL_:
				valor_tag.D = atof(valor_char);
        	break;
        	case BIT_:
				valor_tag.B = (atoi(valor_char) == 1) ? true : false;
        	break;
        	case TERROR_:
        	default:
				valor_tag.L = LONG_MIN;
		}
		t.asignar_propiedades(nombre_tag,valor_tag,tipo_tag);
		c->pht.shm.insertar_tag(t);
	}


	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(), "alarmas_cliente") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas_cliente>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <alarmas_cliente>");
	}
	sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
	cantidad = atoi( sub_ele->GetText() );

	//----------Crear la Memoria Compartida para Alarmas
	c->pha.shm.crear_shm_tablahash( cantidad );

	sub_ele = sub_ele->NextSiblingElement();
	if(!sub_ele || strcmp(sub_ele->Value(),"alarma") != 0){
		escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarma>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <alarma>");
	}
	char	grupo[_LONG_NOMB_GALARM_],
			subgrupo[_LONG_NOMB_SGALARM_],
			nombre[_LONG_NOMB_ALARM_],
			valor_tmp[100];
	int		idg, idsg;
	Ealarma estado;
	Tdato tipo_origen;
	Tvalor valor_comparacion;
	Tcomp tipo_comparacion;
	alarma a;
	for(int i = 0; i < cantidad; i++ , sub_ele = sub_ele->NextSiblingElement()){
		sub2_ele = sub_ele->FirstChildElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(), "grupo") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
		}
		attr = sub2_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Obteniendo Atributos de <grupo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <grupo> -> id");
		}
		idg = atoi( attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "descripcion") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Obteniendo Atributos de <grupo> -> descripcion" );
			throw runtime_error("Error Obteniendo Atributos de <grupo> -> descripcion");
		}
		strcpy( grupo, attr->Value() );
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"subgrupo") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Obteniendo Atributos de -> <subgrupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <subgrupo>");
		}
		attr = sub2_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Obteniendo Atributos de <subgrupo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <subgrupo> -> id");
		}
		idsg = atoi( attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "descripcion") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Obteniendo Atributos de <subgrupo> -> descripcion" );
			throw runtime_error("Error Obteniendo Atributos de <subgrupo> -> descripcion");
		}
		strcpy( subgrupo, attr->Value() );
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(), "nombre") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre,sub2_ele->GetText());
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"estado_x_defecto") != 0){
			mensaje_error = "Error Parseando Archivo de Configuracion -> <estado_x_defecto> ";
			escribir_log( "as-cliente", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		TEXTO2ESTADO(estado,sub2_ele->GetText());
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"valor_comparacion") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_comparacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_comparacion>");
		}
		if(sub2_ele->GetText())
			strcpy( valor_tmp, sub2_ele->GetText() );
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"tipo_comparacion") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo_comparacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo_comparacion>");
		}
		if(sub2_ele->GetText())
			TEXTO2COMP(tipo_comparacion,sub2_ele->GetText());
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"tipo_origen") != 0){
			escribir_log( "as-cliente", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo_origen>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo_origen>");
		}
		TEXTO2DATO(tipo_origen, sub2_ele->GetText());
		switch(tipo_origen){
			case ENTERO_:
				valor_comparacion.I = atoi(valor_tmp);
        	break;
        	case REAL_:
				valor_comparacion.F = (float) atof(valor_tmp);
        	break;
        	case DREAL_:
				valor_comparacion.D = atof(valor_tmp);
        	break;
        	case BIT_:
        	case TERROR_:
        	default:
				valor_comparacion.L = LONG_MIN;
		}
		sub2_ele = sub2_ele->NextSiblingElement();
		//DESCRIPCION

		a.asignar_propiedades( idg, grupo, idsg, subgrupo, nombre, estado,
				valor_comparacion, tipo_comparacion, tipo_origen );
		c->pha.shm.insertar_alarma(a);
	}
	return true;
}

/**
* @brief 	Se ejecuta por un Hilo para permitir la concurrencia
*			en el procesamiento de los mensajes recibidos y asi
*			actualizar los Tags del Cliente
* @param	pqte	Atibutos del Tag
* @return	vacio	Nada
*/
void * procesar_tags(pqte_hilo_tags * pqte) {
    tags    temp;
	for (unsigned int i = 0; i < pqte->tags_x_msj; i++) {
	    temp.asignar_propiedades( pqte->pqte->arreglo[i].nombre,
                pqte->pqte->arreglo[i].valor_campo, pqte->pqte->arreglo[i].tipo );

		pqte->shm.actualizar_tag(&temp);

#ifdef _DEBUG_
		cout << "TAG " << i << " : " << temp << endl;
#endif

	}
	pthread_exit(0);
}

/**
* @brief 	Se ejecuta por un Hilo para permitir la concurrencia
*			en el procesamiento de los mensajes recibidos y asi
*			actualizar las Alarmas del Cliente
* @param	pqte	Atibutos de la Alarma
* @return	vacio	Nada
*/
void * procesar_alarmas(pqte_hilo_alarmas * pqte) {
    alarma  temp;
	for (unsigned int i = 0; i < pqte->alarmas_x_msj; i++) {
	    temp.asignar_propiedades( pqte->pqte->arreglo[i].nombre,
                pqte->pqte->arreglo[i].estado, pqte->pqte->arreglo[i].tipo_origen );

        pqte->shm.actualizar_alarma(&temp);

#ifdef _DEBUG_
		cout << "ALARMA " << i << " : " << temp << endl;
#endif

	}
	pthread_exit(0);
}

