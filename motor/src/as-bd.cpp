/**
* @file	as-bd.cpp
* @brief		<b>Proceso encargado de Leer los Tags y las Alarmas de
				la Memoria Compartida de Tendecias y Alarmeros para
				Almacenarlos en Base de Datos.<br>
				Actualmente soporta PostgreSQL 8.3</b><br>
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <mqueue.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <float.h>
#include <signal.h>

#include "as-shmhistorico.h"
#include "as-historico.h"
#include "as-util.h"
#include "libpq-fe.h"
#include "tinyxml.h"
#include "version.h"


/*--------------------------------------------------------
	Conversion de un string a un Tipo de
	Dato valido
----------------------------------------------------------*/
#define CONVERSION(HACIA,DESDE,TIPO) \
	switch(TIPO){	\
		case CARAC_:	\
			HACIA = (double) DESDE.C;	\
		break;	\
		case CORTO_:	\
			HACIA = (double) DESDE.S;	\
		break;	\
		case ENTERO_:	\
			HACIA = (double) DESDE.I;	\
		break;	\
		case LARGO_:	\
			HACIA = (double) DESDE.L;	\
		break;	\
		case DREAL_:	\
			HACIA = (double) DESDE.D;	\
		break;	\
		case BIT_:	\
			HACIA = (double) DESDE.B;	\
		break;	\
		case REAL_:	\
		case TERROR_:	\
		default:	\
			HACIA = (double) DESDE.F;	\
	}



using namespace std;
using namespace argos;

static int	 		verbose_flag;			//!< Modo Verboso
string				mensaje_error;			//!< Mensaje de Error por Pantalla

namespace bdConf{



/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			almacena en base de datos para un registro historico</b><br>
*/
class configuracion{
public:
	char					hist_tags_shmem[100];		//!< Memoria Compartida de Tendencias
	char					hist_alarmas_shmem[100];	//!< Memoria Compartida del Alarmero
	char					mensaje_mqueue[100];		//!< Mensaje con datos para realizar el registro en BD
	char					bdName[100];				//!< Nombre de la Base de Datos
	char					bdEsquema[100];				//!< Nombre del Esquema de Base de Datos
	char					bdLogin[50];				//!< Nombre del Usuario de Base de Datos
	char					bdPwd[20];					//!< Contrasena de Base de Datos
	unsigned int	mensaje_long;				//!< Tamano del Mensaje de la Cola
	unsigned int	tiempo_consulta;			//!< Tiempo entre cada consulta
	unsigned int	tiempo_descarga;			//!< Tiempo para transferir Tablas de N1 a N2
	vector
	<alarma>			v_alarm;			//!< Vector de Alarmas a Almacenar
	vector
	<historico>		v_hist;				//!< Vector de Tags a Almacenar
	unsigned int	alarmas_buf;				//!< Tamano del Buffer Circular de Alarmas
	shm_historico	hist_shmem;

/**
* @brief	Constructor de Atributos por Defecto para la
*			Configuracion
* @return	Referencia al Objeto
*/
	configuracion(){
		strcpy(hist_tags_shmem,"tendencias");
		strcpy(hist_alarmas_shmem,"alarmeros");
		strcpy(mensaje_mqueue,"/procesar_historicos");
		strcpy(bdName,"complejo1");
		strcpy(bdEsquema,"argos");
		strcpy(bdLogin,"argos");
		strcpy(bdPwd,"argos");
		mensaje_long		= 128;
		tiempo_consulta		= 100000;
		tiempo_descarga		= 10;
		alarmas_buf			= 500;
	}

/**
* @brief	Acceder a la Cola donde obtiene los
*			Mensajes del Registrador
* @return	id_cola		Desciptor de la Cola de Mensajes
*/
	mqd_t abrir_cola(){
		mqd_t			id_cola;

		id_cola = mq_open(mensaje_mqueue, O_RDONLY, 00666, NULL );
		if(id_cola == (mqd_t)-1){
			mensaje_error = "Error Abriendo Cola de Mensajes: " + string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}

		struct
		mq_attr		atributos;

		if( mq_getattr( id_cola, &atributos ) < 0 ){
			mensaje_error = "Error Obteniendo Atributos de Cola de Mensajes: " + string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}

		mensaje_long = atributos.mq_msgsize < mensaje_long ?
			mensaje_long : atributos.mq_msgsize;

#ifdef _DEBUG_
    cout << "Abriendo Cola: " << endl <<
		"Mensajes Actuales:\t" << atributos.mq_curmsgs << endl <<
		"Cantidad Maxima Mensajes:\t" << atributos.mq_maxmsg << endl <<
		"Longitud de Mensajes:\t" << atributos.mq_msgsize << endl;
#endif

		return id_cola;
	}

};

}

bdConf::configuracion conf;				//!< Objeto de Configuracion de ssBD


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool					asignar_opciones(int, char **);
bool 					leer_config();
double 				obtener_media(Tmsgssbd, tendencia *, int );
double 				obtener_desviacion(Tmsgssbd, tendencia *, int, double );
template <class T>
string 				to_string(T t, ios_base & (*f)(ios_base&));
bool 					obtener_pz(Tmsgssbd, tendencia *, double &, double &,
													double &, double &, double &, int, double, double, double);
