/**
* @file	as-escaner.cpp
* @brief		<b>Proceso encargado de Leer las SHMEM de los distintos <br>
*				dispositivos y mantener actualizadas las SHMEM <br>
*				principales de Tags y alarmas</b><br>
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

#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <ctime>
#include <cstdio>
#include <climits>

#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "as-util.h"
#include "as-registro.h"
#include "as-tags.h"
#include "as-thtag.h"
#include "as-shmtag.h"
#include "as-alarma.h"
#include "as-thalarma.h"
#include "as-shmalarma.h"
#include "tinyxml.h"


#include <stdexcept>


using namespace std;
using namespace argos;


static int	 		verbose_flag;			//!< Modo Verboso
string				mensaje_error;			//!< Mensaje de Error por Pantalla
string				nombre_tags;			//!< Memoria Compartida de Tags
string				nombre_alarmas;			//!< Memoria Compartida de Alarmas


/*--------------------------------------------------------
	Estructura de dato usada para encapsular el valor
	del tag, dependiendo del tipo.
----------------------------------------------------------*/
struct Reg_t{
	char		shm_dispositivo[100];		//!< Memorias Compartidas de Dispositivos
	registro	origen;						//!< Datos Registro Origen
	registro	*shm_origen;				//!< Registro Origen en SHM
	tags		destino;					//!< Tag de Destino
	short		bit;						//!< Si el Tag es un Bit del Registro
}*regis2tag;

struct Reg_a{
	char		shm_dispositivo[100];		//!< Memorias Compartidas de Dispositivos
	registro	origen_r;					//!< Datos Registro Origen
	tags		origen_t;					//!< Daros Tags Origen
	registro	*shm_origen;				//!< Registro Origen en SHM
	alarma		destino;					//!< Alarma de Destino
	short		bit;						//!< Si la Alarma es un Bit del Registro
	bool		desde_registro;				//!< Desde Registro o Tag
}*regis2alarma;

struct Shm_d{
	char		nombre[100];				//!< Nombre del Dispositivo
	registro	*dispositivo;				//!< Apuntador a los Registros del Dispos.
	int			n_registros;				//!< Cantidad de Registros en el Dispos.
}*shmem_dispositivos;



shm_thtag 			shm_tht;				//!< Tabla Hash de Tags en SHM
shm_thalarma		shm_tha;				//!< Tabla Hash de Alarmas en SHM


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool 		asignar_opciones( int , char ** );
bool 		leer_config_dispositivos( int & );
bool 		construir_shm_tags( shm_thtag * , int & );
bool 		construir_shm_alarmas( shm_thalarma * , int & );
bool 		asignar_disp2tags( int , int );
bool 		asignar_disp2alarmas( int , int );
void 		escanear_tags( int * );
void 		escanear_alarmas( int * );
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc, char * argv[]){
#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-escaner) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-escaner", LOG_INFO, "Iniciando Servicio..." );


	//-----------------Opciones de Configuracion
	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }

	int d_cantidad = 0;
	//-----------------Leer la ubicacion de las Memorias Compartidas
	//-----------------de cada dispositivo configurado
#ifdef _DEBUG_
	cout << "Leyendo Archivo de Configuracion de Dispositivos....." << endl;
#endif
	leer_config_dispositivos(d_cantidad);

	//------------------Apertura de Memorias Compartidas
#ifdef _DEBUG_
	cout << "Abriendo SHMEM de Dispositivos....." << endl;
