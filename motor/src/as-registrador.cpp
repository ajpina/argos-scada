/**
* @file	as-registrador.cpp
* @brief		<b>Proceso encargado de adquirir los valores de las
*				Memorias Compartidas de Tags y Alarmas para crear los
*				Buffer Circulares de Tendencias y Alarmeros en Memoria
*				Compartida y enviar Mensajes a traves de Colas al
*				proceso que almacenara estos datos en BD</b><br>
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
#include <vector>
#include <algorithm>
#include <ctime>
#include <cfloat>
#include <cstring>
#include <fstream>
#include <climits>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/capability.h>
#include <mqueue.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <sched.h>

#include "as-shmhistorico.h"
#include "tinyxml.h"
#include "as-util.h"
#include "as-tags.h"
#include "as-alarma.h"
#include "as-shmtag.h"
#include "as-shmalarma.h"


using namespace std;
using namespace argos;


static int	 		verbose_flag;		//!< Modo Verboso
string					mensaje_error;		//!< Mensaje de Error por Pantalla


namespace regConf {






/**
* @class 	configuracion
* @brief 	<b>Objeto usado para la configuracion del Proceso que
*			registra los valores de los Tags y las Alarmasen los Buffer Circulares</b><br>
*/
class configuracion {
public:
    char				hist_tags_shmem[100];		//!< Nombre del Buffer Circular de Tags
    char				tags_shmem[100];			//!< Memoria Compartida de Tags
    char				hist_alarmas_shmem[100];	//!< Nombre del Buffer Circular de Alarmas
    char				alarmas_shmem[100];			//! Memoria Compartida de Alarmas
    char				mensaje_mqueue[100];		//!< Cola de Mensajes con ssBD
    unsigned
    int					mensaje_long;				//!< Tamano del Mensaje
    unsigned
    int					mensaje_cant;				//!< Canrtidad de Mensajes
    unsigned
    int					tiempo_escaner;				//!< Tiempo de Muestreo para los Datos

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los argumentos
	para el hilo que actualiza el buffer circular (Alarmero)
----------------------------------------------------------*/
		typedef struct _Tpqte_hilo_alarmas{
				shm_thalarma			shm;				//!< Memoria Compartida de Alarmas
				vector <alarma>		v_alarm;			//!< Vector de Alarmas a Registrar
		}Tpqte_hilo_alarmas;

    Tpqte_hilo_alarmas	pha;						//!< Estructura para las Alarmas a Registrar

/*--------------------------------------------------------
	Estructura de dato usada para encapsular los argumentos
	para el hilo que actualiza el buffer circular (Tendencia)
----------------------------------------------------------*/
		typedef struct _Tpqte_hilo_tags{
				shm_thtag					shm;		//!< Memoria Compartida de Tags
				vector
				<argos::historico>	v_hist;		//!< Vector de Tags a Registrar
		}Tpqte_hilo_tags;

    Tpqte_hilo_tags	pht;						//!< Estructura para los Tags a Registrar
    shm_historico		hist_shmem;
    unsigned int		alarmas_buf;				//!< Cantidad de Alarmas en el Buffer
    unsigned int		alarmas2mensaje;			//!< Alarmas registradas para poder enviar el Mensaje



    /**
    * @brief	Constructor de Atributos por Defecto para la
    *			Configuracion
    * @return	Referencia al Objeto
    */
    configuracion() {
        strcpy(hist_tags_shmem,"tendencias");
        strcpy(tags_shmem,"tags");
        strcpy(hist_alarmas_shmem,"alarmeros");
        strcpy(alarmas_shmem,"alarmas");
        strcpy(mensaje_mqueue,"/procesar_historicos");
        mensaje_long		= 128;
        mensaje_cant		= 4096;
        tiempo_escaner		= 10000;
        alarmas_buf			= 500;
        alarmas2mensaje 	= 250;
    }


