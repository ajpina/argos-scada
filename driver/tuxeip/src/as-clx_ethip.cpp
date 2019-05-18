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

#include "tinyxml.h"
#include "as-clx_ethip.h"

#include <signal.h>
#include <getopt.h>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <climits>

using namespace std;
using namespace argos;

static int					verbose_flag;			//!< Modo Verboso

ethipConf::configuracion	conf;			//!< Objeto de Configuracion de ssCLX_ETHIP


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool		asignar_opciones(int, char **);
bool		construir_shm_registros();
int			path_from_char2byte(int *);
bool		registrar_manejador_senales(sigset_t *);
void		manejador_senales( int );
void		escribir_a_dispositivo();
int			intento_escritura(registro,Eip_Session *,Eip_Connection *);
void		timeout_ack( int );
int			establecer_comunicacion( BYTE *, int );
int			finalizar_comunicacion();
/*--------------------------------------------------------
----------------------------------------------------------*/


/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(int argc,char *argv[]){

#ifdef _DEBUG_
    cout << "\tProyecto Argos (as-clx_ethip) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-clx_ethip", LOG_INFO, "Iniciando Servicio..." );

	//-----------------Opciones de Configuracion
	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }


	construir_shm_registros();      //Cargar las opciones desde el archivo de configuracion
	conf.ordenar_registros();       //Ordenar los Registros para crear la SHMEM
	conf.crear_memoria_compartida();    //Creacion de la Memoria Compartida
	conf.crear_cola();              //Creacion de la Cola para Escritura

	//Registro del Manejador de Senales
	sigset_t 		set,
					pendset;
	registrar_manejador_senales(&set);
	pthread_t		hilo_escritura;

	//Arrancar el hilo para la Escritura en el Dispositivo
	if( pthread_create( &hilo_escritura, NULL, (void *(*)(void *))escribir_a_dispositivo,
			NULL ) ){
		mensaje_error = "Error Creando Hilo para Escribir en Dispositivo: "
				+ string( strerror( errno ) );
		escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
	}
	pthread_detach( hilo_escritura );