#endif
	int segmento_memoria = 0, fd = -1;
    for(int i = 0; i < d_cantidad; i++){
		string nombre_shm = "dispositivos/" +
				string((*(shmem_dispositivos + i)).nombre);

 		fd = shm_open(nombre_shm.c_str(), O_RDONLY, S_IREAD);
		if ( -1 == fd ) {
			mensaje_error = "Error Abriendo SHMEM /dev/shm/" + nombre_shm +
					string( strerror( errno ) );
			escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		segmento_memoria = sizeof(registro) * (*(shmem_dispositivos + i)).n_registros;
	    (*(shmem_dispositivos + i)).dispositivo = ( registro * )mmap(0, segmento_memoria,
				PROT_READ, MAP_SHARED, fd, (long)0);

		if ( NULL == (*(shmem_dispositivos + i)).dispositivo ) {
			mensaje_error = "Error Mapeando SHMEM /dev/shm/" + nombre_shm +
					string( strerror( errno ) );
			escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
    }


	//-------------------Crear el Directorio para las SHMEM
#ifdef _DEBUG_
	cout << "Creando Directorio para Tags y Alarmas....." << endl;
#endif
	struct stat datos_directorio;
    lstat("/dev/shm/escaner", &datos_directorio);
    if ( !S_ISDIR( datos_directorio.st_mode ) ) {
        if (-1 == mkdir("/dev/shm/escaner",0777) ) {
            mensaje_error = "Error Creando Directorio /dev/shm/escaner: "
					+ string( strerror( errno ) );
			escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
            throw runtime_error( mensaje_error.c_str() );
        }
    }


	int t_cantidad = 0, a_cantidad = 0;

	//-----------------Construir la SHMEM de tags
#ifdef _DEBUG_
	cout << "Leyendo Archivo de Configuracion de Tags....." << endl;
#endif
	construir_shm_tags( &shm_tht, t_cantidad);

	//-----------------Asignar las SHMEM de los Dispositivos a cada Tag
#ifdef _DEBUG_
	cout << "Asignando Dispositivos a Tags....." << endl;
#endif
	asignar_disp2tags( d_cantidad, t_cantidad );

	//-----------------Construir la SHMEM de alarmas
#ifdef _DEBUG_
	cout << "Leyendo Archivo de Configuracion de Alarmas....." << endl;
#endif
	construir_shm_alarmas( &shm_tha, a_cantidad);

	//-----------------Asignar las SHMEM de los Dispositivos a cada Alarma
#ifdef _DEBUG_
	cout << "Asignando Dispositivos a Alarmas....." << endl;
#endif
	asignar_disp2alarmas( d_cantidad, a_cantidad );


	pthread_t hilo_tags;
	pthread_t hilo_alarmas;

	//----------------Arrancar el Hilo para Escanear los Tags
#ifdef _DEBUG_
	cout << "Arrancando Hilo para Tags....." << endl;
#endif
	if( pthread_create( &hilo_tags, NULL, (void *(*)(void *))escanear_tags,
			&t_cantidad ) ){
		mensaje_error = "Error Creando Hilo para Escanear Tags: "
				+ string( strerror( errno ) );
		escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}

	//----------------Arrancar el Hilo para Escanear las Alarmas
#ifdef _DEBUG_
	cout << "Arrancando Hilo para Alarmas....." << endl;
#endif
	if( pthread_create( &hilo_alarmas, NULL, (void *(*)(void *))escanear_alarmas,
			&a_cantidad ) ){
		mensaje_error = "Error Creando Hilo para Escanear Alarmas: "
				+ string( strerror( errno ) );
		escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
		throw runtime_error( mensaje_error.c_str() );
	}

	pthread_join( hilo_tags, NULL );
	pthread_join( hilo_alarmas, NULL );


	shm_tht.cerrar_shm_tablahash();


	free(shmem_dispositivos);
	free(regis2tag);
	free(regis2alarma);



}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	bool	Falso si existen opciones no validas
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
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rs:d:t:a:",long_options, &option_index);
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
        case '?':
            break;
        default:
            return false;
        }
    }

    if (verbose_flag){
        cout << "Modo Verbose" << endl;
		cout << "\tProyecto Argos (as-escaner) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* @return	verdadero	Operacion Satisfactoria
*/
bool leer_config_dispositivos( int & cantidad ){
	TiXmlDocument doc(__ARCHIVO_CONF_DEVS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_DEVS) + "\n" + string(strerror(errno));
        escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
        escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad1 = atoi( ele->GetText() );
    cantidad = cantidad1;
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
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
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <dispositivo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <dispositivo>");
		}
		attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
		}

		strcpy(id, attr->Value());
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
		}

		strcpy(modelo, attr->Value());

		sub_ele = ele->FirstChildElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
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
				escribir_log( "as-escaner", LOG_ERR, "Error Contando numero de Registros <lectura>" );
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
					//Si el Manejador de Comunicacion lee bloques de registros
					//se debe sumar al total la cantidad de registros por bloque,
					//Como sucede con el protocolo Modbus
					attr = sub_sub_ele->FirstAttribute();
					while( attr && strcmp(attr->Name(),"cantidad") != 0 ){
						attr = attr->Next();
					}
					if( attr && strcmp(attr->Name(),"cantidad") == 0 ){
						reg_x_disp[ind] += atoi(attr->Value());
					}
					else{
						reg_x_disp[ind]++;
					}
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

	if( ( shmem_dispositivos = (struct Shm_d *)malloc(sizeof(struct Shm_d) * cantidad) ) == NULL ){
		escribir_log( "as-escaner", LOG_ERR, "Error Reservando Memoria para Dispositivos" );
		throw runtime_error("Error Reservando Memoria para Dispositivos");
	}

	for( int i = 0; i < cantidad; i++ ){
#ifdef _DEBUG_
		cout <<"Nombre SHMEM: "<< marca_id_nombre[i] << "\t\tRegistros: [" <<
			reg_x_disp[i] << "]" << endl;
#endif
		strcpy( (*(shmem_dispositivos + i)).nombre, marca_id_nombre[i].c_str() );
		(*(shmem_dispositivos + i)).n_registros = reg_x_disp[i];
	}

    return true;
}