    /**
    * @brief	Crea la Cola de Mensajes para Enviar al proceso ssBD
    *			la Autorizacion para que guarde en BD los historicos
    * @return	id_cola		Descriptor de la Cola de Mensajes
    */
    mqd_t crear_cola() {

        struct mq_attr atributos;
        mqd_t id_cola;
        memset(&atributos,0,sizeof(atributos));

        // Tratar de redimensionar la cola de mensajes
        cap_t cgp = cap_get_proc();

        cap_flag_t flag = CAP_PERMITTED;
        cap_flag_value_t value;
        cap_value_t v = CAP_SYS_RESOURCE;
        cap_get_flag( cgp, v, flag, &value );

        if(value==CAP_SET) {
            atributos.mq_maxmsg = mensaje_cant;
            atributos.mq_msgsize = mensaje_long;
            atributos.mq_flags = O_NONBLOCK;
#ifdef _DEBUG_
            cout << "Redimensionando la Cola de Mensajes, Cantidad de Mensajes: " << atributos.mq_maxmsg << endl;
            cout << "Abriendo Cola de Mensajes: " << mensaje_mqueue << endl;
#endif
            id_cola = mq_open(mensaje_mqueue, O_CREAT | O_WRONLY, 00666, &atributos );
        } else {
#ifdef _DEBUG_
            cout << "No tiene privilegios para Redimensionar la Cola..." << endl;
#endif
            escribir_log( "as-registrador", LOG_WARNING, "No tiene privilegios para Redimensionar la Cola..." );
            id_cola = mq_open(mensaje_mqueue, O_CREAT | O_WRONLY, 00666, NULL );
        }

        if(id_cola == (mqd_t)-1) {
            mensaje_error = "Error Abriendo Cola de Mensajes: " + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error( mensaje_error.c_str() );
        }

        return id_cola;
    }

};

}


regConf::configuracion conf;			//!< Objeto de Configuracion de ssREGISTRADOR


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool asignar_opciones(int, char **);
bool leer_config(regConf::configuracion *);
unsigned long int calc_tiempo_transcurrido( struct timeval, struct timeval );
bool shmem2disco( tendencia *, unsigned int);
void registrar_tags(mqd_t *);
void registrar_alarmas(mqd_t *);
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
    cout << "\tProyecto Argos (as-registrador) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

    escribir_log( "as-registrador", LOG_INFO, "Iniciando Servicio..." );

    // Obtener los Archivos de Configuracion
    if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    // Cargar las Configuraciones en vectores utilitarios
    leer_config(&conf);

#ifdef _DEBUG_
    cout << conf.pht.v_hist.size() << " Tags van a ser registrados" << endl;
    cout << conf.pha.v_alarm.size() << " Alarmas van a ser registradas" << endl;
#endif

    // Ordenar por Nombre de Tags y Alarmas
    sort(conf.pht.v_hist.begin(), conf.pht.v_hist.end());
    sort(conf.pha.v_alarm.begin(), conf.pha.v_alarm.end());

#ifdef _DEBUG_
    cout << "\n\tListado de Tags....\n";
    for ( unsigned int i = 0; i < conf.pht.v_hist.size(); i++) {
        cout << conf.pht.v_hist[i];
    }
    cout << "\n\n\tListado de Alarmas....\n";
    for ( unsigned int i = 0; i < conf.pha.v_alarm.size(); i++) {
        cout << conf.pha.v_alarm[i] << endl;
    }

#endif

    // Crear el Directorio que contendra las memorias compartidas
    // de tendencias y alarmeros
    struct stat datos_directorio;
    lstat("/dev/shm/registrador", &datos_directorio);
    if ( !S_ISDIR( datos_directorio.st_mode ) ) {
        if (-1 == mkdir("/dev/shm/registrador",0777) ) {
            mensaje_error = "Error Creando Directorio /dev/shm/registrador: " + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error( mensaje_error.c_str() );
        }
    }

    mqd_t cola = conf.crear_cola();

    pthread_t hilo_tags;
    pthread_t hilo_alarmas;

    //----------------Arrancar el Hilo para Registrar los Tags
#ifdef _DEBUG_
    cout << "Arrancando Hilo para Tags....." << endl;