//cip_debuglevel=LogDebug; // You can uncomment this line to see the data exchange between TuxEip and your Logic controller

	int 		ipath[50],
				spath = path_from_char2byte(ipath);			//Obtener la Ruta de Acceso al Dispositivo
	BYTE  		*path;

	if(spath > 0)
		path = new BYTE[spath];
	if(path){
		for(int i = 0; i < spath; i++)
			path[i] = ipath[i];
	}

    int com = establecer_comunicacion( path, spath );
    if( com == -1 ) throw runtime_error( mensaje_error.c_str() );
    while(true){
        unsigned int indice = 0;
        while( indice < conf.vregistros.size() ){
#ifdef _DEBUG_
            cout << "Adquiriendo " << conf.vregistros[indice].direccion << endl;
#endif
            conf.data = ReadLgxData( conf.session, conf.connection,
                    conf.vregistros[indice].direccion, 1 );
            if(conf.data != NULL){
                if(conf.data->Varcount > 0){
#ifdef _DEBUG_
                    cout << "Lectura Exitosa Ok :\n";
                    cout << "Elementos :" << conf.data->Varcount <<
                            "\tTipo : " << conf.data->type <<
                            "\tTamano Total: " << conf.data->totalsize <<
                            "\tTamano Elemento : " << conf.data->elementsize <<
                            "\t" << conf.data->mask << "\n";
                    cout << conf.vregistros[indice].direccion << " ";
#endif
                    for ( int i=0; i < conf.data->Varcount; i++ ){
                        switch(conf.data->type){
                            case DT_BOOL:
                                (*(conf.bloque + indice)).valor.B = (bool)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = BOOLEANO(" <<
                                        (*(conf.bloque + indice)).valor.B << ")\n";
#endif
                            break;
                            case DT_BITARRAY:
                                (*(conf.bloque + indice)).valor.D = (double)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = BOOLEANO(" <<
                                        (*(conf.bloque + indice)).valor.B << ")\n";
#endif
                            break;
                            case DT_SINT:
                                (*(conf.bloque + indice)).valor.S = (short)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = SHORT(" <<
                                        (*(conf.bloque + indice)).valor.S << ")\n";
#endif
                            break;
                            case DT_INT:
                                (*(conf.bloque + indice)).valor.I = (int)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = ENTERO(" <<
                                        (*(conf.bloque + indice)).valor.I << ")\n";
#endif
                            break;
                            case DT_DINT:
                                (*(conf.bloque + indice)).valor.L = (long)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = LONG(" <<
                                        (*(conf.bloque + indice)).valor.L << ")\n";
#endif
                            break;
                            case DT_REAL:
                                (*(conf.bloque + indice)).valor.F = (float)_GetLGXValueAsFloat(conf.data,i);
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = REAL(" <<
                                        (*(conf.bloque + indice)).valor.F << ")\n";
#endif
                            break;
                            default:
                                (*(conf.bloque + indice)).valor.L =  _ERR_DATO_;
#ifdef _DEBUG_
                                cout << "Valor [" << i << "] = DESCONOCIDO(" <<
                                        (*(conf.bloque + indice)).valor.L << ")\n";
#endif
                        }
                    }
                }
#ifdef _DEBUG_
                else {
                    cout << "Registro no Encontrado : " << cip_err_msg << endl;
                }
#endif
                free(conf.data);
            }
			else{
#ifdef _DEBUG_
                cout << "Error : Leyendo " << cip_err_msg << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
#endif
                if( cip_errno == E_RcvTimeOut ){
                    com = finalizar_comunicacion();
                    usleep( 5000000 );
                    com = establecer_comunicacion( path, spath );
                }
            }
			indice++;
		}
        sigpending( &pendset );
        if( sigismember(&pendset, SIGHUP ) ||
            sigismember(&pendset, SIGQUIT ) ||
            sigismember(&pendset, SIGTERM ) ||
            sigismember(&pendset, SIGUSR1 ) ||
            sigismember(&pendset, SIGUSR2 ) ){
            sigprocmask( SIG_UNBLOCK, &set, NULL );
        }
        usleep(conf.tiempo_muestreo * 1000);
    }
    finalizar_comunicacion();
	delete [] path;
	close(conf.fd);
	return(0);
}

/**
* @brief	Realiza la conexion con el dispositivo y registra
*			la sesion a traves CIP
* @param	path	Ruta del Modulo de Comunicacion dentro del Backplane
* @param	spath	Tamano del path
* @return	int		Establecimiento de la Comunicacion
*/
int establecer_comunicacion(BYTE * path, int spath){
#ifdef _DEBUG_
	cout << "Arrancando, Pid = " << getpid() << endl;
	cout << "Abriendo Sesion \n";
#endif
	conf.session = OpenSession(conf.direccion);
	if (conf.session == NULL)
	{
		ostringstream oss;
		oss << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
		mensaje_error = "Error : Abriendo Sesion " + string(cip_err_msg) + oss.str();
		escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
		return -1;

	}
#ifdef _DEBUG_
	cout << "Sesion Abierta Ok\n";
	cout << "Registrando Sesion \n";
#endif
	if ( RegisterSession(conf.session) == Error){
	    ostringstream oss;
		oss << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
		mensaje_error = "Error : Registrando Sesion " + string(cip_err_msg) + oss.str();
		escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
	    return -1;
	}
#ifdef _DEBUG_
    cout << "Sesion Registrada Ok\n";
	cout << "Conectando con Dispositivo " << conf.str_tipo << " \n";
#endif
	conf.connection = ConnectPLCOverCNET(
			conf.session, // session whe have open
			conf.tipo, // plc type
			0x87654321, // Target To Originator connection ID
			0x9876, // Connection Serial Number
			100, // Request Packet Interval
			path, // Path to the ControlLogix
			spath// path size
			);

	if (conf.connection == NULL){
	    ostringstream oss;
        oss << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
        mensaje_error = "Error : Conectando con Dispositivo " + string(cip_err_msg) + oss.str();
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
	    return -1;
	}
#ifdef _DEBUG_
	cout << "Conectado con Dispositivo " << conf.str_tipo << " OK\n";
#endif
	escribir_log( "as-clx_ethip", LOG_INFO, string("Conectado con Dispositivo " + string(conf.direccion)).c_str() );
    return 0;
}

/**
* @brief	Realiza la desconexion con el dispositivo y cierra
*			la sesion a traves CIP
* @return	int		Cierre de la Comunicacion
*/
int finalizar_comunicacion(){
#ifdef _DEBUG_
	cout << "Cerrando Conexion\n";
#endif
	if (Forward_Close(conf.connection) == Error){
        ostringstream oss;
        oss << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
        mensaje_error = "Error : Cerrando Conexion " + string(cip_err_msg) + oss.str();
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
	    return -1;
	}
#ifdef _DEBUG_
	cout << "Conexion Cerrada OK "<< cip_err_msg << endl;
#endif
	UnRegisterSession(conf.session);
	CloseSession(conf.session);
#ifdef _DEBUG_
	cout << "Registro de Sesion Culminado : " << cip_err_msg << endl;
	cout << "Sesion Cerrada\n";
#endif
	escribir_log( "as-clx_ethip", LOG_INFO, "Conexion Cerrada..." );
    return 0;
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
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "rs:d:",long_options, &option_index);
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
		cout << "\tProyecto Argos (as-clx_ethip) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
* 			Configuracion
* @return 	verdadero	Construccion Exitosa
*/
bool construir_shm_registros() {
    TiXmlDocument doc(__ARCHIVO_CONF_DEVS);
    if ( !doc.LoadFile() ) {
    	mensaje_error = "Error Cargando Archivo de Configuracion: " + string(__ARCHIVO_CONF_DEVS) + "\n" + string(strerror(errno));
        escribir_log( "as-clx_ethip", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    //string nombre = ele->GetText();

	ele = ele->NextSiblingElement();
	for (int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
		if (!ele || strcmp(ele->Value(),"dispositivo") != 0) {
			escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <dispositivo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <dispositivo>");
		}
		TiXmlAttribute * attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-clx_ethip", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
		}
		char id[10];
		strcpy(id, attr->Value());
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-clx_ethip", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
		}
		char modelo[100];
		strcpy(modelo, attr->Value());
		if( strcmp(modelo,"AB") != 0 ){
			continue;
		}
		else{
			TiXmlElement * sub_ele = ele->FirstChildElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
			}
			string marca_id_nombre = string(modelo) + "_" + string(id) + "_" + string(sub_ele->GetText());
			strcpy(conf.nombre_marca_id,marca_id_nombre.c_str());
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "marca") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <marca>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <marca>");
			}
			//strcpy(dispositivo.marca,sub_ele->GetText());
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "tipo") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
			}
			if(strcmp("LGX", sub_ele->GetText() ) == 0)
				conf.tipo = LGX;
			else if(strcmp("SLC500", sub_ele->GetText() ) == 0)
				conf.tipo = SLC500;
			else if(strcmp("PLC5", sub_ele->GetText() ) == 0)
				conf.tipo = PLC5;
			else
				conf.tipo = Unknow;
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "habilitado") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <habilitado>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <habilitado>");
			}
			conf.habilitado = (bool) atoi( sub_ele->GetText() );
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "comunicacion") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <comunicacion>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <comunicacion>");
			}
			TiXmlElement * sub_sub_ele = sub_ele->FirstChildElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "tipo") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
			}
			strcpy(conf.str_tipo,sub_sub_ele->GetText());
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "direccion") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <direccion>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <direccion>");
			}
			strcpy(conf.direccion,sub_sub_ele->GetText());
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "ruta") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <ruta>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <ruta>");
			}
			if(sub_sub_ele->GetText())
				strcpy(conf.ruta,sub_sub_ele->GetText());
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "nodo") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nodo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nodo>");
			}
			if(sub_sub_ele->GetText())
				strcpy(conf.nodo,sub_sub_ele->GetText());
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "grupos") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupos>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <grupos>");
			}
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "tiempo_muestreo") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_muestreo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_muestreo>");
			}
			conf.tiempo_muestreo = atoi( sub_ele->GetText() );
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "lectura") != 0) {
				escribir_log( "as-clx_ethip", LOG_ERR, "Error Parseando Archivo de Configuracion -> <lectura>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <lectura>");
			}
			sub_sub_ele = sub_ele->FirstChildElement();
			int indice = 0;
			registro actual;
			while( sub_sub_ele && strcmp(sub_sub_ele->Value(), "registro") == 0 ){
				if(indice > _MAX_CANT_REGS_){
					escribir_log( "as-clx_ethip", LOG_ERR, "Error Superada Maxima la Cantidad de Registros Permitidos" );
					throw runtime_error("Error Superada Maxima la Cantidad de Registros Permitidos");
				}
				attr = sub_sub_ele->FirstAttribute();
				if( !attr || strcmp(attr->Name(), "direccion") != 0){
					escribir_log( "as-clx_ethip", LOG_ERR, "Error Obteniendo Atributos de <registro> -> direccion" );
					throw runtime_error("Error Obteniendo Atributos de <registro> -> direccion");
				}
				strcpy(conf.registros[indice].direccion, attr->Value());
				strcpy(actual.direccion, attr->Value());
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "tipo") != 0){
					escribir_log( "as-clx_ethip", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tipo");
					throw runtime_error("Error Obteniendo Atributos de <registro> -> tipo");
				}
				TEXTO2DATO(conf.registros[indice].tipo, attr->Value());
				TEXTO2DATO(actual.tipo, attr->Value() );
				actual.valor.L = _ERR_DATO_;
				attr = attr->Next();
				if( attr && strcmp(attr->Name(), "escritura") == 0){
					conf.registros[indice].escritura = ( atoi( attr->Value() ) == 1 ) ?
						true : false;
				}
				conf.vregistros.push_back( actual );
				indice++;
				sub_sub_ele = sub_sub_ele->NextSiblingElement();
			}

			break;
		}
	}


    return true;
}