/**
* @brief	Obtiene los Argumentos y Par&aacute;metros de Configuraci&oacute;n
*			de los tags que se desean muestrear de un archivo XML
* @param	shm			Tabla Hash de Tags
* @param	cantidad	Cantidad de Tags
* @return	verdadero	Creacion Exitosa de SHM
*/
bool construir_shm_tags( shm_thtag * shm, int & cantidad ){
	TiXmlDocument doc(__ARCHIVO_CONF_TAGS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_TAGS) + "\n" + string(strerror(errno));
        escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    nombre_tags = "escaner/" + string(ele->GetText());

    //---------------Inicializar los valores para la SHMEM
#ifdef _DEBUG_
	cout << "Iniciando SHMEM Tags....." << endl;
#endif
	shm_tht.inicializar_shm_thtag(nombre_tags);

	if( ( regis2tag = (struct Reg_t *)malloc(( sizeof(struct Reg_t) )* cantidad) ) == NULL ){
		escribir_log( "as-escaner", LOG_ERR, "Error Reservando Memoria para los Tags" );
		throw runtime_error("Error Reservando Memoria para los Tags");
	}

	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"tag") != 0){
		escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tag>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
	}

	TiXmlElement *sub_ele, *sub_sub_ele;
	TiXmlAttribute * attr;
	char nombre_tag[100], valor_char[100], tmp1[100], tmp2[100];
	Tvalor valor_tag;
	Tdato tipo_tag;
	shm->crear_shm_tablahash(cantidad);
	tags t;

	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_x_defecto>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_x_defecto>");
		}
		strcpy(valor_char,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
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
		t.asignar_propiedades(nombre_tag,valor_tag,tipo_tag);
		shm->insertar_tag(t);
		(*(regis2tag + i)).destino.asignar_propiedades( nombre_tag,valor_tag,
				tipo_tag );

		sub_ele = sub_ele->NextSiblingElement();
		if(sub_ele && ( strcmp(sub_ele->Value(),"dispositivo") != 0 ) ){
			sub_ele = sub_ele->NextSiblingElement();
		}
		if(sub_ele && ( strcmp(sub_ele->Value(),"dispositivo") == 0 ) ){
			attr = sub_ele->FirstAttribute();
			if( !attr || strcmp(attr->Name(), "id") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
				throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
			}
			strcpy(tmp1, attr->Value());
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "modelo") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
				throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
			}
			strcpy(tmp2, attr->Value());
			sub_sub_ele = sub_ele->FirstChildElement();
			if(!sub_sub_ele || strcmp(sub_sub_ele->Value(),"nombre") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> nombre" );
				throw runtime_error("Error Parseando Archivo de Configuracion <dispositivo> -> nombre");
			}
			sprintf( (*(regis2tag + i)).shm_dispositivo, "%s_%s_%s", tmp2, tmp1, sub_sub_ele->GetText() );
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if(!sub_sub_ele || strcmp(sub_sub_ele->Value(),"registro") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> registro" );
				throw runtime_error("Error Parseando Archivo de Configuracion <dispositivo> -> registro");
			}
			attr = sub_sub_ele->FirstAttribute();
			if( !attr || strcmp(attr->Name(), "direccion") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <registro> -> direccion" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> direccion");
			}
			strcpy( (*(regis2tag + i)).origen.direccion, attr->Value());
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "tipo") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tipo" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> tipo");
			}
			TEXTO2DATO( (*(regis2tag + i)).origen.tipo, attr->Value());
			(*(regis2tag + i)).origen.valor.L = LONG_MIN;
			attr = attr->Next();
			if( !attr || strcmp(attr->Name(), "bit") != 0){
				escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <registro> -> bit" );
				throw runtime_error("Error Obteniendo Atributos de <registro> -> bit");
			}
			(*(regis2tag + i)).bit = ( strcmp(attr->Value(),"") != 0 ) ? atoi( attr->Value() ) : -1;
		}
	}
    return true;
}