string 				crear_consulta(Tmsgssbd, tendencia *, int, double, double, int,
													double);
bool 					crear_tablas_en_bd(PGconn *, PGresult *);
bool					almacenar_tag(Tmsgssbd, PGconn *, PGresult *);
bool 					almacenar_alarma(Tmsgssbd, PGconn *, PGresult *);
void 					arrancar_hilo_transferir(int );
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal para el Almacenamiento en
*			BD de los Tags y Alarmas Registradas
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char **argv)
{
#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-bd) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-bd", LOG_INFO, "Iniciando Servicio..." );

	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }


	// Leer los archivos de configuracion de los tags, las alarmas y
	// de los datos historicos
    leer_config();

#ifdef _DEBUG_
    cout << conf.v_hist.size() << " Tags van a ser registrados" << endl;
    cout << conf.v_alarm.size() << " Alarmas van a ser registradas" << endl;
#endif
    sort(conf.v_hist.begin(), conf.v_hist.end());
    sort(conf.v_alarm.begin(), conf.v_alarm.end());
#ifdef _DEBUG_
    for ( unsigned int i = 0; i < conf.v_hist.size(); i++)
        cout << conf.v_hist[i];
	for ( unsigned int i = 0; i < conf.v_alarm.size(); i++)
        cout << conf.v_alarm[i] << endl;
#endif


	// Abrir la cola de mensajes para escuchar los registros a almacenar
	mqd_t 				cola = conf.abrir_cola();

  Tmsgssbd 			mensaje;
	unsigned int 	prio;
	int 					nbytes;


	// Abrir la memoria compartida de donde se obtendran los puntos
	// para almacenarlos en la Base de Datos
	string 				nombre_shm = "registrador/" + string(conf.hist_tags_shmem);
	conf.hist_shmem.inicializar_shm_tendencia( nombre_shm );
	conf.hist_shmem.obtener_shm_tendencia();

	// Abrir la memoria compartida de donde se obtendran las alarmas
	// para almacenarlas en la Base de Datos
	nombre_shm = "registrador/" + string(conf.hist_alarmas_shmem);
	conf.hist_shmem.inicializar_shm_alarmero( nombre_shm );
	conf.hist_shmem.obtener_shm_alarmero();

	char       		*pghost,
								*pgport,
								*pgoptions,
								*pgtty;

    pghost 		= NULL;
    pgport 		= NULL;
    pgoptions 	= NULL;
    pgtty 		= NULL;

    PGconn     *conn;
    PGresult   *res;

	// Crear la Conexion con la Base de Datos para almacenar los puntos
    conn = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, conf.bdName, conf.bdLogin, conf.bdPwd);
    if (PQstatus(conn) == CONNECTION_BAD)
    {
		mensaje_error = "Error Conectandose a: " + string(conf.bdName) + "\n" +
			string( strerror( errno ) ) + "\n" + string( PQerrorMessage( conn ) );
		PQfinish(conn);
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
    }

	crear_tablas_en_bd( conn, res );



	//--------------------Registro del Manejador de Senales
	struct
	sigaction 		n_accion;
	sigset_t 			set,
								pendset;

	n_accion.sa_flags = 0;
	sigemptyset( &n_accion.sa_mask );
	n_accion.sa_handler = arrancar_hilo_transferir;

	sigaction( SIGALRM, &n_accion, NULL );

	sigemptyset( &set );
	sigaddset( &set, SIGALRM );


	struct itimerval	intervalo;
	intervalo.it_value.tv_sec = conf.tiempo_descarga * 60;
	intervalo.it_value.tv_usec = 0;
	intervalo.it_interval.tv_sec = conf.tiempo_descarga * 60;
	intervalo.it_interval.tv_usec = 0;

	setitimer( ITIMER_REAL, &intervalo, NULL );


	while (true){
		sigprocmask( SIG_BLOCK, &set, NULL );

		// Esperar por mensajes para luego buscar los datos a almacenar
		if( -1 == ( nbytes = mq_receive( cola, (char *)&mensaje, conf.mensaje_long, &prio) ) ){
			mensaje_error = "Error Recibiendo Mensaje de Cola: " + string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}

		switch(mensaje.tipo){
			case _TIPO_TAG_:
				almacenar_tag(mensaje, conn, res);
			break;
			case _TIPO_ALARMA_:
				almacenar_alarma(mensaje, conn, res);
			break;
			default:
#ifdef _DEBUG_
				cout << "\tMensaje Ignorado, Tipo Desconocido\t"<< " -- BYTES: " << nbytes << endl;
#endif
				escribir_log( "as-bd", LOG_ERR, "Mensaje Ignorado, Tipo Desconocido" );
		}
		sigpending( &pendset );
		if( sigismember(&pendset, SIGALRM ) ){
			sigprocmask( SIG_UNBLOCK, &set, NULL );
		}
		usleep(conf.tiempo_consulta);
	}

	mq_close(cola);
    PQfinish(conn);
	return 1;
}

