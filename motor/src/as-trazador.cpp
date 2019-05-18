/**
* @file	as-trazador.cpp
* @brief		<b>Proceso encargado del Itercambio de Tendencias
				Entre Servidores y Clientes (Usando Serializacion en XML),
				obtenidas del buffer circular o de la base de datos.</b><br>
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

#include <vector>
#include <stdexcept>
#include <algorithm>

//#include "sstraza.h"
#include "as-shmhistorico.h"
#include "tinyxml.h"
#include "as-util.h"
#include "libpq-fe.h"

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>


using namespace std;
using namespace argos;


static int	 		verbose_flag;		//!< Modo Verboso
string					mensaje_error;		//!< Mensaje de Error por Pantalla



/*--------------------------------------------------------
	Estructura de dato usada para encapsular los datos para
	el hilo que procesa la tendencia de memoria compartida
	y base de datos
----------------------------------------------------------*/
typedef struct _Ttraza_pthread{
	int						sockt;		//!< Socket para responder la peticion
	string				xml_in;		//!< XML requerimiento de tendencia
}Ttraza_pthread;



namespace trazConf{

/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			registra los valores de los Tags y las Alarmasen los Buffer Circulares</b><br>
*/
class configuracion{
public:
	char					hist_tags_shmem[100];		//!< Nombre del Buffer Circular de Tags
	char					hist_alarmas_shmem[100];	//!< Nombre del Buffer Circular de Alarmas
	char					bdName[100];				//!< Nombre de la Base de Datos
	char					bdEsquema[100];				//!< Nombre del Esquema de Base de Datos
	char					bdLogin[50];				//!< Nombre del Usuario de Base de Datos
	char					bdPwd[20];					//!< Contrasena de Base de Datos
	vector
	<argos::historico>
								v_hist;		//!< Vector de Tags a Registrar
	shm_historico	hist_shmem;
	unsigned int	alarmas_buf;				//!< Cantidad de Alarmas en el Buffer
	int						puerto;						//!< Puerto para escuchar las conexiones
	int						longitud_mensaje;			//!< Tamano del Mensaje escuchado
	int						max_conexiones;				//!< Cantidad de conexiones permitidas

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
	configuracion(){
		strcpy(hist_tags_shmem,"tendencias");
		strcpy(hist_alarmas_shmem,"alarmeros");
		alarmas_buf			= 500;
		puerto 				= 16009;
		longitud_mensaje 	= 20000;
		max_conexiones 		= 20;
	}

};

}


trazConf::configuracion conf;			//!< Objeto de Configuracion de ssTRAZADOR
Ttraza_pthread			pthread_arg;

/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool asignar_opciones(int, char **);
bool leer_config();
string obtener_tendencia_shm( char *, char *, char * );
string obtener_tendencia_bd( char *, char *, char * );
void obtener_desde_bd( Ttraza_pthread * );
void obtener_desde_shm( Ttraza_pthread * );
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char **argv) {

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-trazador) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-trazador", LOG_INFO, "Iniciando Servicio..." );

	// Obtener los Archivos de Configuracion
    if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

	// Cargar las Configuraciones en vectores utilitarios
    leer_config();

#ifdef _DEBUG_
    cout << conf.v_hist.size() << " Tags fueron registrados" << endl;
    //cout << conf.pha.v_alarm.size() << " Alarmas van a ser registradas" << endl;
#endif

	// Ordenar por Nombre de Tags y Alarmas
    sort(conf.v_hist.begin(), conf.v_hist.end());
    //sort(conf.pha.v_alarm.begin(), conf.pha.v_alarm.end());

#ifdef _DEBUG_
	cout << "\n\tListado de Tags....\n";
    for ( unsigned int i = 0; i < conf.v_hist.size(); i++){
        cout << conf.v_hist[i];
    }
//	cout << "\n\n\tListado de Alarmas....\n";
//	for ( unsigned int i = 0; i < conf.pha.v_alarm.size(); i++){
//       cout << conf.pha.v_alarm[i] << endl;
//	}