/**
* @brief	Obtiene los Argumentos y Parametros de Configuracion de
*			las Alarmas que se desean muestrear de un archivo XML
* @param	shm			Tabla Hash de Alarmas
* @param	cantidad	Cantidad de Alarmas
* @return	verdadero	Creacion Exitosa de SHM
*/
bool construir_shm_alarmas(shm_thalarma * shm, int & cantidad ){
	TiXmlDocument doc(__ARCHIVO_CONF_ALARMAS);
	if( !doc.LoadFile() ){
		mensaje_error = "Error Cargando Archivo de Configuracion: " +
			string(__ARCHIVO_CONF_ALARMAS) + "\n" + string(strerror(errno));
		escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
	}
	TiXmlHandle hdle(&doc);
	TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
	}
	cantidad = atoi( ele->GetText() );

	if( ( regis2alarma = (struct Reg_a *)malloc(( sizeof(struct Reg_a) )* cantidad) ) == NULL ){
		escribir_log( "as-escaner", LOG_ERR, "Error Reservando Memoria para las Alarmas" );
		throw runtime_error("Error Reservando Memoria para las Alarmas");
	}

	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	nombre_alarmas = "escaner/" + string(ele->GetText());

	 //---------------Inicializar los valores para la SHMEM
#ifdef _DEBUG_
	cout << "Iniciando SHMEM Alarmas....." << endl;