/**
* @brief 	Construye la Configuracion a partir de un archivo XML
* @return	verdadero		Configuracion exitosa
*/
bool leer_config() {
    TiXmlDocument doc(__ARCHIVO_CONF_HISTORICOS);
    if ( !doc.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_HISTORICOS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    TiXmlElement * sub_ele, *ssub_ele;
    if (!ele || strcmp(ele->Value(), "cola") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cola>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cola>");
    }
    strcpy( conf.mensaje_mqueue, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tags") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tags>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tags>");
    }
    strcpy( conf.hist_tags_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"alarmas") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarmas>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <alarmas>");
    }
    strcpy( conf.hist_alarmas_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"bd") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <bd>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <bd>");
    }
    sub_ele = ele->FirstChildElement();
	if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	strcpy(conf.bdName,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"esquema") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <esquema>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <esquema>");
	}
	strcpy(conf.bdEsquema,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"usuario") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <usuario>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <usuario>");
	}
	strcpy(conf.bdLogin,sub_ele->GetText());
	sub_ele = sub_ele->NextSiblingElement();
	if (!sub_ele || strcmp(sub_ele->Value(),"pwd") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <pwd>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <pwd>");
	}
	strcpy(conf.bdPwd,sub_ele->GetText());
	ele = ele->NextSiblingElement();
	if (!ele || strcmp(ele->Value(),"mensaje") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <mensaje>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <mensaje>");
    }
    sub_ele = ele->FirstChildElement();
	if (!sub_ele || strcmp(sub_ele->Value(), "longitud") != 0) {
		escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <longitud>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <longitud>");
	}
	conf.mensaje_long = atoi( sub_ele->GetText() == NULL ? "0" : sub_ele->GetText() );
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_escaner") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_escaner>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_escaner>");
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_entre_consultas") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_entre_consultas>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_entre_consultas>");
    }
	conf.tiempo_consulta = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_descarga_lote") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_descarga_lote>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_descarga_lote>");
    }
	conf.tiempo_descarga = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );
	ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"buffer_alarmas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <buffer_alarmas>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    conf.alarmas_buf = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );


	// Ahora leer el archivo de configuracion de tags
	TiXmlDocument doc1(__ARCHIVO_CONF_TAGS);
    if ( !doc1.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_TAGS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    TiXmlHandle hdle1(&doc1);
    ele = hdle1.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
        escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tag") != 0) {
    	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tag>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
    }
    char nombre_tag[_LONG_NOMB_TAG_], valor_char[100], tmp_funcion[100],
		*valor_funcion;
    Tvalor valor_tag;
    Tdato tipo_tag;
    historico tmp;
    Tlog tl;
    Tfunciones tf;
    unsigned int mts;
    unsigned int n_func_disponibles = numero_funciones_disponibles();
    for (int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
        sub_ele = ele->FirstChildElement();
        if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
        	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
            throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
        }
        strcpy(nombre_tag,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0) {
        	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_x_defecto>" );
            throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_x_defecto>");
        }
        strcpy(valor_char,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0) {
        	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
            throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
        }
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
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
            }
            TEXTO2LOG(tl,ssub_ele->GetText());
            tmp.asignar_tipo(tl);
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "tiempo") != 0) {
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo>");
            }
            tmp.asignar_tiempo( atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "maximo") != 0) {
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <maximo>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <maximo>");
            }
            tmp.asignar_maximo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "minimo") != 0) {
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <minimo>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <minimo>");
            }
            tmp.asignar_minimo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "muestras") != 0) {
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <muestras>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <muestras>");
            }
            mts = atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() );
            tmp.asignar_muestras( mts > _MAX_CANT_MUESTRAS_ ? _MAX_CANT_MUESTRAS_ : mts );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "funciones") != 0) {
            	escribir_log( "as-bd", LOG_ERR, "Error Parseando Archivo de Configuracion -> <funciones>" );
                throw runtime_error("Error Parseando Archivo de Configuracion -> <funciones>");
            }
            strcpy(tmp_funcion, ssub_ele->GetText());
            valor_funcion = strtok (tmp_funcion,",");
			while (valor_funcion != NULL){
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
        escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}
	TiXmlHandle hdle2(&doc2);
	ele = hdle2.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <cantidad>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	cantidad = atoi( ele->GetText() );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <nombre>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error(mensaje_error.c_str());
	}
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"alarma") != 0){
		mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <alarma>\n "
			+ string( strerror( errno ) );
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
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
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"subgrupo") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <subgrupo>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <nombre>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		strcpy(nombre,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"estado_x_defecto") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <estado_x_defecto>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		TEXTO2ESTADO(estado,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_comparacion") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <valor_comparacion>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo_comparacion") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <tipo_comparacion>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"historicos") != 0){
			mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <historicos>\n "
				+ string( strerror( errno ) );
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error(mensaje_error.c_str());
		}
		attr = sub_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "habilitado") != 0){
			escribir_log( "as-bd", LOG_ERR, "Error Obteniendo Atributos de <habilitado> -> tipo" );
			throw runtime_error("Error Obteniendo Atributos de <habilitado> -> tipo");
		}
		if( strcmp( attr->Value(), "1" ) == 0 ){
			valor_comparacion.L = _ERR_DATO_;
			a.asignar_propiedades( 0, "", 0, "", nombre, estado,
				valor_comparacion, AERROR_, ENTERO_ );
			conf.v_alarm.push_back( a );
		}
	}

    return true;
}