#endif

    if( pthread_create( &hilo_tags, NULL, (void *(*)(void *))registrar_tags,
                        &cola ) ) {
        mensaje_error = "Error Creando Hilo para Registrar Tags: "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    //----------------Arrancar el Hilo para Escanear las Alarmas
#ifdef _DEBUG_
    cout << "Arrancando Hilo para Alarmas....." << endl;
#endif
    if( pthread_create( &hilo_alarmas, NULL, (void *(*)(void *))registrar_alarmas,
                        &cola ) ) {
        mensaje_error = "Error Creando Hilo para Registrar Alarmas: "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    pthread_join( hilo_tags, NULL );
		pthread_join( hilo_alarmas, NULL );



    return 1;
}

/**
* @brief	Calcula el tiempo transcurrido desde la ultima adquisicion
*			hasta el instante actual
* @param	ahora	Instante Actual
* @param	antes	Instante del Ultimo registro
* @return	tiempo	Tiempo Transcurrido
*/
unsigned long int calc_tiempo_transcurrido( struct timeval ahora, struct timeval antes ) {
    if (antes.tv_usec > ahora.tv_usec) {
        ahora.tv_usec += 1000000;
        ahora.tv_usec--;
    }
    return (ahora.tv_sec - antes.tv_sec)*1000 + (ahora.tv_usec - antes.tv_usec)/1000;
}

/**
* @brief 	Obtiene los parametros y argumentos de configuracion
*			a partir de un archivo XML
* @param 	c			Atributos de Configuracion
* @return	verdadero	Configuracion Exitosa
*/
bool leer_config(regConf::configuracion * c) {

    // Primero Leer el archivo de configuracion de historicos
    TiXmlDocument doc(__ARCHIVO_CONF_HISTORICOS);
    if ( !doc.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_HISTORICOS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    TiXmlElement * sub_ele, *ssub_ele;
    if (!ele || strcmp(ele->Value(), "cola") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <cola>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy( c->mensaje_mqueue, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tags") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tags>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy( c->hist_tags_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"alarmas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <alarmas>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy( c->hist_alarmas_shmem, ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"bd") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <bd>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"mensaje") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <mensaje>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    sub_ele = ele->FirstChildElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "longitud") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <longitud>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    c->mensaje_long = atoi( sub_ele->GetText() == NULL ? "0" : sub_ele->GetText() );
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "cantidad") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <cantidad>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    c->mensaje_cant = atoi( sub_ele->GetText() == NULL ? "0" : sub_ele->GetText() );
    sub_ele = sub_ele->NextSiblingElement();
    if (!sub_ele || strcmp(sub_ele->Value(), "alarmas_x_mensaje") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <alarmas_x_mensaje>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    c->alarmas2mensaje = atoi( sub_ele->GetText() == NULL ? "0" : sub_ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_escaner") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_escaner>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    c->tiempo_escaner = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_entre_consultas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_entre_consultas>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tiempo_descarga_lote") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <tiempo_descarga_lote>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"buffer_alarmas") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_HISTORICOS) + " -> <buffer_alarmas>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    c->alarmas_buf = atoi( ele->GetText() == NULL ? "0" : ele->GetText() );



    // Ahora Leer el archivo de configuracion de tags
    TiXmlDocument doc1(__ARCHIVO_CONF_TAGS);
    if ( !doc1.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_TAGS) + "\n " + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle1(&doc1);
    ele = hdle1.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <cantidad>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <nombre>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy(c->tags_shmem, ele->GetText());
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"tag") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " -> <tag>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
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
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        strcpy(nombre_tag,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <tag> -> <valor_x_defecto>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        strcpy(valor_char,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if (!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <tag> -> <tipo>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        //tipo_tag = *(sub_ele->GetText());
        TEXTO2DATO(tipo_tag,sub_ele->GetText());
        switch(tipo_tag) {
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
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            TEXTO2LOG(tl,ssub_ele->GetText());
            tmp.asignar_tipo(tl);
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "tiempo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <tiempo>\n "
                                + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_tiempo( atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "maximo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <maximo>\n "
                                + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_maximo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "minimo") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <minimo>\n "
                                + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            tmp.asignar_minimo( atof( ssub_ele->GetText() ) );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "muestras") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <muestras>\n "
                                + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            unsigned int mts = atoi( ssub_ele->GetText() == NULL ? "0" : ssub_ele->GetText() );
            tmp.asignar_muestras( mts > _MAX_CANT_MUESTRAS_ ? _MAX_CANT_MUESTRAS_ : mts );
            ssub_ele = ssub_ele->NextSiblingElement();
            if (!ssub_ele || strcmp(ssub_ele->Value(), "funciones") != 0) {
                mensaje_error = "Error " + string(__ARCHIVO_CONF_TAGS) + " <historicos> -> <funciones>\n "
                                + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error(mensaje_error.c_str());
            }
            strcpy(tmp_funcion, ssub_ele->GetText());
            valor_funcion = strtok (tmp_funcion,",");
            while (valor_funcion != NULL) {
                for (unsigned int j = 0; j < n_func_disponibles; j++) {
                    if ( strcmp(valor_funcion,_FUNCIONES_HISTORICOS_[j]) == 0 ) {
                        TEXTO2FUNC(tf,_FUNCIONES_HISTORICOS_[j]);
                        tmp.asignar_funcion(tf);
                    }
                }
                valor_funcion = strtok (NULL, ",");
            }
            c->pht.v_hist.push_back(tmp);
            tmp.reset_cant_funciones();
        }
    }


    // Ahora leer el archivo de configuracion de alarmas
    TiXmlDocument doc2(__ARCHIVO_CONF_ALARMAS);
    if( !doc2.LoadFile() ) {
        mensaje_error = "Error Cargando Archivo de Configuracion: " +
                        string(__ARCHIVO_CONF_ALARMAS) + "\n" + string(strerror(errno));
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle2(&doc2);
    ele = hdle2.FirstChildElement().FirstChildElement().Element();
    if(!ele || strcmp(ele->Value(), "cantidad") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <cantidad>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if(!ele || strcmp(ele->Value(),"nombre") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <nombre>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }
    strcpy(c->alarmas_shmem, ele->GetText());
    ele = ele->NextSiblingElement();
    if(!ele || strcmp(ele->Value(),"alarma") != 0) {
        mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " -> <alarma>\n "
                        + string( strerror( errno ) );
        escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error(mensaje_error.c_str());
    }

    TiXmlAttribute * attr;
    char	nombre[_LONG_NOMB_ALARM_];
    Ealarma estado;
    Tvalor valor_comparacion;

    alarma a;
    for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
        sub_ele = ele->FirstChildElement();
        if(!sub_ele || strcmp(sub_ele->Value(), "grupo") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <grupo>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(),"subgrupo") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <subgrupo>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <nombre>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        strcpy(nombre,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(),"estado_x_defecto") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <estado_x_defecto>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        TEXTO2ESTADO(estado,sub_ele->GetText());
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(),"valor_comparacion") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <valor_comparacion>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(),"tipo_comparacion") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <tipo_comparacion>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        sub_ele = sub_ele->NextSiblingElement();
        if(!sub_ele || strcmp(sub_ele->Value(),"historicos") != 0) {
            mensaje_error = "Error " + string(__ARCHIVO_CONF_ALARMAS) + " <alarma> -> <historicos>\n "
                            + string( strerror( errno ) );
            escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error(mensaje_error.c_str());
        }
        attr = sub_ele->FirstAttribute();
        if( !attr || strcmp(attr->Name(), "habilitado") != 0) {
            escribir_log( "as-registrador", LOG_ERR, "Error Obteniendo Atributos de <habilitado> -> tipo" );
            throw runtime_error("Error Obteniendo Atributos de <habilitado> -> tipo");
        }
        if( strcmp( attr->Value(), "1" ) == 0 ) {
            valor_comparacion.L = _ERR_DATO_;
            a.asignar_propiedades( 0, "", 0, "", nombre, estado,
                                   valor_comparacion, AERROR_, ENTERO_ );
            c->pha.v_alarm.push_back( a );
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
            {"verbose", no_argument, &verbose_flag, 1},
            {"brief",   no_argument, &verbose_flag, 0},
            {"rec-all",	no_argument,		0, 'r'},
            {"shm",		required_argument,	0, 's'},
            {"tags",		required_argument,	0, 't'},
            {"alarm",	required_argument,	0, 'a'},
            {0, 0, 0, 0}
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

    if (verbose_flag) {
        cout << "Modo Verbose" << endl;
        cout << "\tProyecto Argos (as-registrador) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @brief	Hace una Copia en Disco del Contenido del Buffer Circular
*			de Tendencias
* @param	trend		Referencia a la Memoria Comaprtida
* @param	cuenta		Cantidad de Datos a Copiar
* @return	verdadero	Volcado Exitoso
*/
bool shmem2disco( tendencia * trend, unsigned int cuenta) {
    ofstream salida("trend.csv");
    tendencia * tr;
    tags * t;
    struct timeval fecha;
    Tvalor * p;

    for (unsigned int i=0; i < cuenta; i++) {
			tr = conf.hist_shmem.obtener_tendencia( i );
        salida << "\"" << tr->obtener_nombre_tag() << "\";";
    }
    salida << "\"Fin\"\n";
    for (unsigned int i=0; i < _MAX_CANT_MUESTRAS_; i++) {
        for (unsigned int j=0; j < cuenta; j++) {
        	tr = conf.hist_shmem.obtener_tendencia( j );
        	t = tr->obtener_tag();
        	fecha = tr->obtener_fecha_punto(i);
        	p = tr->obtener_punto(i);
					switch(t->tipo_dato()) {
					case CARAC_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << static_cast<palabra_t> (p->C) << "; ";
							break;
					case CORTO_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << static_cast<short> (p->S) << "; ";
							break;
					case ENTERO_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << p->I << "; ";
							break;
					case LARGO_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << p->L << "; ";
							break;
					case REAL_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << p->F << "; ";
							break;
					case DREAL_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << p->D << "; ";
							break;
					case BIT_:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ " << p->B << "; ";
							break;
					case TERROR_:
					default:
							salida << fecha.tv_sec << "." << fecha.tv_usec << " @ DESCONOCIDO\t";
					}
        }
        salida << "0.0\n";
    }
    return true;
}

/**
* @brief	Almacena en el Buffer Circular los Valores de los Tags
* @param	q		Descriptor de la Cola de Mensajes donde enviar la senal
*/
void registrar_tags(mqd_t * q) {

    // Crear la memoria compartida para las tendencias
    string nombre_shm = "registrador/" + string(conf.hist_tags_shmem);
    conf.hist_shmem.inicializar_shm_tendencia( nombre_shm );
   	conf.hist_shmem.crear_shm_tendencia( conf.pht.v_hist.size() );


    /*
     * Se abre la SHMEM de Tags para hacer las Lecturas
    */
    nombre_shm = "escaner/" + string(conf.tags_shmem);
    conf.pht.shm.inicializar_shm_thtag(nombre_shm.c_str());
    conf.pht.shm.obtener_shm_tablahash();

    tags * aux;
    struct timeval ahora;
    tendencia * tr;
    /*
     * Se copia a la SHMEM de Historicos los Registros que estan el en Segmento de Datos del
     * Proceso, junto con el primer valor leido de la SHMEM de Tags
     */
    unsigned int i;
    for ( i=0; i < conf.pht.v_hist.size(); i++ ) {
        tr = conf.hist_shmem.obtener_tendencia( i );
        tr->agregar_historico( conf.pht.v_hist[i] );
        aux = tr->obtener_tag();
        conf.pht.shm.buscar_tag(aux);
        tr->insertar_nuevo_punto(aux, 0);
    }



    i = 0;
    unsigned int 		pos = 0;
    int					muestras=0;
    unsigned long int 	tiempo_transcurrido = 0;
    bool				enviar = false;

    while (true) {
        enviar = false;
        gettimeofday(&ahora, (struct timezone *)0);
        tr = conf.hist_shmem.obtener_tendencia( i );

        pos = tr->obtener_posicion() - 1;
        switch (tr->obtener_tipo_log()) {
        case MUESTREO_:
            tiempo_transcurrido = calc_tiempo_transcurrido(ahora, tr->obtener_fecha_punto(pos));
            if (tiempo_transcurrido > tr->obtener_tiempo_muestreo() ) {
                //LOGUEAR
#ifdef _DEBUG_
                cout << "Logueando " << tr->obtener_nombre_tag() << " Tiempo Transcurrido = " << tiempo_transcurrido << endl;
#endif
                if ( ++pos >= _MAX_CANT_MUESTRAS_ ) {
                    pos = 0;
                    shmem2disco( tr , conf.pht.v_hist.size());
                }
                //Obtener el valor del Tag de la SHMEM de Tags
                aux = tr->obtener_tag();
                conf.pht.shm.buscar_tag(aux);
                //Almacenar el nuevo punto en la SHMEM de Tendencias
                tr->insertar_nuevo_punto(aux,pos);
                enviar = true;
            }
            break;
        case EVENTO_:
            aux = tr->obtener_tag();
            conf.pht.shm.buscar_tag(aux);

            if( memcmp(&(aux->valor_campo), tr->obtener_punto(pos), sizeof(Tvalor)) != 0 ) {
#ifdef _DEBUG_
                cout << "Logueando " << tr->obtener_nombre_tag() << " Evento Disparado" << endl;
#endif
                if ( ++pos >= _MAX_CANT_MUESTRAS_ ) {
                    pos = 0;
                }
                tr->insertar_nuevo_punto(aux,pos);
                enviar = true;
            }
            break;
        default:
            mensaje_error = "Logueando " + string(tr->obtener_nombre_tag()) + " Tipo de Muestreo Desconocido";
#ifdef _DEBUG_
            cout << mensaje_error << endl;
#endif
            escribir_log( "as-registrador", LOG_WARNING, mensaje_error.c_str() );
            break;
        }

        //Cuando se alcanza la cantidad de muestras se envia mensaje al SSBD
        if ( enviar && ( pos % ( tr->obtener_muestras() ) == 0 ) ) {

            // Preparar el Mensaje que sera enviado a SSBD
            Tmsgssbd mensaje;
            mensaje.tipo = _TIPO_TAG_;
            strcpy(mensaje.nombre_tag, aux->nombre);
            mensaje.posicion = i;
            muestras = pos - tr->obtener_muestras();
            mensaje.desde = muestras < 0 ? muestras + _MAX_CANT_MUESTRAS_ :
                            muestras;
            mensaje.hasta = pos;

            // Enviar el mensaje a traves de una cola
            if( mq_send( *q, (const char *)&mensaje, conf.mensaje_long, 0) == -1 ) {
                mensaje_error = "[TAG] Error Enviando Mensaje a Cola: " + string( strerror( errno ) );
                escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                throw runtime_error( mensaje_error.c_str() );
            }
#ifdef _DEBUG_
            cout << "\tMensaje Enviado -> TAG: " << mensaje.nombre_tag << " , POSICION: " <<  mensaje.posicion
                 << " , INICIO: " << mensaje.desde << " , HASTA: " << mensaje.hasta << endl;
#endif
        }

        if ( ++i >= conf.pht.v_hist.size() ) {
            i = 0;
//            conf.hist_shmem.imprimir_shm_tendencia();
            usleep(conf.tiempo_escaner);
            continue;
        }
    }

    // Cerrar Descriptores
		conf.hist_shmem.cerrar_shm_tendencia();
    conf.pht.shm.cerrar_shm_tablahash();
}

/**
* @brief	Almacena en el Buffer Circular los Valores de las Alarmas
* @param	q		Descriptor de la Cola de Mensajes donde enviar la senal
*/
void registrar_alarmas(mqd_t * q) {

    // Crear la memoria compartida para el alarmero
    string nombre_shm = "registrador/" + string(conf.hist_alarmas_shmem);
    conf.hist_shmem.inicializar_shm_alarmero( nombre_shm );
    conf.hist_shmem.crear_shm_alarmero( conf.alarmas_buf );


    /*
     * Se abre la SHMEM de Alarmas para hacer las Lecturas
    */
    nombre_shm = "escaner/" + string(conf.alarmas_shmem);
    conf.pha.shm.inicializar_shm_thalarma(nombre_shm.c_str());
    conf.pha.shm.obtener_shm_tablahash();

    alarma 				*aux1,
    							*aux2;
    struct timeval 		ahora;
    alarmero 			*al;
    Ealarma				estado_anterior = DESACT_;
    int 					pos = 0,
    							indice = 0;
    unsigned int	pos_buffer = 0,
    							cuenta = 0;


    vector <alarma>::iterator	it = conf.pha.v_alarm.begin();

    while (true) {
        // Calcular el indice del arreglo
        if( it == conf.pha.v_alarm.end() ) {
            it = conf.pha.v_alarm.begin();
            pos = 0;
            usleep(conf.tiempo_escaner);
        }

        // Obtener la alarma para comparar
        aux1 = &conf.pha.v_alarm[pos];
        estado_anterior = aux1->estado;

        // Obtener el estado actual
        conf.pha.shm.buscar_alarma(aux1);

        if( estado_anterior != aux1->estado ) {
#ifdef _DEBUG_
            cout << cuenta << " Logueando " << aux1->nombre << " Cambio de Estado" << endl;
#endif
            // Almacenar en el buffer de alarmas
            al = conf.hist_shmem.obtener_alarmero( pos_buffer );
            al->insertar_nueva_alarma(aux1);
            cuenta++;

            // Actualizar el puntero al buffer
            if ( ++pos_buffer >= conf.alarmas_buf ) {
                pos_buffer = 0;
            }

            // El estado actual se convierte en el estado anterior del
            // proximo escaneo
            conf.pha.v_alarm[pos].estado = aux1->estado;
            al = conf.hist_shmem.obtener_alarmero( pos_buffer );
            aux2 = al->obtener_alarma();
            aux2->actualizar_estado(EERROR_);


            //Cuando se alcanza la cantidad en el buffer se envia mensaje al SSBD
            if ( cuenta >= conf.alarmas2mensaje ) {

                // Preparar el Mensaje que sera enviado a SSBD
                Tmsgssbd mensaje;
                mensaje.tipo = _TIPO_ALARMA_;
                strcpy(mensaje.nombre_alarma, aux1->nombre);
                mensaje.posicion = 0;
                indice = (pos_buffer - 1) < 0 ? conf.alarmas_buf : pos_buffer - 1;
                mensaje.desde = (indice - cuenta) < 0 ? (indice - cuenta + conf.alarmas_buf) :
                                (indice - cuenta);
                mensaje.hasta = indice;

                // Enviar el mensaje a traves de una cola
                if( mq_send( *q, (const char *)&mensaje, conf.mensaje_long, 0) == -1 ) {
                    mensaje_error = "[ALARMA] Error Enviando Mensaje a Cola: " + string( strerror( errno ) );
                    escribir_log( "as-registrador", LOG_ERR, mensaje_error.c_str() );
                    throw runtime_error( mensaje_error.c_str() );
                }
#ifdef _DEBUG_
                cout << "\tMensaje Enviado -> ALARMA: " << mensaje.nombre_alarma << " , POSICION: " <<  mensaje.posicion
                     << " , INICIO: " << mensaje.desde << " , HASTA: " << mensaje.hasta << endl;
#endif
                cuenta = 0;
            }


        }

        pos++;
        it++;
    }
    // Cerrar Descriptores
    conf.hist_shmem.cerrar_shm_alarmero();
    conf.pha.shm.cerrar_shm_tablahash();


}