#endif
	shm_tha.inicializar_shm_thalarma(nombre_alarmas);

	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"alarma") != 0){
		escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <alarma>" );
		throw runtime_error("Error Parseando Archivo de Configuracion -> <alarma>");
	}

	TiXmlElement * sub_ele, *sub2_ele;
	TiXmlAttribute * attr;
	char	grupo[_LONG_NOMB_GALARM_],
			subgrupo[_LONG_NOMB_SGALARM_],
			nombre[_LONG_NOMB_ALARM_],
			nombre_reg[_LONG_NOMB_REG_],
			valor_tmp[100],
			ubi_id[10],
			ubi_modelo[20],
			ubi_nombre[100],
			dat_bit[4];
	int		idg, idsg;
	Ealarma estado;
	Tdato tipo_origen;
	Tvalor valor_comparacion, long_minimo;
	Tcomp tipo_comparacion;

	shm->crear_shm_tablahash(cantidad);
	alarma a;
	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "grupo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
		}
		attr = sub_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <grupo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <grupo> -> id");
		}
		idg = atoi( attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "descripcion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <grupo> -> descripcion" );
			throw runtime_error("Error Obteniendo Atributos de <grupo> -> descripcion");
		}
		strcpy( grupo, attr->Value() );
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"subgrupo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <subgrupo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <subgrupo>");
		}
		attr = sub_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <subgrupo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <subgrupo> -> id");
		}
		idsg = atoi( attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "descripcion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <subgrupo> -> descripcion" );
			throw runtime_error("Error Obteniendo Atributos de <subgrupo> -> descripcion");
		}
		strcpy( subgrupo, attr->Value() );
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"estado_x_defecto") != 0){
			mensaje_error = "Error Parseando Archivo de Configuracion -> <estado_x_defecto> ";
			escribir_log( "as-escaner", LOG_ERR, mensaje_error.c_str() );
			throw runtime_error( mensaje_error.c_str() );
		}
		TEXTO2ESTADO(estado,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_comparacion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <valor_comparacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_comparacion>");
		}
		if(sub_ele->GetText())
			strcpy( valor_tmp, sub_ele->GetText() );
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo_comparacion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo_comparacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo_comparacion>");
		}
		if(sub_ele->GetText()){
			TEXTO2COMP(tipo_comparacion,sub_ele->GetText());
		}
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"historicos") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <historicos>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <historicos>");
		}

		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"origen") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <origen>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <origen>");
		}
		attr = sub_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "tipo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen> -> tipo" );
			throw runtime_error("Error Obteniendo Atributos de <origen> -> tipo");
		}
		if( strcmp( attr->Value(), "registro" ) == 0 )
			(*(regis2alarma + i)).desde_registro = true;
		else if( strcmp( attr->Value(), "tag" ) == 0 )
			(*(regis2alarma + i)).desde_registro = false;
		else{
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen> -> tipo (Valor Desconocido)" );
			throw runtime_error("Error Obteniendo Atributos de <origen> -> tipo\nValor Desconocido");
		}

		sub2_ele = sub_ele->FirstChildElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"ubicacion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <origen><ubicacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <origen><ubicacion>");
		}
		attr = sub2_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><ubicacion> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <origen><ubicacion> -> id");
		}
		strcpy( ubi_id, attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><ubicacion> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <origen><ubicacion> -> modelo");
		}
		strcpy( ubi_modelo, attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "nombre") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><ubicacion> -> nombre" );
			throw runtime_error("Error Obteniendo Atributos de <origen><ubicacion> -> nombre");
		}
		strcpy( ubi_nombre, attr->Value() );
		sub2_ele = sub2_ele->NextSiblingElement();
		if(!sub2_ele || strcmp(sub2_ele->Value(),"dato") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Parseando Archivo de Configuracion -> <origen><dato>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <origen><dato>");
		}
		attr = sub2_ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "direccion") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><dato> -> direccion" );
			throw runtime_error("Error Obteniendo Atributos de <origen><dato> -> direccion");
		}
		strcpy( nombre_reg, attr->Value() );
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "tipo") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><dato> -> tipo" );
			throw runtime_error("Error Obteniendo Atributos de <origen><dato> -> tipo");
		}
		TEXTO2DATO(tipo_origen, attr->Value());
		switch(tipo_origen){
			case CARAC_:
				valor_comparacion.C = static_cast<char> (atoi(valor_tmp));
        	break;
			case CORTO_:
				valor_comparacion.S = static_cast<short> (atoi(valor_tmp));
        	break;
			case ENTERO_:
				valor_comparacion.I = atoi(valor_tmp);
        	break;
			case LARGO_:
				valor_comparacion.L = atoll(valor_tmp);
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
				valor_comparacion.L = _ERR_DATO_;
		}
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "bit") != 0){
			escribir_log( "as-escaner", LOG_ERR, "Error Obteniendo Atributos de <origen><dato> -> bit" );
			throw runtime_error("Error Obteniendo Atributos de <origen><dato> -> bit");
		}
		strcpy( dat_bit, attr->Value() );
		if( (*(regis2alarma + i)).desde_registro ){
			sprintf( (*(regis2alarma + i)).shm_dispositivo, "%s_%s_%s",
				ubi_modelo, ubi_id, ubi_nombre );
			strcpy( (*(regis2alarma + i)).origen_r.direccion, nombre_reg );
			(*(regis2alarma + i)).origen_r.tipo = tipo_origen;
			(*(regis2alarma + i)).origen_r.valor.L = LONG_MIN;
		}
		else{
			strcpy( (*(regis2alarma + i)).shm_dispositivo, ubi_nombre );
			long_minimo.L = LONG_MIN;
			(*(regis2alarma + i)).origen_t.asignar_propiedades( nombre_reg,
					long_minimo ,tipo_origen );
		}
		(*(regis2alarma + i)).bit = ( strcmp(dat_bit,"") != 0 ) ? atoi( dat_bit ) : -1;

		a.asignar_propiedades( idg, grupo, idsg, subgrupo, nombre, estado,
				valor_comparacion, tipo_comparacion, tipo_origen );
		shm->insertar_alarma(a);
		(*(regis2alarma + i)).destino.asignar_propiedades( idg, grupo, idsg,
				subgrupo, nombre, estado, valor_comparacion, tipo_comparacion,
				tipo_origen );
	}
	return true;
}