/**
* @brief	Lee las opciones pasadas al programa como parametros
*			en la linea de comandos
* @param	argc		Cantidad de Argumentos
* @param	argv		Lista de Argumentos
* @return	verdadero	Lectura Satisfactoria
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
            cout << "Archivo de Configuracion de Historicos: " << __ARCHIVO_CONF_HISTORICOS << endl;
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
        cout << "\tProyecto Argos (as-bd) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @brief	Calcular el promedio de una serie de datos de
*			la Memoria Compartida de Tendencias
* @param	m		Mensaje del Registrador
* @param	tr		Buffer de Tandencias de Tags
* @param	c		Cantidad de Datos
* @return	media	Media Aritmetica
*/
double obtener_media(Tmsgssbd m, tendencia * tr, int c){
	int 					indice = 0,
								muestras_validas = 0;
	double 				valor = 0.0,
								media = 0.0;
	tags					*aux;
	Tvalor				pto,
								*ptr_pto;

	for(int j = 0; j < c; j++, muestras_validas++ ){
		if ( m.desde + j >= _MAX_CANT_MUESTRAS_ ){
			indice = m.desde + j - _MAX_CANT_MUESTRAS_;
		}
		else{
			indice = m.desde + j;
		}

		ptr_pto = tr->obtener_punto(indice);
		pto = *ptr_pto;
		aux = tr->obtener_tag();
		CONVERSION(valor,pto,aux->tipo_dato())
		if( isnan(valor) || isinf(valor) ){
			muestras_validas--;
			continue;
		}
		media += valor;
	}
	if( c == 0 || muestras_validas == 0 )
		return DBL_MIN;
	media /= muestras_validas;
	return media;
}

/**
* @brief	Calcular la desviacion estandar de una serie de
*			datos de la Memoria Compartida de Tendencias
* @param	m			Mensaje del Registrador
* @param	tr			Buffer de Tandencias de Tags
* @param	c			Cantidad de Datos
* @param	md			Media Aritmetica
* @return	desviacion	Desviacion Estandar
*/
double obtener_desviacion(Tmsgssbd m, tendencia * tr, int c, double md){
	int 					indice = 0,
								muestras_validas = 0;
	double 				valor = 0.0,
								desviacion = 0.0;
	tags					*aux;
	Tvalor				pto,
								*ptr_pto;

	for(int k = 0; k < c; k++, muestras_validas++ ){
		if ( m.desde + k >= _MAX_CANT_MUESTRAS_ ){
			indice = m.desde + k - _MAX_CANT_MUESTRAS_;
		}
		else{
			indice = m.desde + k;
		}
		ptr_pto = tr->obtener_punto(indice);
		pto = *ptr_pto;
		aux = tr->obtener_tag();
		CONVERSION(valor,pto,aux->tipo_dato())
		if( isnan(valor) || isinf(valor) ){
			muestras_validas--;
			continue;
		}
		desviacion += pow((valor - md),2);
	}
	if( c <= 1 || muestras_validas <= 1 )
		return DBL_MIN;
	desviacion = sqrt( desviacion / (c - 1) );
	return desviacion;
}

/**
* @brief	Convierte un numero con base dada en string
* @param	t			Numero
* @param	ios_base	Base del Numero
* @return	str			Cadena de Caracteres
*/
template <class T> string to_string(T t, ios_base & (*f)(ios_base&))
{
  ostringstream oss;
  oss << f << t;
  return oss.str();
}