/**
* @brief	Verifica si el registro esta habilitado para ser escrito
* @param	reg			Registro a Buscar
* @param	pos			Posicion dentro del Vector
* @return	verdadero	Habilitado para ser escrito
*/
bool habilitado_escritura( registro reg, int & pos ){
	for( int i = 0; i < conf.vregistros.size(); i++ ){
		if( strcmp( conf.registros[i].direccion,
					reg.direccion ) == 0 ){
			pos = i;
			return conf.registros[i].escritura;
		}
	}
	pos = -1;
	return false;
}

/**
* @brief	Envia el Mensaje para la Escritura de un
*			valor en el dispositivo
*/
void escribir_a_dispositivo(){
	registro			tag_escritura;
	struct sigaction	action;
	sigset_t			set;
	unsigned int		prio;
	int					nbytes,
						posicion,
						res;
	int					h_ipath[50],
						h_spath = path_from_char2byte(h_ipath);			//Obtener la Ruta de Acceso al Dispositivo
	BYTE  				*h_path;
	Eip_Session         *session;
	Eip_Connection      *connection;
	string              mensaje_hilo_error;


	action.sa_handler = timeout_ack;
	action.sa_flags = 0;
	sigemptyset(&(action.sa_mask));
	sigaction(SIGALRM,&action,NULL);
	sigemptyset( &set );
	sigaddset( &set, SIGALRM );
	pthread_sigmask (SIG_UNBLOCK, &set, NULL);

	if(h_spath > 0)
		h_path = new BYTE[h_spath];
	if(h_path){
		for(int i = 0; i < h_spath; i++)
			h_path[i] = h_ipath[i];
	}
    while( true ){
		usleep( 500000 );       // Esperar 500 mseg entre cada operacion de escritura
		if( -1 == ( nbytes = mq_receive( conf.qd,
								(char *)&tag_escritura, 8192, &prio) ) ){
			if( errno == EAGAIN ){      // Si no hay mensajes en la Cola, se detiene la ejecucion
				usleep( 500000 );       // por 500 milisegundos y luego se inicia el siguiente ciclo
				continue;
			}
			else{
				mensaje_hilo_error = "[HILO]Error Recibiendo Mensaje de Cola: " + string( strerror( errno ) );
				escribir_log( "as-clx_ethip", LOG_ERR, mensaje_hilo_error.c_str() );
				usleep( 500000 );
				continue;
			}
		}
#ifdef _DEBUG_
		cout << "\n---------->\n[HILO]Escribiendo Registro:\t" << tag_escritura;
#endif
		if( !habilitado_escritura( tag_escritura, posicion ) ){
#ifdef _DEBUG_
            cout << "[HILO]Imposible Escribir\nRegistro No esta habilitado para escritura" << endl;
#endif
			escribir_log( "as-clx_ethip", LOG_ERR, "[HILO]Imposible Escribir\nRegistro No esta habilitado para escritura" );
			continue;
		}
		else{
            //cip_debuglevel=LogDebug; // You can uncomment this line to see the data exchange between TuxEip and your Logic controller
#ifdef _DEBUG_
            cout << "[HILO]Arrancando, Pid = " << getpid() << endl;
            cout << "[HILO]Abriendo Sesion \n";
#endif
            session = OpenSession(conf.direccion);
            if(session == NULL){
                ostringstream oss;
                oss << "(" << cip_errno << ":" + cip_ext_errno << ")\n";
                mensaje_hilo_error = "[HILO]Error : Abriendo Sesion " + string(cip_err_msg) + oss.str();
                escribir_log( "as-clx_ethip", LOG_ERR, mensaje_hilo_error.c_str() );
                throw runtime_error( mensaje_hilo_error.c_str() );
            }
#ifdef _DEBUG_
            cout << "[HILO]Sesion Abierta Ok\n";
            cout << "[HILO]Registrando Sesion \n";
#endif
            res = RegisterSession(session);
            if (res != Error){
#ifdef _DEBUG_
                cout << "[HILO]Sesion Registrada Ok\n";
                cout << "[HILO]Conectando con Dispositivo " << conf.str_tipo << " \n";
#endif
                connection = ConnectPLCOverCNET(
                    session, // session whe have open
                    conf.tipo, // plc type
                    0x12345678, // Target To Originator connection ID
                    0x6789, // Connection Serial Number
                    50, // Request Packet Interval (RPI)
                    h_path, // Path to the ControlLogix
                    h_spath// path size
                    );
                if (connection != NULL){
#ifdef _DEBUG_
                    cout << "[HILO]Conectado con Dispositivo " << conf.str_tipo << " OK\n";
#endif
					res = intento_escritura(tag_escritura,session,connection);
					if(res == Error){
						escribir_log( "as-clx_ethip", LOG_ERR, string("[HILO]Error : Escribiendo " + string(cip_err_msg) +
								"(" + numero2string(cip_errno, std::dec) + ":" + numero2string(cip_ext_errno, std::dec) + ")").c_str() );
                        escribir_log( "as-clx_ethip", LOG_ERR, "[HILO]Error : Esperando 1,5 seg para hacer nuevo intento" );
                        usleep( 1500000 );      // Esperar 1,5 seg proxima operacion de escritura
						res = intento_escritura(tag_escritura,session,connection);
						if(res == Error){
							escribir_log( "as-clx_ethip", LOG_ERR, string("[HILO]Error : Escribiendo " + string(cip_err_msg) +
								"(" + numero2string(cip_errno, std::dec) + ":" + numero2string(cip_ext_errno, std::dec) + ")").c_str() );
							escribir_log( "as-clx_ethip", LOG_ERR, "[HILO]Error : Esperando 2 seg para hacer nuevo intento" );
                            usleep( 2000000 );      // Esperar 2 seg proxima operacion de escritura
							res = intento_escritura(tag_escritura,session,connection);
							if( res == Error ){
							    escribir_log( "as-clx_ethip", LOG_ERR, string("[HILO]Error : Imposible Escribir ( " +
									string(tag_escritura.direccion) + ")").c_str() );
							}
#ifdef _DEBUG_
                            else{
                                 cout << "[HILO]Escritura OK, intento 3: (" << tag_escritura.direccion << ")" << endl;
                            }
#endif
						}
#ifdef _DEBUG_
                        else{
                             cout << "[HILO]Escritura OK, intento 2: (" << tag_escritura.direccion << ")" << endl;
                        }
#endif

					}
#ifdef _DEBUG_
					else{
					     cout << "[HILO]Escritura OK, intento 1: (" << tag_escritura.direccion << ")" << endl;
					}
#endif
				}
				else{
                    mensaje_hilo_error = "[HILO]Error : Conectando con Dispositivo " + string(cip_err_msg) +
										"(" + numero2string(cip_errno, std::dec) + ":" + numero2string(cip_ext_errno, std::dec) + ")";
                    escribir_log( "as-clx_ethip", LOG_ERR, mensaje_hilo_error.c_str() );
                    throw runtime_error( mensaje_hilo_error.c_str() );
                }
                UnRegisterSession(session);
#ifdef _DEBUG_
                cout << "[HILO]Registro de Sesion Culminado : " << cip_err_msg << endl;
#endif
            }
            else{
                mensaje_hilo_error = "[HILO]Error : Registrando Sesion " + string(cip_err_msg) +
										"(" + numero2string(cip_errno, std::dec) + ":" + numero2string(cip_ext_errno, std::dec) + ")";
                escribir_log( "as-clx_ethip", LOG_ERR, mensaje_hilo_error.c_str() );
                throw runtime_error( mensaje_hilo_error.c_str() );
            }
#ifdef _DEBUG_
            cout << "[HILO]Sesion Cerrada\n";
#endif
            CloseSession(session);

        }
    }
	delete [] h_path;
}