/**
* @brief	Asigna las Memorias Compartidas de los Dispositivos
*			a los tags que se desean muestrear
* @param	d_cantidad	Cantidad de Dispositivos
* @param	t_cantidad	Cantidad de Tags
* @return	verdadero	Operacion Exitosa
*/
bool asignar_disp2tags( int d_cantidad, int t_cantidad ){
	register int inicio = 0, final = 0, medio = 0;
	registro * r_tmp;
	int comparacion = 0;


	for( int j = 0; j < t_cantidad; j++){
		(*(regis2tag + j)).shm_origen = NULL;   //Eliminar cualquier direccion invalida
		for(int i = 0; i < d_cantidad; i++){
			if( strcmp( (*(shmem_dispositivos + i)).nombre,
					(*(regis2tag + j)).shm_dispositivo ) == 0 ){
				inicio = 0;
				final = (*(shmem_dispositivos + i)).n_registros - 1;
				while (inicio <= final) {
					medio = (inicio + final) / 2 ;
					r_tmp = ( (*(shmem_dispositivos + i)).dispositivo + medio );
					comparacion = strcmp( (*(regis2tag + j)).origen.direccion,
							r_tmp->direccion );
					if ( comparacion == 0 ){
						(*(regis2tag + j)).shm_origen = r_tmp;
						i = d_cantidad + 1; // Para salir del FOR y pasar al siguiente tag
						break;
					}
					else if (comparacion > 0)
						inicio = medio + 1;
					else
						final = medio - 1;
				}

			}
		}
	}
	return true;
}

/**
* @brief	Asigna las Memorias Compartidas de los Dispositivos
*			a las alarmas que se desean muestrear
* @param	d_cantidad	Cantidad de Dispositivos
* @param	a_cantidad	Cantidad de Alarmas
* @return	verdadero	Operacion Exitosa
*/
bool asignar_disp2alarmas( int d_cantidad, int a_cantidad ){
	register int inicio = 0, final = 0, medio = 0;
	registro * r_tmp;
	int comparacion = 0;


	for( int j = 0; j < a_cantidad; j++){
		(*(regis2alarma + j)).shm_origen = NULL;   //Eliminar cualquier direccion invalida

		//---Si la alarma proviene de un tag
		if( !(*(regis2alarma + j)).desde_registro ){
			continue;
		}
		for(int i = 0; i < d_cantidad; i++){
			if( strcmp( (*(shmem_dispositivos + i)).nombre,
					(*(regis2alarma + j)).shm_dispositivo ) == 0 ){
				inicio = 0;
				final = (*(shmem_dispositivos + i)).n_registros - 1;
				while (inicio <= final) {
					medio = (inicio + final) / 2 ;
					r_tmp = ( (*(shmem_dispositivos + i)).dispositivo + medio );
					comparacion = strcmp( (*(regis2alarma + j)).origen_r.direccion,
							r_tmp->direccion );
					if ( comparacion == 0 ){
						(*(regis2alarma + j)).shm_origen = r_tmp;
						i = d_cantidad + 1; // Para salir del FOR y pasar al siguiente tag
						break;
					}
					else if (comparacion > 0)
						inicio = medio + 1;
					else
						final = medio - 1;
				}

			}
		}
	}
	return true;
}

/**
* @brief	Hilo Encargado de hacer la Adquisicion de Tags de los
*			Dispositivos
* @param	cantidad	Cantidad de Registros
*/
void escanear_tags( int * cantidad ){
	register int 	indice = 0;
	register int 	r_bit = 0;
	registro 		*r_tmp;
	struct Reg_t 		*r_actual;
	int 			comparacion = 0;

	//-----------------Leer Registros desde los Dispositivos y Actualizar
	//-----------------los tags
	while(true){

		r_actual = (regis2tag + indice);
		r_tmp = r_actual->shm_origen;

		if( r_tmp ){
			comparacion = strcmp( r_actual->origen.direccion,
					r_tmp->direccion );
			if( comparacion == 0 ){
				switch(r_actual->destino.tipo){
					case CARAC_:
						r_actual->destino.valor_campo.C = r_tmp->valor.C;
					break;
					case CORTO_:
						r_actual->destino.valor_campo.S = r_tmp->valor.S;
					break;
					case ENTERO_:
						r_actual->destino.valor_campo.I = r_tmp->valor.I;
					break;
					case LARGO_:
						r_actual->destino.valor_campo.L = r_tmp->valor.L;
					break;
					case REAL_:
						r_actual->destino.valor_campo.F = r_tmp->valor.F;
					break;
					case DREAL_:
						r_actual->destino.valor_campo.D = r_tmp->valor.D;
					break;
					case BIT_:
						if(r_tmp->tipo == BIT_){
							r_actual->destino.valor_campo.B = r_tmp->valor.B;
						}
						else{
							r_bit = r_actual->bit;
							r_actual->destino.valor_campo.B =
							( r_tmp->valor.L &
							( ( r_bit >= 0 && r_bit < 32 ) ? __BIT2HEX[r_bit] : 0 ) );
						}
					break;
					case TERROR_:
					default:
						r_actual->destino.valor_campo.L = _ERR_DATO_;
				}
				shm_tht.actualizar_tag( &(r_actual->destino) );
			}
			else{
#ifdef _DEBUG
				cout << "Error localizando registro origen" << endl;
#endif
			}
		}
		else{
#ifdef _DEBUG
				cout << "Error localizando registro origen" << endl;
#endif
		}
		if( indice < ( (*cantidad) - 1 ) )
			indice++;
		else{
			indice = 0;
			usleep(500);
		}
	}
}