/**
* @brief	Calcular el Punto Z de una serie de
*			datos de la Memoria Compartida de Tendencias
* @param	m			Mensaje del Registrador
* @param	tr			Buffer de Tendencias de Tags
* @param	pz_ant		Punto Z muestra anterior
* @param	pz_act		Punto Z muestra actual
* @param	pz_sig		Punto Z muestra siguiente
* @param	dif_ad		Diferencia PZ Anterior
* @param	dif_at		Diferencia PZ Siguiente
* @param	p			Indice dentro del Buffer
* @param	md			Media Artimetica
* @param	dv			Desviacion Estandar
* @param	v			Valor del Tag
* @return	verdadero	Desviacion Estandar
*/
bool obtener_pz(Tmsgssbd m, tendencia * tr, double & pz_ant, double & pz_act,
	double & pz_sig, double & dif_ad, double & dif_at, int p, double md, double dv,
	double v){

	double				valor_double_a = 0.0,
								valor_double_d = 0.0;
	tags					*aux;
	Tvalor				pto,
								*ptr_pto;

	if( p < _MAX_CANT_MUESTRAS_ && p > 0 ){
		pz_act = abs((md - v)/dv);
		ptr_pto = tr->obtener_punto(p-1);
		pto = *ptr_pto;
		aux = tr->obtener_tag();
		CONVERSION(valor_double_a,pto,aux->tipo_dato())
		pz_ant = abs((md - valor_double_a)/dv);
		ptr_pto = tr->obtener_punto(p+1);
		pto = *ptr_pto;
		CONVERSION(valor_double_d,pto,aux->tipo_dato())
		pz_sig = abs((md - valor_double_d)/dv);
		dif_ad = v - valor_double_d;
		dif_at = v - valor_double_a;
	}
	else if(p <= 0){
		pz_act = abs((md - v)/dv);
		pz_ant = pz_act;
		ptr_pto = tr->obtener_punto(p+1);
		pto = *ptr_pto;
		aux = tr->obtener_tag();
		CONVERSION(valor_double_d,pto,aux->tipo_dato())
		pz_sig = abs((md - valor_double_d)/dv);
		dif_ad = v - valor_double_d;
		dif_at = 0.0;
	}
	else{
		pz_act = abs((md - v)/dv);
		ptr_pto = tr->obtener_punto(p-1);
		pto = *ptr_pto;
		aux = tr->obtener_tag();
		CONVERSION(valor_double_a,pto,aux->tipo_dato())
		pz_ant = abs((md - valor_double_a)/dv);
		pz_sig = pz_act;
		dif_ad = 0.0;
		dif_at = v - valor_double_a;
	}
	return true;
}

/**
* @brief	Al Iniciar el proceso se encarga de crear las tablas
*			en Base de Datos para los historicos de Tags y Alarmas
* @param	c			Conexion con PgSQL
* @param	r			ResultSet de PgSQL
* @return	verdadero	Creacion Exitosa
*/
bool crear_tablas_en_bd(PGconn * c, PGresult * r){
	string 				consulta;

    consulta = "SELECT tablename FROM pg_tables WHERE tableowner='" +
		string(conf.bdLogin) + "' AND schemaname='" + string(conf.bdEsquema) + "';";
	r = PQexec(c, consulta.c_str() );
    if (!r || PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        mensaje_error = "Error Retornando Registros: " +
			string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( r ) );
		PQclear( r );
		PQfinish(c);
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
    }

	// Crear las Tablas de Nivel Uno que almacenaran los puntos
	string 				tmp("\0"),
								tabla1,
								tabla2;

	bool 					existe = false,
								al_menos_una = false,
								existe_tendencias = false,
								existe_alarmeros = false;

	for (unsigned int i = 0; i < conf.v_hist.size(); i++)
    {
    	tabla1 = "t_" + string( conf.v_hist[i].t.nombre ) + "_tbl";
    	existe = false;
    	for( int j = 0; j < PQntuples(r); j++ ){
			tabla2 = string( PQgetvalue(r, j, 0) );
    		if( tabla1.compare( tabla2 ) == 0 ){
    			existe = true;
    			break;
    		}
    	}
    	if( !existe ){
    		switch(conf.v_hist[i].t.tipo){
			case CARAC_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto \"char\", fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
        	case CORTO_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto smallint, fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
			case ENTERO_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto integer, fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
        	case LARGO_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto bigint, fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
        	case DREAL_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto double precision, fecha date, tiempo time without time zone," +
				"promedio double precision, desviacion double precision, muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
        	case BIT_:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto boolean, fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
        	break;
        	case REAL_:
        	case TERROR_:
        	default:
				tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\"(punto numeric(16,6), fecha date, tiempo time without time zone," +
				"promedio numeric(16,6), desviacion numeric(16,6), muestras integer" +
				") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) + ".\"" +
				tabla1 + "\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
				//"COMMENT ON TABLE " + string(conf.bdEsquema) + "." + tabla1 +
				//" \"Variable " + string( hist[i].t.nombre ) + "\";\n\n";
			}
    		al_menos_una = true;
    	}
    }

    for( int j = 0; j < PQntuples(r); j++ ){
		tabla2 = string( PQgetvalue(r, j, 0) );
		if( tabla2.compare( "t_tendencias_tbl" ) == 0 ){
			existe_tendencias = true;
			continue;
		}
		if( tabla2.compare( "a_alarmeros_tbl" ) == 0 ){
			existe_alarmeros = true;
			continue;
		}
	}

	if( !existe_tendencias ){
		tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) +
			".\"t_tendencias_tbl\"(punto_real numeric(16,6), punto_entero integer," +
			"punto_dreal double precision, punto_bit boolean, punto_largo bigint," +
			"punto_corto smallint, punto_caracter \"char\", tipo integer, fecha date, " +
			"tiempo time without time zone, promedio numeric(16,6), " +
			" desviacion numeric(16,6), muestras integer, variable character(100)" +
			") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) +
			".\"t_tendencias_tbl\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
		al_menos_una = true;
	}
	if( !existe_alarmeros ){
		tmp = tmp + "CREATE TABLE " + string(conf.bdEsquema) +
			".\"a_alarmeros_tbl\"(estado integer, fecha date, " +
			"tiempo time without time zone, nombre character(100)" +
			") WITH (OIDS=FALSE);\nALTER TABLE " + string(conf.bdEsquema) +
			".\"a_alarmeros_tbl\" OWNER TO " + string(conf.bdLogin) + ";\n\n";
		al_menos_una = true;
	}

    PQclear(r);

    if( al_menos_una ){
		consulta = "BEGIN;\n" + tmp + "COMMIT;\n";
#ifdef _DEBUG_
		cout << consulta.c_str();
#endif
		r = PQexec(c, consulta.c_str());
		if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
		{
			mensaje_error = "Error Creando Tablas: \n" + consulta + "\n" +
				string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( r ) );
			PQclear(r);
			PQfinish(c);
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		PQclear(r);
    }

	return true;
}