/**
* @brief	Efectua la operacion de escritura en el dispositivo
* @param	tag_escritura	Valor a escribir
* @param	session			Sesion registrada Ethernet/IP
* @param	connection		Conexion abierta Ethernet/IP
* @return	res				Resultado de la Escritura
*/
int intento_escritura( registro tag_escritura, Eip_Session * session, Eip_Connection * connection ){
	int		res;
	switch(tag_escritura.tipo){
		case CARAC_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_SINT,&(tag_escritura.valor.C),1);
		break;
		case CORTO_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_INT,&(tag_escritura.valor.S),1);
		break;
		case ENTERO_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_DINT,&(tag_escritura.valor.I),1);
		break;
		case LARGO_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_DINT,&(tag_escritura.valor.L),2);
		break;
		case REAL_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_REAL,&(tag_escritura.valor.F),1);
		break;
		case DREAL_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_REAL,&(tag_escritura.valor.D),2);
		break;
		case BIT_:
			res = WriteLgxData(session,
						connection,
						tag_escritura.direccion,
						LGX_BOOL,&(tag_escritura.valor.B),1);
		break;
		case TERROR_:
		default:
			escribir_log( "as-clx_ethip", LOG_ERR, "[HILO]Imposible Escribir Tipo de Registro Desconocido" );

	}
	return res;
}