/**
* @brief	Hilo Encargado de hacer la Adquisicion de Alarmas de los
*			Dispositivos y de los Tags
* @param	cantidad	Cantidad de Registros
*/
void escanear_alarmas( int * cantidad ){
	register int indice = 0;
	registro 	* r_tmp;
	struct Reg_a	* r_actual;
	tags		* t_tmp;
	int 		comparacion = 0;

	//-----------------Leer Registros desde los Dispositivos y Actualizar
	//-----------------las Alarmas
	while(true){

		r_actual = (regis2alarma + indice);

		//----Si es una alarma proveniente de un registro
		if( r_actual->desde_registro ){

			r_tmp = r_actual->shm_origen;

			if( r_tmp ){
				comparacion = strcmp( r_actual->origen_r.direccion,
						r_tmp->direccion );
				if( comparacion == 0 ){

					if( r_actual->bit == -1 && r_actual->destino.tipo_origen == BIT_ ){
						r_actual->destino.estado = r_tmp->valor.B ? ACTIVA_ : DESACT_;
					}
					else if( r_actual->bit == -1 && r_actual->destino.tipo_origen != BIT_ ){
						switch(r_actual->destino.tipo_origen){
						    case CARAC_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.C == r_tmp->valor.C ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.C > r_actual->destino.valor_comparacion.C ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.C < r_actual->destino.valor_comparacion.C ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.C >= r_actual->destino.valor_comparacion.C ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.C <= r_actual->destino.valor_comparacion.C ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
						    case CORTO_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.S == r_tmp->valor.S ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.S > r_actual->destino.valor_comparacion.S ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.S < r_actual->destino.valor_comparacion.S ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.S >= r_actual->destino.valor_comparacion.S ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.S <= r_actual->destino.valor_comparacion.S ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
						    case ENTERO_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.I == r_tmp->valor.I ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.I > r_actual->destino.valor_comparacion.I ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.I < r_actual->destino.valor_comparacion.I ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.I >= r_actual->destino.valor_comparacion.I ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.I <= r_actual->destino.valor_comparacion.I ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
							case LARGO_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.L == r_tmp->valor.L ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.L > r_actual->destino.valor_comparacion.L ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.L < r_actual->destino.valor_comparacion.L ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.L >= r_actual->destino.valor_comparacion.L ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.L <= r_actual->destino.valor_comparacion.L ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
							case REAL_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.F == r_tmp->valor.F ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.F > r_actual->destino.valor_comparacion.F ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.F < r_actual->destino.valor_comparacion.F ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.F >= r_actual->destino.valor_comparacion.F ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.F <= r_actual->destino.valor_comparacion.F ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
							case DREAL_:
								switch(r_actual->destino.tipo_comparacion){
									case IGUAL_QUE_:
										if( r_actual->destino.valor_comparacion.D == r_tmp->valor.D ){
											r_actual->destino.estado = ACTIVA_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_QUE_:
										if( r_tmp->valor.D > r_actual->destino.valor_comparacion.D ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_QUE_:
										if( r_tmp->valor.D < r_actual->destino.valor_comparacion.D ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MAYOR_IGUAL_QUE_:
										if( r_tmp->valor.D >= r_actual->destino.valor_comparacion.D ){
											r_actual->destino.estado = ALTO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case MENOR_IGUAL_QUE_:
										if( r_tmp->valor.D <= r_actual->destino.valor_comparacion.D ){
											r_actual->destino.estado = BAJO_;
										}
										else{
											r_actual->destino.estado = DESACT_;
										}
									break;
									case AERROR_:
									default:
										r_actual->destino.estado = EERROR_;
								}
							break;
							case TERROR_:
							default:
								r_actual->destino.estado = EERROR_;
						}
					}
					else if(r_actual->bit >= 0 && r_actual->bit < 32){
						r_actual->destino.estado =
							( r_tmp->valor.L & __BIT2HEX[r_actual->bit] ) ?
								ACTIVA_ : DESACT_;
					}
					else{
						r_actual->destino.estado = EERROR_;
					}

					shm_tha.actualizar_alarma( &(r_actual->destino) );
				}
				else{
#ifdef _DEBUG
					cout << "Error localizando registro origen" << endl;
#endif
				}
			}
			else{
#ifdef _DEBUG
					cout << "Error localizando registro origen" << endl;
#endif
			}
		}//----Si es una alarma proveniente de un tag
		else{
			if( shm_tht.buscar_tag( &(r_actual->origen_t) ) ){
				t_tmp = &(r_actual->origen_t);
				if( r_actual->bit == -1 && r_actual->destino.tipo_origen == BIT_ ){
					r_actual->destino.estado = t_tmp->valor_campo.B ? ACTIVA_ : DESACT_;
				}
				else if( r_actual->bit == -1 && r_actual->destino.tipo_origen != BIT_ ){
					switch(r_actual->destino.tipo_origen){
						case CARAC_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.C == t_tmp->valor_campo.C ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.C > r_actual->destino.valor_comparacion.C ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.C < r_actual->destino.valor_comparacion.C ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.C >= r_actual->destino.valor_comparacion.C ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.C <= r_actual->destino.valor_comparacion.C ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case CORTO_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.S == t_tmp->valor_campo.S ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.S > r_actual->destino.valor_comparacion.S ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.S < r_actual->destino.valor_comparacion.S ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.S >= r_actual->destino.valor_comparacion.S ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.S <= r_actual->destino.valor_comparacion.S ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case ENTERO_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.I == t_tmp->valor_campo.I ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.I > r_actual->destino.valor_comparacion.I ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.I < r_actual->destino.valor_comparacion.I ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.I >= r_actual->destino.valor_comparacion.I ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.I <= r_actual->destino.valor_comparacion.I ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case LARGO_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.L == t_tmp->valor_campo.L ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.L > r_actual->destino.valor_comparacion.L ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.L < r_actual->destino.valor_comparacion.L ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.L >= r_actual->destino.valor_comparacion.L ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.L <= r_actual->destino.valor_comparacion.L ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case REAL_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.F == t_tmp->valor_campo.F ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.F > r_actual->destino.valor_comparacion.F ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.F < r_actual->destino.valor_comparacion.F ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.F >= r_actual->destino.valor_comparacion.F ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.F <= r_actual->destino.valor_comparacion.F ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case DREAL_:
							switch(r_actual->destino.tipo_comparacion){
								case IGUAL_QUE_:
									if( r_actual->destino.valor_comparacion.D == t_tmp->valor_campo.D ){
										r_actual->destino.estado = ACTIVA_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_QUE_:
									if( t_tmp->valor_campo.D > r_actual->destino.valor_comparacion.D ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_QUE_:
									if( t_tmp->valor_campo.D < r_actual->destino.valor_comparacion.D ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MAYOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.D >= r_actual->destino.valor_comparacion.D ){
										r_actual->destino.estado = ALTO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case MENOR_IGUAL_QUE_:
									if( t_tmp->valor_campo.D <= r_actual->destino.valor_comparacion.D ){
										r_actual->destino.estado = BAJO_;
									}
									else{
										r_actual->destino.estado = DESACT_;
									}
								break;
								case AERROR_:
								default:
									r_actual->destino.estado = EERROR_;
							}
						break;
						case TERROR_:
						default:
							r_actual->destino.estado = EERROR_;
					}
				}
				else if(r_actual->bit >= 0 && r_actual->bit < 32){
					r_actual->destino.estado =
						( t_tmp->valor_campo.L & __BIT2HEX[r_actual->bit] ) ?
							ACTIVA_ : DESACT_;
				}
				else{
					r_actual->destino.estado = EERROR_;
				}

				shm_tha.actualizar_alarma( &(r_actual->destino) );
			}
			else{
#ifdef _DEBUG
					cout << "Error localizando tag origen" << endl;
#endif
			}
		}
		if( indice < ( (*cantidad) - 1 ) )
			indice++;
		else{
			indice = 0;
			usleep(500);
		}
	}
}