/**
* @brief	Prepara la Consulta para Registrar Tags y Alarmas
*			en las Tablas de Base de Datos
* @param	m		Mensaje del Registrador
* @param	tr		Buffer de Tendencias de Tags
* @param	c		Cantidad de Datos
* @param	md		Media Artimetica
* @param	dv		Desviacion Estandar
* @param	p		Indice dentro del Buffer
* @param	v		Valor del Tag
* @return	cons	Consulta SQL
*/
string crear_consulta(Tmsgssbd m, tendencia * tr, int c, double md, double dv, int p,
	double v){
	string 				cons;
	struct
	timeval				t_tiempo;
	time_t 				tiempo;
	tm 						*ltiempo;
	tags					*aux;

	t_tiempo = tr->obtener_fecha_punto(p);
	tiempo = t_tiempo.tv_sec;
	ltiempo = localtime(&tiempo);
	aux = tr->obtener_tag();

	switch(aux->tipo_dato()){
		case CARAC_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "('" +
					to_string<char>( v, std::dec ) +
					"','" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case CORTO_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<short>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case ENTERO_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<int>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case LARGO_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<__int64_t>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case DREAL_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<double>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<double>( md, std::dec ) + "," +
					to_string<double>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case BIT_:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<bool>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
		break;
		case REAL_:
		case TERROR_:
		default:
			cons += "INSERT INTO " + string(conf.bdEsquema) + ".\"t_" + string(m.nombre_tag) +
					"_tbl\"(punto,fecha,tiempo,promedio,desviacion,muestras) VALUES "
					+ "(" +
					to_string<float>( v, std::dec ) +
					",'" +
					to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
					to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
					to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
					to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
					to_string<int>( t_tiempo.tv_usec, std::dec )
					+ "'," + to_string<float>( md, std::dec ) + "," +
					to_string<float>( dv, std::dec ) + "," +
					to_string<int>( c, std::dec ) + ");\n\n";
	}
	return cons;
}