/**
* @brief	Tiempo Consumido en el envio del Mensaje de
*			escritura al dispositivo
* @param	sig			Senal ALARM
*/
void timeout_ack(int sig){
	cout << "Timeout. Signal: " << sig << endl;
}

/**
* @brief	Transformacion de la ruta del Modulo de Comunicacion
*			de String a Int
* @param	p		Ruta
* @return	i		Longitud de la Ruta
*/
int path_from_char2byte(int * p){
	char    *str,
            ruta[100];
	int     i = 0;
	strcpy(ruta, conf.ruta);
	str = strtok(ruta,",");
	while(str != NULL){
		*(p + i) = atoi(str);
		str = strtok( NULL, "," );
		i++;
	}
	return i;
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
	sigaction( SIGQUIT, &n_accion, NULL );
	sigaction( SIGTERM, &n_accion, NULL );
	sigaction( SIGUSR1, &n_accion, NULL );
	sigaction( SIGUSR2, &n_accion, NULL );

	sigemptyset( set );
	sigaddset( set, SIGHUP );
	sigaddset( set, SIGQUIT );
	sigaddset( set, SIGTERM );
	sigaddset( set, SIGUSR1 );
	sigaddset( set, SIGUSR2 );

	//sigprocmask( SIG_BLOCK, set, NULL );
	pthread_sigmask (SIG_BLOCK, set, NULL);

	return true;
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
	escribir_log( "as-clx_ethip", LOG_INFO, string("Finalizando Proceso. Senal Recivida: " + numero2string(signum, std::dec)).c_str() );
	int res;
	if( conf.connection ){
		res = Forward_Close( conf.connection );
#ifdef _DEBUG_
	cout << "Cerrando Conexion..................." << endl;
#endif
	}
	if( conf.session ){
		UnRegisterSession( conf.session );
#ifdef _DEBUG_
	cout << "Registro de Sesion Culminado........" << endl;
#endif
		CloseSession( conf.session );
#ifdef _DEBUG_
	cout << "Cerrando Session...................." << endl;
#endif
	}
	if( conf.fd != -1 ){
		close( conf.fd );
#ifdef _DEBUG_
	cout << "Cerrando SHMEM......................" << endl;
#endif
	}
	escribir_log( "as-clx_ethip", LOG_INFO, "Terminando Servicio..." );
	exit(EXIT_SUCCESS);

}