#endif


	// Abrir la memoria compartida de donde se obtendran los puntos
	string 				nombre_shm = "registrador/" + string(conf.hist_tags_shmem);
	conf.hist_shmem.inicializar_shm_tendencia( nombre_shm );
	conf.hist_shmem.obtener_shm_tendencia();

	// Abrir la memoria compartida de donde se obtendran las alarmas
	nombre_shm = "registrador/" + string(conf.hist_alarmas_shmem);
	conf.hist_shmem.inicializar_shm_alarmero( nombre_shm );
	conf.hist_shmem.obtener_shm_alarmero();


	//Crear Servidor TCP/IP

	//Esperar por requisito de escritura o reconocimiento de alarma

	//Cuando llega la solicitud levantar un hilo


	struct
	sockaddr_in		sin,
								pin;
	int						sckt,
								sckt2;
	socklen_t			addr_size;


	sckt = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( -1 == sckt ){
		mensaje_error = "Error Abriendo Socket: " + string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	bzero( &sin, sizeof( sin ) );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl( INADDR_ANY );
	sin.sin_port = htons( conf.puerto );

	if( bind( sckt, (struct sockaddr *)&sin, sizeof( sin )) == -1 ){
		mensaje_error = "Error Enlazando Socket: " + string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	if( listen( sckt, conf.max_conexiones ) == -1 ){
		mensaje_error = "Error Cantidad de Conexiones por Socket: " + string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

//	addr_size = sizeof( pin );
//	sckt2 = accept( sckt, (struct sockaddr *)&pin, &addr_size );
//	if( -1 == sckt2 ){
//		mensaje_error = "Error Aceptando Conexiones: " + string( strerror( errno ) );
//		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
//		throw runtime_error(mensaje_error.c_str());
//	}

	char	mensaje[_MAX_TRAZA_XML_REQ_];
	Cmens	tipo_tendencia;

	while( true ){
		addr_size = sizeof( pin );
		sckt2 = accept( sckt, (struct sockaddr *)&pin, &addr_size );
		if( -1 == sckt2 ){
			mensaje_error = "Error Aceptando Conexiones: " + string( strerror( errno ) );
			throw runtime_error(mensaje_error.c_str());
		}


		memset( mensaje, 0, sizeof(Cmens) );

		if( recv( sckt2, (char *)(&mensaje), sizeof(mensaje), 0 ) == -1 ){
			mensaje_error = "Error Recibiendo Datos: " + string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}

		memcpy( &tipo_tendencia, mensaje, sizeof( Cmens ) );

		if( tipo_tendencia != _TRAZA_BD_ && tipo_tendencia != _TRAZA_SHM_ ){
			escribir_log( "as-trazador", LOG_WARNING, "Recibido Mensaje Desconocido" );
#ifdef _DEBUG_
			strcat(&mensaje[100], "......\0");
    	cout << mensaje << endl;
#endif
			continue;
		}

		TiXmlDocument res;
		res.Parse( &mensaje[ sizeof( Cmens ) ] );


/*
		const char * xml_in1 =
			"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
			"<tendencias_xml>"
			"<cantidad_variables>12</cantidad_variables>"
			"<tipo>rt</tipo>"
			"<variable id=\"1\" nombre=\"trasl_puente_I_motor_lado_uno\" >"
			"<desde>12000000</desde>"
			"<hasta>1250000000</hasta>"
			"</variable>"
			"<variable id=\"2\" nombre=\"temperatura2\" >"
			"<desde>13000000</desde>"
			"<hasta>1250000000</hasta>"
			"</variable>"
			"<variable id=\"3\" nombre=\"trasl_puente_Vel_motor_lado_uno\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"4\" nombre=\"compres_Presion_Salida_Alta\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"5\" nombre=\"compres_Presion_Salida_Baja\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"6\" nombre=\"ganch_col_I_Motor_Dir\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"7\" nombre=\"rot_ms_Tiempo_Uso_Motor\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"8\" nombre=\"trasl_ms_Arranques_Motor\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"9\" nombre=\"trasl_puente_Arr_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"10\" nombre=\"trasl_puente_I_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"11\" nombre=\"trasl_puente_I_motor_lado_uno\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"12\" nombre=\"trasl_puente_Vel_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"</tendencias_xml>";

		const char * xml_in2 =
			"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
			"<tendencias_xml>"
			"<cantidad_variables>12</cantidad_variables>"
			"<tipo>bd</tipo>"
			"<variable id=\"1\" nombre=\"trasl_puente_I_motor_lado_uno\" >"
			"<desde>12000000</desde>"
			"<hasta>1250000000</hasta>"
			"</variable>"
			"<variable id=\"2\" nombre=\"temperatura2\" >"
			"<desde>13000000</desde>"
			"<hasta>1250000000</hasta>"
			"</variable>"
			"<variable id=\"3\" nombre=\"trasl_puente_Vel_motor_lado_uno\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"4\" nombre=\"compres_Presion_Salida_Alta\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"5\" nombre=\"compres_Presion_Salida_Baja\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"6\" nombre=\"ganch_col_I_Motor_Dir\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"7\" nombre=\"rot_ms_Tiempo_Uso_Motor\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"8\" nombre=\"trasl_ms_Arranques_Motor\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"9\" nombre=\"trasl_puente_Arr_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"10\" nombre=\"trasl_puente_I_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"11\" nombre=\"trasl_puente_I_motor_lado_uno\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"<variable id=\"12\" nombre=\"trasl_puente_Vel_motor_lado_dos\" >"
			"<desde>14000000</desde>"
			"<hasta>12500000000</hasta>"
			"</variable>"
			"</tendencias_xml>";

		res.Parse( xml_in1 );
*/

		if( res.Error() ){
			mensaje_error = "Error Parseando: " + string( res.Value() ) + string( res.ErrorDesc() );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}

#ifdef _DEBUG_
		TiXmlPrinter printer;
		res.Accept( &printer );
		fprintf( stdout, "%s", printer.CStr() );
#endif


		TiXmlNode 		*n1 = res.FirstChild( "tendencias_xml" );
		TiXmlElement 	*ele1 = n1->ToElement()->FirstChildElement();

		if (!ele1 || strcmp(ele1->Value(), "cantidad_variables") != 0) {
			mensaje_error = "Error -> <cantidad_variables>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}


		ele1 = ele1->NextSiblingElement();

		pthread_t 	tendencia_rt,
								tendencia_bd;

		if (!ele1 || strcmp(ele1->Value(), "tipo") != 0) {
			mensaje_error = "Error -> <tipo>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		if( strcmp( ele1->GetText(), "rt" ) == 0 ){
			//----------------Arrancar el Hilo para Tendencias RT
#ifdef _DEBUG_
			cout << "Arrancando Hilo para Tendencias RT....." << endl;
#endif

			pthread_arg.sockt = sckt2;
			pthread_arg.xml_in.assign( &mensaje[ sizeof( Cmens ) ] );
			//pthread_arg.xml_in.assign( xml_in1 );
			if( pthread_create( &tendencia_rt, NULL, (void *(*)(void *))obtener_desde_shm,
					&pthread_arg ) ){
				mensaje_error = "Error Creando Hilo para Tendencias RT: "
						+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error( mensaje_error.c_str() );
			}
			pthread_detach( tendencia_rt );
		}
		else if( strcmp( ele1->GetText(), "bd" ) == 0 ){

			//----------------Arrancar el Hilo para Tendencias Historicas
#ifdef _DEBUG_
			cout << "Arrancando Hilo para Tendencias Historicas....." << endl;
#endif
			pthread_arg.sockt = sckt2;
			pthread_arg.xml_in.assign( &mensaje[ sizeof( Cmens ) ] );
			if( pthread_create( &tendencia_bd, NULL, (void *(*)(void *))obtener_desde_bd,
					&pthread_arg ) ){
				mensaje_error = "Error Creando Hilo para Tendencias Historicas: "
						+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error( mensaje_error.c_str() );
			}
			pthread_detach( tendencia_bd );
		}

		res.Clear();

	}









//}



    return 1;
}

/**
* @brief 	Obtiene los parametros y argumentos de configuracion
*			a partir de un archivo XML
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config() {

	// Primero Leer el archivo de configuracion de historicos
	TiXmlDocument doc(__ARCHIVO_CONF_HISTORICOS);
    if ( !doc.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_HISTORICOS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    TiXmlElement * sub_ele, *ssub_ele;
    if (!ele || strcmp(ele->Value(), "cola") != 0) {
    	mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <cola>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tags") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tags>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy( conf.hist_tags_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"alarmas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <alarmas>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy( conf.hist_alarmas_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"bd") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <bd>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
	sub_ele = ele->FirstChildElement();
	if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
		escribir_log( "as-trazador", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	strcpy(conf.bdName,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"esquema") != 0) {
		escribir_log( "as-trazador", LOG_ERR, "Error Parseando Archivo de Configuracion -> <esquema>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <esquema>");
	}
	strcpy(conf.bdEsquema,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"usuario") != 0) {
		escribir_log( "as-trazador", LOG_ERR, "Error Parseando Archivo de Configuracion -> <usuario>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <usuario>");
	}
	strcpy(conf.bdLogin,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"pwd") != 0) {
		escribir_log( "as-trazador", LOG_ERR, "Error Parseando Archivo de Configuracion -> <pwd>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <pwd>");
	}
	strcpy(conf.bdPwd,sub_ele->GetText());
	ele = ele->NextSiblingElement();
	if (!ele || strcmp(ele->Value(),"mensaje") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <mensaje>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_escaner") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_escaner>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_entre_consultas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_entre_consultas>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_descarga_lote") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_descarga_lote>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"buffer_alarmas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <buffer_alarmas>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    conf.alarmas_buf = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );


	// Ahora Leer el archivo de configuracion de tags
    TiXmlDocument doc1(__ARCHIVO_CONF_TAGS);
    if ( !doc1.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_TAGS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle1(&doc1);
    ele = hdle1.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <cantidad>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <nombre>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tag") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <tag>\n "
			+ string( strerror( errno ) );
        escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    char nombre_tag[_LONG_NOMB_TAG_], valor_char[100], *valor_funcion,
		tmp_funcion[100];
    Tvalor valor_tag;
    Tdato tipo_tag;
    historico tmp;
    Tlog tl;
    Tfunciones tf;
    unsigned int n_func_disponibles = numero_funciones_disponibles();
    for (int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
        sub_ele = ele->FirstChildElement();
        if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <tag> -> <nombre>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
        }
        strcpy(nombre_tag,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <tag> -> <valor_x_defecto>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
        }
        strcpy(valor_char,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <tag> -> <tipo>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
        }
        //tipo_tag = *(sub_ele->GetText());
        TEXTO2DATO(tipo_tag,sub_ele->GetText());
        switch(tipo_tag){
			case CARAC_:
				valor_tag.C = static_cast<char> (atoi(valor_char));
        	break;
			case CORTO_:
				valor_tag.S = static_cast<short> (atoi(valor_char));
        	break;
			case ENTERO_:
				valor_tag.I = atoi(valor_char);
        	break;
			case LARGO_:
				valor_tag.L = atoll(valor_char);
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
				valor_tag.L = _ERR_DATO_;
		}

        tmp.t.asignar_propiedades(nombre_tag,valor_tag,tipo_tag);

        sub_ele = sub_ele->NextSiblingElement();
        if (sub_ele && strcmp(sub_ele->Value(), "historicos") == 0) {
            ssub_ele = sub_ele->FirstChildElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "tipo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <tipo>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            TEXTO2LOG(tl,ssub_ele->GetText());
            tmp.asignar_tipo(tl);
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "tiempo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <tiempo>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_tiempo( atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "maximo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <maximo>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_maximo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "minimo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <minimo>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_minimo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "muestras") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <muestras>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            unsigned int mts = atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() );
            tmp.asignar_muestras( mts > _MAX_CANT_MUESTRAS_ ? _MAX_CANT_MUESTRAS_ : mts );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "funciones") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <funciones>\n "
					+ string( strerror( errno ) );
				escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
				throw runtime_error(mensaje_error.c_str());
            }
            strcpy(tmp_funcion, ssub_ele->GetText());
            valor_funcion = strtok (tmp_funcion,",");
			while (valor_funcion != NULL)
			{
				for (unsigned int j = 0; j < n_func_disponibles; j++) {
					if ( strcmp(valor_funcion,_FUNCIONES_HISTORICOS_[j]) == 0 ) {
						TEXTO2FUNC(tf,_FUNCIONES_HISTORICOS_[j]);
						tmp.asignar_funcion(tf);
					}
				}
				valor_funcion = strtok (NULL, ",");
			}
            conf.v_hist.push_back(tmp);
            tmp.reset_cant_funciones();
        }
    }


	// Ahora leer el archivo de configuracion de alarmas
    TiXmlDocument doc2(__ARCHIVO_CONF_ALARMAS);
	if( !doc2.LoadFile() ){
		mensaje_error = "Error Cargando Archivo de Configuracion: " +
			string(__ARCHIVO_CONF_ALARMAS) + "\n" + string(strerror(errno));
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}
	TiXmlHandle hdle2(&doc2);
	ele = hdle2.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <cantidad>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	cantidad = atoi( ele->GetText() );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <nombre>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"alarma") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <alarma>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	TiXmlAttribute * attr;
	char	nombre[_LONG_NOMB_ALARM_];
	Ealarma estado;
	Tvalor valor_comparacion;

	alarma a;
	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "grupo") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <grupo>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"subgrupo") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <subgrupo>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <nombre>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy(nombre,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"estado_x_defecto") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <estado_x_defecto>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		TEXTO2ESTADO(estado,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_comparacion") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <valor_comparacion>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo_comparacion") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <tipo_comparacion>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"historicos") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <historicos>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = sub_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "habilitado") != 0){
			escribir_log( "as-trazador", LOG_ERR, "Error Obteniendo Atributos de <habilitado> -> tipo" );
			throw runtime_error("Error Obteniendo Atributos de <habilitado> -> tipo");
		}
		if( strcmp( attr->Value(), "1" ) == 0 ){
			valor_comparacion.L = _ERR_DATO_;
			a.asignar_propiedades( 0, "", 0, "", nombre, estado,
				valor_comparacion, AERROR_, ENTERO_ );
//			conf.pha.v_alarm.push_back( a );
		}
	}
    return true;
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
            {"verbose", no_argument, &verbose_flag, 1
            }, {"brief",   no_argument, &verbose_flag, 0}, {"rec-all",	no_argument,		0, 'r'}, {"shm",		required_argument,	0, 's'}, {"tags",		required_argument,	0, 't'}, {"alarm",	required_argument,	0, 'a'}, {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rh:t:a:",long_options, &option_index);
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
        case 'h':
			strcpy(__ARCHIVO_CONF_HISTORICOS, optarg);
#ifdef _DEBUG_
            cout <<"Archivo de Configuracion de Historicos: "<< __ARCHIVO_CONF_HISTORICOS << endl;
#endif
            break;
        case 't':
            strcpy(__ARCHIVO_CONF_TAGS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Tags: " << __ARCHIVO_CONF_TAGS << endl;
#endif
            break;
        case 'a':
            strcpy(__ARCHIVO_CONF_ALARMAS, optarg);
#ifdef _DEBUG_
            cout << "Archivo de Configuracion de Alarmas: " << __ARCHIVO_CONF_ALARMAS << endl;
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
        cout << "\tProyecto Argos (as-trazador) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @brief	Obtiene los puntos de una variable en las distintas memorias
*			compartidas de acuerdo a un intervalo de tiempo
* @param	variable	Variable a buscar
* @param	inicio		Fecha de inicio (segundos desde epoch)
* @param	fin			Fecha de fin (segundos desde epoch)
* @return	string		Documento XML con los puntos obtenidos
*/
string obtener_tendencia_shm( char * variable, char * inicio, char * fin ){
	historico 		h;
	int						pos,
								puntos,
								indice,
								ptid;
	vector
	<historico>::iterator	it;
	struct
	timeval				desde,
								hasta,
								tiempo;
	string				xml;

	xml = "<desde>" + string( inicio ) + "</desde>\n"
			"<hasta>" + string( fin ) + "</hasta>\n";

	h.t.asignar_nombre( variable );

	it = find( conf.v_hist.begin(), conf.v_hist.end(), h );

	if( it != conf.v_hist.end() ){
		pos = it - conf.v_hist.begin();
	}
	else{
		xml += "<cantidad_puntos>0</cantidad_puntos>\n";
		return xml;
	}

	desde.tv_sec = atol( inicio );
	hasta.tv_sec = atol( fin );
	desde.tv_usec = hasta.tv_usec = 0;

	//Tvalor resultado = (conf.ph.trend + pos)->obtener_inicio_tendencia(desde, hasta, puntos, indice, tiempo);

	tendencia * tr = conf.hist_shmem.obtener_tendencia( pos );
	Tvalor resultado = tr->obtener_inicio_tendencia(desde, hasta, puntos, indice, tiempo);
	tags * aux = tr->obtener_tag();


	xml += "<cantidad_puntos>" + numero2string<int>(puntos, std::dec) + "</cantidad_puntos>\n";
	ptid = 1;
	while( puntos > 0 ){
		xml += "<punto id=\"" + numero2string<int>(ptid, std::dec) + "\" tiempo=\"" +
				numero2string<__time_t>(tiempo.tv_sec, std::dec) + "." +
				numero2string<__time_t>(tiempo.tv_usec, std::dec) + "\" valor=\"" +
				valor2texto(resultado,aux->tipo_dato()) + "\" />\n";

		resultado = tr->obtener_siguiente_punto( puntos, indice, tiempo);
		ptid++;
	}

	return xml;
}

/**
* @brief	Obtiene los puntos de una variable en la base de datos de
*			acuerdo a un intervalo de tiempo
* @param	variable	Variable a buscar
* @param	inicio		Fecha de inicio (segundos desde epoch)
* @param	fin			Fecha de fin (segundos desde epoch)
* @return	string		Documento XML con los puntos obtenidos
*/
string obtener_tendencia_bd( char * variable, char * inicio, char * fin ){
	string				xml,
								tendencia = "",
								consulta,
								campo;
	char		      *pghost = NULL,
								*pgport = NULL,
								*pgoptions = NULL,
								*pgtty = NULL;
	PGconn			  *conn;
	PGresult	   	*res;
	historico			h;
	int						pos;
	vector
	<historico>::iterator	it;
	tags 					*t;

	xml = "<desde>" + string( inicio ) + "</desde>\n"
			"<hasta>" + string( fin ) + "</hasta>\n";


	h.t.asignar_nombre( variable );
	it = find( conf.v_hist.begin(), conf.v_hist.end(), h );
	if( it != conf.v_hist.end() ){
		t = (*it).obtener_tag();
		switch(t->tipo_dato()){
			case CARAC_:
				campo = "punto_caracter";
			break;
			case CORTO_:
				campo = "punto_corto";
			break;
			case ENTERO_:
				campo = "punto_entero";
			break;
			case LARGO_:
				campo = "punto_entero";
			break;
			case REAL_:
				campo = "punto_real";
			break;
			case DREAL_:
				campo = "punto_dreal";
			break;
			case BIT_:
				campo = "punto_bit";
			break;
			case TERROR_:
			default:
				campo = "punto_entero";
		}
	}
	else{
		t = NULL;
		campo = "punto_entero";
	}

	// Crear la Conexion con la Base de Datos para buscar los puntos
	conn = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, conf.bdName, conf.bdLogin, conf.bdPwd);
	if (PQstatus(conn) == CONNECTION_BAD)
	{
	mensaje_error = "Error Conectandose a: " + string(conf.bdName) + "\n" +
		string( strerror( errno ) ) + "\n" + string( PQerrorMessage( conn ) );
	PQfinish(conn);
	escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
	throw runtime_error( mensaje_error.c_str() );
	}

	consulta = "SELECT t1.punto, EXTRACT(epoch FROM t1.fecha + t1.tiempo) AS "
				"tiempo_epoch FROM argos.\"t_" + string(variable) + "_tbl\" t1 "
				"WHERE EXTRACT(epoch FROM t1.fecha + t1.tiempo) "
				"BETWEEN " + string( inicio ) + " AND " + string( fin ) + " "
				"UNION SELECT t2." + campo + ", "
				"EXTRACT(epoch FROM t2.fecha + t2.tiempo) AS tiempo_epoch "
				"FROM " + string(conf.bdEsquema) + ".\"t_tendencias_tbl\" t2 "
				"WHERE t2.variable = '" + string(variable) + "' "
				"AND EXTRACT(epoch FROM t2.fecha + t2.tiempo) "
				"BETWEEN " + string( inicio ) + " AND " + string( fin ) + " "
				"ORDER BY tiempo_epoch LIMIT 10000;\n\n";
#ifdef _DEBUG_
	cout << consulta.c_str();
#endif

	res = PQexec(conn, consulta.c_str());
	if (!res ){
		mensaje_error = "Error Ejecutando Consulta: \n" + consulta + "\n" +
			string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( res ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
	}

	if( PQresultStatus(res) != PGRES_TUPLES_OK ){
		mensaje_error = "Error Devolviendo Tuplas: \n" + string( strerror( errno ) )
			+ "\n" + string( PQresultErrorMessage( res ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
	}
	else{
		int filas = PQntuples( res );
		int puntos = 0;
		for( int i = 0; i < filas; i++ ){
			if( !(PQgetisnull( res, i, 0 ) || PQgetisnull( res, i, 1 )) ){
				puntos++;
				tendencia += "<punto id=\"" + numero2string<int>(puntos, std::dec) + "\" tiempo=\"" +
						string( PQgetvalue( res, i, 1 ) ) + "\" valor=\"" +
						string( PQgetvalue( res, i, 0 ) )  + "\" />\n";
			}

		}
		xml += "<cantidad_puntos>" + numero2string<int>(puntos, std::dec) +
			"</cantidad_puntos>\n" + tendencia;
	}

	PQclear(res);
	PQfinish(conn);

	return xml;
}

/**
* @brief	Hilo que se encarga de atender el requisito de busqueda de los
*			datos historicos de variables que se encuentran en memoria
*			compartida
* @param	arg			Variables en XML, socket, etc.
*/
void obtener_desde_shm( Ttraza_pthread * arg){

	string			tendencia_xml,
					xml_out;
	char			nomb_tag[_LONG_NOMB_TAG_],
					fec_desde[20],
					fec_hasta[20];
	TiXmlDocument	res;

	res.Parse( arg->xml_in.c_str() );
	if( res.Error() ){
		mensaje_error = "Error Parseando: " + string( res.Value() ) + string( res.ErrorDesc() );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}

	TiXmlNode 		*n1 = res.FirstChild( "tendencias_xml" );
	TiXmlElement 	*ele1 = n1->ToElement()->FirstChildElement(),
					*ele2;
	TiXmlAttribute	*attr;

	if (!ele1 || strcmp(ele1->Value(), "cantidad_variables") != 0) {
		mensaje_error = "Error -> <cantidad_variables>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	int cant_var = atoi( ele1->GetText() );
	xml_out =	"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				"<tendencias_xml>\n"
				"<cantidad_variables>" + string(ele1->GetText()) + "</cantidad_variables>\n";

	ele1 = ele1->NextSiblingElement();
	if (!ele1 || strcmp(ele1->Value(), "tipo") != 0) {
		mensaje_error = "Error -> <tipo>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	ele1 = ele1->NextSiblingElement();
	for( int i = 1; i <= cant_var; i++ ){
		if (!ele1 || strcmp(ele1->Value(), "variable") != 0) {
			mensaje_error = "Error -> <variable id=\"" + numero2string<int>( i, std::dec  ) + "\" >\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = ele1->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			mensaje_error = "Error Obteniendo Atributos de <variable> -> id";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		if( atoi( attr->Value() ) != i ){
			mensaje_error = "Error Variable -> id " + numero2string<int>( i, std::dec ) + "\n";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "nombre") != 0){
			mensaje_error = "Error Obteniendo Atributos de <variable> -> nombre";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( nomb_tag, attr->Value() );

		xml_out += "<variable id=\"" + numero2string<int>( i, std::dec ) +
			"\" nombre=\"" + string(nomb_tag) + "\">\n";

		ele2 = ele1->FirstChildElement();
		if (!ele2 || strcmp(ele2->Value(), "desde") != 0) {
			mensaje_error = "Error -> <desde>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( fec_desde, ele2->GetText() );
		ele2 = ele2->NextSiblingElement();
		if (!ele2 || strcmp(ele2->Value(), "hasta") != 0) {
			mensaje_error = "Error -> <hasta>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( fec_hasta, ele2->GetText() );

		tendencia_xml = "";
		tendencia_xml = obtener_tendencia_shm( nomb_tag, fec_desde, fec_hasta );

		if( !tendencia_xml.empty() )
			xml_out += tendencia_xml + "</variable>\n";


		ele1 = ele1->NextSiblingElement();
	}

	xml_out += "</tendencias_xml>\n";

//#ifdef _DEBUG_
	cout << xml_out << "\ntamano: " << xml_out.size() << endl;
//#endif

    xml_out += '\u0000';

	if( send( arg->sockt, xml_out.c_str(), xml_out.size(), 0 ) == -1 ){
		mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
}

/**
* @brief	Hilo que se encarga de atender el requisito de busqueda de los
*			datos historicos de variables que se encuentran en base de datos
* @param	arg			Variables en XML, socket, etc.
*/
void obtener_desde_bd( Ttraza_pthread * arg){
	string			tendencia_xml,
					xml_out;
	char			nomb_tag[_LONG_NOMB_TAG_],
					fec_desde[20],
					fec_hasta[20];
	TiXmlDocument	res;

	res.Parse( arg->xml_in.c_str() );
	if( res.Error() ){
		mensaje_error = "Error Parseando: " + string( res.Value() ) + string( res.ErrorDesc() );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}

	TiXmlNode 		*n1 = res.FirstChild( "tendencias_xml" );
	TiXmlElement 	*ele1 = n1->ToElement()->FirstChildElement(),
					*ele2;
	TiXmlAttribute	*attr;

	if (!ele1 || strcmp(ele1->Value(), "cantidad_variables") != 0) {
		mensaje_error = "Error -> <cantidad_variables>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

	int cant_var = atoi( ele1->GetText() );
	xml_out =	"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				"<tendencias_xml>\n"
				"<cantidad_variables>" + string(ele1->GetText()) + "</cantidad_variables>\n";

	ele1 = ele1->NextSiblingElement();
	if (!ele1 || strcmp(ele1->Value(), "tipo") != 0) {
		mensaje_error = "Error -> <tipo>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	ele1 = ele1->NextSiblingElement();
	for( int i = 1; i <= cant_var; i++ ){
		if (!ele1 || strcmp(ele1->Value(), "variable") != 0) {
			mensaje_error = "Error -> <variable id=\"" + numero2string<int>( i, std::dec  ) + "\" >\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = ele1->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			mensaje_error = "Error Obteniendo Atributos de <variable> -> id";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		if( atoi( attr->Value() ) != i ){
			mensaje_error = "Error Variable -> id " + numero2string<int>( i, std::dec ) + "\n";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "nombre") != 0){
			mensaje_error = "Error Obteniendo Atributos de <variable> -> nombre";
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( nomb_tag, attr->Value() );

		xml_out += "<variable id=\"" + numero2string<int>( i, std::dec ) +
			"\" nombre=\"" + string(nomb_tag) + "\">\n";

		ele2 = ele1->FirstChildElement();
		if (!ele2 || strcmp(ele2->Value(), "desde") != 0) {
			mensaje_error = "Error -> <desde>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( fec_desde, ele2->GetText() );
		ele2 = ele2->NextSiblingElement();
		if (!ele2 || strcmp(ele2->Value(), "hasta") != 0) {
			mensaje_error = "Error -> <hasta>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy( fec_hasta, ele2->GetText() );

		tendencia_xml = "";
		tendencia_xml = obtener_tendencia_bd( nomb_tag, fec_desde, fec_hasta );

		if( !tendencia_xml.empty() )
			xml_out += tendencia_xml + "</variable>\n";


		ele1 = ele1->NextSiblingElement();
	}

	xml_out += "</tendencias_xml>\n";

#ifdef _DEBUG_
	cout << xml_out << "\ntamano: " << xml_out.size() << endl;
#endif

    xml_out += '\u0000';

	if( send( arg->sockt, xml_out.c_str(), xml_out.size(), 0 ) == -1 ){
		mensaje_error = "Error Enviando Datos: " + string( strerror( errno ) );
		escribir_log( "as-trazador", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}

}