/**
* @brief	Ejecutar la Consulta para Almacenar en Base de
*			Datos los historicos de Tags
* @param	m			Mensaje del Registrador
* @param	c			Conexion con PgSQL
* @param	r			ResultSet de PgSQL
* @return	verdadero	Insercion Exitosa
*/
bool almacenar_tag(Tmsgssbd m, PGconn * c, PGresult * r){
	int 					cantidad = 0,
								pos = 0;
	double 				media = 0.0,
								desviacion = 0.0,
								pz_anterior = 0.0,
								pz_actual = 0.0,
								pz_siguiente = 0.0,
								diferencia_atras = 0.0,
								diferencia_adelante = 0.0,
								valor_double = 0.0;
	string 				consulta = "\0";
	bool					al_menos_una = false;
	tags					*aux;
	Tvalor				pto,
								*ptr_pto;

#ifdef _DEBUG_
	cout << "TAG: " << m.nombre_tag << " , POSICION: " <<  m.posicion
			<< " , INICIO: " << m.desde << " , HASTA: " << m.hasta << endl;
#endif

	cantidad = (m.hasta - m.desde);
	if( cantidad < 0 )
		cantidad += _MAX_CANT_MUESTRAS_;


	// Indica que hay puntos por almacenar en la Base de Datos
	al_menos_una = false;
	tendencia * tr = conf.hist_shmem.obtener_tendencia( m.posicion );

	// Escoger el Tipo de Procesamiento
	switch(tr->obtener_funcion_historico(0)){
		case AVG_:
			// Calcular el Promedio de las Muestras
			media = obtener_media(m, tr, cantidad);
			// Obtener la desviacion estandar de las muestras
			desviacion = obtener_desviacion(m, tr, cantidad, media);
			consulta += crear_consulta( m, tr, cantidad, media, desviacion,
							m.hasta, media );
			al_menos_una = true;

		break;
		case PZ_:
			// Calcular el Promedio de las Muestras
			media = obtener_media(m, tr, cantidad);
			// Obtener la desviacion estandar de las muestras
			desviacion = obtener_desviacion(m, tr, cantidad, media);

			if( desviacion == 0.0 ){
				break;
			}

			for(int i = 0; i < cantidad ; i++){
				if ( m.desde + i >= _MAX_CANT_MUESTRAS_ ){
					pos = m.desde + i - _MAX_CANT_MUESTRAS_;
				}
				else{
					pos = m.desde + i;
				}

				ptr_pto = tr->obtener_punto(pos);
				pto = *ptr_pto;
				aux = tr->obtener_tag();
				CONVERSION(valor_double,pto,aux->tipo_dato())
				if( isnan(valor_double) || isinf(valor_double) ){
					break;
				}

				// Calcular el Puntaje Zeta de las Muestras
				obtener_pz(m, tr, pz_anterior, pz_actual, pz_siguiente,
					diferencia_adelante, diferencia_atras, pos, media, desviacion,
					valor_double);

#ifdef _DEBUG_
				cout << "Punto: " << valor_double <<
					" Media: " << media << " Desviacion: " << desviacion <<
					" PZ: " << pz_actual << endl;
#endif
				// Existen datos que se deben Almacenar
				if( ( pz_actual > 1.2 && (diferencia_adelante != 0.0 || diferencia_atras != 0.0 ) ) ||
					( pz_actual <= 1.2 && ( pz_anterior > 1.2 || pz_siguiente > 1.2 ) )
					){

					consulta += crear_consulta( m, tr, cantidad, media, desviacion,
						pos, valor_double );
					al_menos_una = true;

				}

			}

		break;
		case NO_:
			// Calcular el Promedio de las Muestras
			media = obtener_media(m, tr, cantidad);
			// Obtener la desviacion estandar de las muestras
			desviacion = obtener_desviacion(m, tr, cantidad, media);
			if( desviacion == 0.0 ){
				break;
			}
			for(int i = 0; i < cantidad ; i++){
				if ( m.desde + i >= _MAX_CANT_MUESTRAS_ ){
					pos = m.desde + i - _MAX_CANT_MUESTRAS_;
				}
				else{
					pos = m.desde + i;
				}
				ptr_pto = tr->obtener_punto(pos);
				pto = *ptr_pto;
				aux = tr->obtener_tag();
				CONVERSION(valor_double,pto,aux->tipo_dato())
				if( isnan(valor_double) || isinf(valor_double) ){
					break;
				}

#ifdef _DEBUG_
				cout << "Punto: " << valor_double <<
					" Media: " << media << " Desviacion: " << desviacion << endl;
#endif

				consulta += crear_consulta( m, tr, cantidad, media, desviacion,
					pos, valor_double );
			}
			al_menos_una = true;

		break;
		case STD_:
		case VAR_:
		case FERROR_:
		default:
#ifdef _DEBUG_
			cout << "\tTipo de Funcion de Agrupacion No Soportada..." << endl;
#endif
			escribir_log( "as-bd", LOG_WARNING, "Tipo de Funcion de Agrupacion No Soportada"  );
	}

	// Existe al menos un dato a insertar en las tablas de nivel 1
	if( al_menos_una ){
		consulta = "BEGIN;\n" + consulta + "COMMIT;\n\n";
#ifdef _DEBUG_
		cout << consulta.c_str();
#endif
		r = PQexec(c, consulta.c_str());
		if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
		{
			mensaje_error = "Error Insertando Punto: \n" + consulta + "\n" +
				string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( r ) );
			PQclear(r);
			PQfinish(c);
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		PQclear(r);
	}
	consulta = "\0";
	return true;
}

/**
* @brief	Ejecutar la Consulta para Almacenar en Base de
*			Datos los historicos de Alarmas
* @param	m			Mensaje del Registrador
* @param	c			Conexion con PgSQL
* @param	r			ResultSet de PgSQL
* @return	verdadero	Insercion Exitosa
*/
bool almacenar_alarma(Tmsgssbd m, PGconn * c, PGresult * r){
	int						cantidad = 0,
								indice = m.desde;
	string 				consulta = "\0";
	alarma				*aux;
	bool					al_menos_una = false;
	struct
	timeval				t_tiempo;
	time_t 				tiempo;
	tm 						*ltiempo;
	alarmero			*alr;

#ifdef _DEBUG_
	cout << "ALARMA: " << m.nombre_alarma << " , POSICION: " <<  m.posicion
			<< " , INICIO: " << m.desde << " , HASTA: " << m.hasta << endl;
#endif

	cantidad = (m.hasta - m.desde);
	if( cantidad < 0 )
		cantidad += conf.alarmas_buf;

	for( int i = 0; i < cantidad; i++,indice++ ){
		if( indice >= conf.alarmas_buf ){
			indice = 0;
		}

		alr = conf.hist_shmem.obtener_alarmero( indice );
		aux = alr->obtener_alarma();
		t_tiempo = alr->obtener_fecha_alarma();
		tiempo = t_tiempo.tv_sec;
		ltiempo = localtime(&tiempo);

		consulta += "INSERT INTO " + string(conf.bdEsquema) +
			".\"a_alarmeros_tbl\"(estado,fecha,tiempo,nombre) VALUES "
				+ "(" +
				to_string<int>( aux->obtener_estado(), std::dec ) + ",'" +
				to_string<int>( ltiempo->tm_year+1900, std::dec ) + "-" +
				to_string<int>( ltiempo->tm_mon+1, std::dec ) + "-" +
				to_string<int>( ltiempo->tm_mday, std::dec ) + "','" +
				to_string<int>( ltiempo->tm_hour, std::dec ) + ":" +
				to_string<int>( ltiempo->tm_min, std::dec ) + ":" +
				to_string<int>( ltiempo->tm_sec, std::dec ) + "." +
				to_string<int>( t_tiempo.tv_usec, std::dec )
				+ "','" + string( aux->obtener_nombre() ) + "');\n\n";
		al_menos_una = true;

	}

	// Existe al menos un dato a insertar en las tablas de nivel 1
	if( al_menos_una ){
		consulta = "BEGIN;\n" + consulta + "COMMIT;\n\n";
#ifdef _DEBUG_
		cout << consulta.c_str();
#endif
		r = PQexec(c, consulta.c_str());
		if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
		{
			mensaje_error = "Error Insertando Alarma: \n" + consulta + "\n" +
				string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( r ) );
			PQclear(r);
			PQfinish(c);
			escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		PQclear(r);
	}
	consulta = "\0";

	return true;
}

/**
* @brief	Ejecutar la Consulta para Transferir en Base de
*			Datos los historicos de las Tablas de Nivel 1 a
*			Tablas de Nivel 2
*/
void transferir2tablas_nivel2(){
	string 			consulta = "\0",
					campo;
	char       		*pghost = NULL,
					*pgport = NULL,
					*pgoptions = NULL,
					*pgtty = NULL;
    PGconn     		*conn;
    PGresult   		*res;

	// Crear la Conexion con la Base de Datos para almacenar los puntos
    conn = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, conf.bdName, conf.bdLogin, conf.bdPwd);
    if (PQstatus(conn) == CONNECTION_BAD)
    {
		mensaje_error = "Error Conectandose a: " + string(conf.bdName) + "\n" +
			string( strerror( errno ) ) + "\n" + string( PQerrorMessage( conn ) );
		PQfinish(conn);
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
    }

	for( int i = 0; i < conf.v_hist.size(); i++ ){
		switch(conf.v_hist[i].t.tipo){
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
				campo = "punto_largo";
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
			default:
				continue;
		}

		consulta += "INSERT INTO " + string(conf.bdEsquema) + ".t_tendencias_tbl(" +
			campo + ",tipo,fecha,tiempo,promedio,desviacion,muestras,variable) " +
			"SELECT punto," + to_string<int>( conf.v_hist[i].t.tipo, std::dec ) +
			",fecha,tiempo,promedio,desviacion,muestras,'" +
			string( conf.v_hist[i].t.nombre ) + "' as variable FROM " +
			string(conf.bdEsquema) + ".\"t_" + string( conf.v_hist[i].t.nombre ) +
			"_tbl\" WHERE fecha <= CURRENT_DATE AND tiempo < CURRENT_TIME;\n";
		consulta += "DELETE FROM " + string(conf.bdEsquema) +
			".\"t_" + string( conf.v_hist[i].t.nombre ) +
			"_tbl\" WHERE fecha <= CURRENT_DATE AND tiempo < CURRENT_TIME;\n";
	}

	consulta = "BEGIN;\n" + consulta + "COMMIT;\n";

#ifdef _DEBUG_
	cout << consulta.c_str();
#endif


	res = PQexec(conn, consulta.c_str());
	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		mensaje_error = "Error Creando Tablas: \n" + consulta + "\n" +
			string( strerror( errno ) ) + "\n" + string( PQresultErrorMessage( res ) );
		PQclear(res);
		PQfinish(conn);
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}
	PQclear(res);
	PQfinish(conn);
}

/**
* @brief	Crea un Hilo para Ejecutar la Consulta de Transferir en Base de
*			Datos los historicos de las Tablas de Nivel 1 a
*			Tablas de Nivel 2, en un intervalo de tiempo definido
* @param	signum
*/
void arrancar_hilo_transferir(int signum){
	pthread_t		hilo;

#ifdef _DEBUG_
	cout << "\n\tRECIBIDA SENAL [" << signum << "]" << endl;
	cout << "\tIniciando Transferencia de Datos a Tablas de Nivel 2....\n\n" << endl;
#endif

	if( pthread_create( &hilo, NULL, (void *(*)(void *))transferir2tablas_nivel2,
			NULL ) ){
		mensaje_error = "Error Creando Hilo para Actualizar Registros: "
				+ string( strerror( errno ) );
		escribir_log( "as-bd", LOG_ERR, mensaje_error.c_str() );
	}
	pthread_detach( hilo );

}

