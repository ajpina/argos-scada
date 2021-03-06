/**
* @file	as-mbtcp.cpp
* @brief		<b>Proceso encargado de Manejar la Comunicacion con
*				Dispositivos que soporten el Protocolo ModBus TCP.<br>
*				Este proceso utiliza la biblioteca de funciones:<br>
*				libmodbus-2.0.3 de St�phane Raimbault stephane.raimbault@gmail.com</b><br>
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
#include "as-mbtcp.h"

#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>

#define G_USEC_PER_SEC 1000000

using namespace std;
using namespace argos;

static int					verbose_flag;			//!< Modo Verboso

mbtcpConf::configuracion	conf;			//!< Objeto de Configuracion de as-mbtcp


/*--------------------------------------------------------
	Prototipos de Funciones Usadas en main()
----------------------------------------------------------*/
bool		asignar_opciones(int, char **);
bool		construir_shm_registros();
bool		registrar_manejador_senales(sigset_t *);
void		manejador_senales( int );
void		escribir_a_dispositivo();
int			intento_escritura(registro,modbus_param_t *);
void		timeout_ack( int );
int			establecer_comunicacion();
int			finalizar_comunicacion();
uint32_t	gettime();
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
    cout << "\tProyecto Argos (as-mbtcp) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
    cout << "\tRevision: " << AutoVersion::_FULLVERSION_STRING << "\n\n\n";
#endif

	escribir_log( "as-mbtcp", LOG_INFO, "Iniciando Servicio..." );

	//-----------------Opciones de Configuracion
	if ( !asignar_opciones(argc,argv) ) {
        mensaje_error = "Opcion Invalida";
        escribir_log( "as-mbtcp", LOG_ERR, mensaje_error.c_str() );
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


	int com = establecer_comunicacion();
	if( com == -1 )
		throw runtime_error( mensaje_error.c_str() );


	//Arrancar el hilo para la Escritura en el Dispositivo
	if( pthread_create( &hilo_escritura, NULL, (void *(*)(void *))escribir_a_dispositivo,
			NULL ) ){
		mensaje_error = "Error Creando Hilo para Escribir en Dispositivo: "
				+ string( strerror( errno ) );
		escribir_log( "as-mbtcp", LOG_ERR, mensaje_error.c_str() );
	}
	pthread_detach( hilo_escritura );


#ifdef _DEBUG_
	uint32_t	start;
	uint32_t	end;
	uint32_t	bytes;
	double		rate;
	double 		elapsed;
#endif

	uint8_t		tab_rp_status[_MAX_CANT_STATUS_MB_] = { 0x00 };
	uint16_t	tab_rp_registers[_MAX_CANT_REGS_MB_] = { 0x0000 };
	int 		ret = 0,
				pos = -1,
				ind1,
				ind2;
	uint32_t	indice = 0;
	registro	auxiliar[1];
	vector <registro>::iterator	it;

    while(true){

#ifdef _DEBUG_
		start = gettime();
#endif

//        memset(tab_rp_status, 0, 10 * sizeof(uint8_t));
//        memset(tab_rp_registers, 0, 10 * sizeof(uint16_t));

        indice = 0;
        while( indice < conf.bloq_count ){
#ifdef _DEBUG_
            cout << "\nAdquiriendo " << conf.registros[indice].direccion << endl;
#endif
			//Se elimina el primer caracter de la direccion
            switch(conf.registros[indice].tipo_mb){
            	case OCOILS_:
					 ret = read_coil_status(&conf.mb_param, conf.nodo,
								atoi(&conf.registros[indice].direccion[1]),
								conf.registros[indice].cantidad, tab_rp_status);


					if(ret != (int)conf.registros[indice].cantidad){
#ifdef _DEBUG_
						cout << "ERROR (" << ret << ") leyendo " << conf.registros[indice].direccion << endl;
#endif
						break;
					}
					strcpy( auxiliar[0].direccion, conf.registros[indice].direccion );
					it = search(conf.vregistros.begin(), conf.vregistros.end(), auxiliar, auxiliar + 1 );
					if( it != conf.vregistros.end() ){
							pos = int(it - conf.vregistros.begin());
							for( ind1 = conf.registros[indice].cantidad - 1, ind2 = 0;
									ind1 >= 0;
									ind1-- , ind2++ ){
#ifdef _DEBUG_
								cout << "Direccion " << conf.registros[indice].direccion << "+"
									<< ind2 << ": (" << bool(tab_rp_status[ind1]) << ")\n";
#endif
								(*(conf.bloque + pos + ind2)).valor.B = tab_rp_status[ind1] == 1 ? true : false;
							}
					}
#ifdef _DEBUG_
					else{
						cout << "Registro " << conf.registros[indice].direccion <<
							" No encontrado en tabla de registros" << endl;
					}
#endif
            	break;
            	case ICOILS_:
				   ret = read_input_status(&conf.mb_param, conf.nodo,
								atoi(&conf.registros[indice].direccion[1]),
								conf.registros[indice].cantidad, tab_rp_status);

					if (ret != (int)conf.registros[indice].cantidad){
#ifdef _DEBUG_
						cout << "ERROR (" << ret << ") leyendo " << conf.registros[indice].direccion << endl;
#endif
						break;
					}
					strcpy( auxiliar[0].direccion, conf.registros[indice].direccion );
					it = search(conf.vregistros.begin(), conf.vregistros.end(), auxiliar, auxiliar + 1 );
					if( it != conf.vregistros.end() ){
							pos = int(it - conf.vregistros.begin());
							ind1 = conf.registros[indice].cantidad - 1;
							ind2 = 0;
							for(; ind1 >= 0; ind1-- , ind2++){
#ifdef _DEBUG_
								cout << "Direccion " << conf.registros[indice].direccion << "+"
									<< ind2 << ": (" << bool(tab_rp_status[ind1]) << ")\n";
#endif
								(*(conf.bloque + pos + ind2)).valor.B = tab_rp_status[ind1] == 1 ? true : false;
							}
					}
#ifdef _DEBUG_
					else{
						cout << "Registro " << conf.registros[indice].direccion <<
							" No encontrado en tabla de registros" << endl;
					}
#endif
            	break;
            	case IREG_:
					ret = read_input_registers(&conf.mb_param, conf.nodo,
								atoi(&conf.registros[indice].direccion[1]),
								conf.registros[indice].cantidad, tab_rp_registers);

					if (ret != (int)conf.registros[indice].cantidad){
#ifdef _DEBUG_
						cout << "ERROR (" << ret << ") leyendo " << conf.registros[indice].direccion << endl;
#endif
						break;
					}
					strcpy( auxiliar[0].direccion, conf.registros[indice].direccion );
					it = search(conf.vregistros.begin(), conf.vregistros.end(), auxiliar, auxiliar + 1 );
					if( it != conf.vregistros.end() ){
						pos = int(it - conf.vregistros.begin());
						for( uint32_t i = 0; i < conf.registros[indice].cantidad; i++ ){
#ifdef _DEBUG_
							cout << "Direccion " << conf.registros[indice].direccion << "+"
									<< i << ": (" << std::hex << std::uppercase << tab_rp_registers[i]
									<< ") -> [" << std::dec << tab_rp_registers[i] << "]\n";
#endif
							(*(conf.bloque + pos + i)).valor.S = tab_rp_registers[i];
						}
					}
#ifdef _DEBUG_
					else{
						cout << "Registro " << conf.registros[indice].direccion <<
							" No encontrado en tabla de registros" << endl;
					}
#endif
            	break;
            	case HREG_:
				   ret = read_holding_registers(&conf.mb_param, conf.nodo,
								atoi(&conf.registros[indice].direccion[1]),
								conf.registros[indice].cantidad, tab_rp_registers);

					if (ret != (int)conf.registros[indice].cantidad){
#ifdef _DEBUG_
						cout << "ERROR (" << ret << ") leyendo " << conf.registros[indice].direccion << endl;
#endif
						break;
					}
					strcpy( auxiliar[0].direccion, conf.registros[indice].direccion );
					it = search(conf.vregistros.begin(), conf.vregistros.end(), auxiliar, auxiliar + 1 );
					if( it != conf.vregistros.end() ){
						pos = int(it - conf.vregistros.begin());
						for( uint32_t i = 0; i < conf.registros[indice].cantidad; i++ ){
#ifdef _DEBUG_
							cout << "Direccion " << conf.registros[indice].direccion << "+"
									<< i << ": (" << std::hex << std::uppercase << tab_rp_registers[i]
									<< ") -> [" << std::dec << tab_rp_registers[i] << "]\n";
#endif
							(*(conf.bloque + pos + i)).valor.S = tab_rp_registers[i];
						}
					}
#ifdef _DEBUG_
					else{
						cout << "Registro " << conf.registros[indice].direccion <<
							" No encontrado en tabla de registros" << endl;
					}
#endif

            	break;
            	case MBERROR_:
            	default:
						escribir_log( "as-mbtcp", LOG_ERR, "ERROR: Tipo de Direccion Desconocida" );
            }
			indice++;
		}

#ifdef _DEBUG_
        end = gettime();
        elapsed = (end - start) / 1000;
        rate = (conf.vregistros.size() * 1) * G_USEC_PER_SEC / (end - start);
        cout << "\n***ESTADISTICAS DE TRANSFERENCIA***\n";
        cout << "--------------------------------------------------------------------------------------\n";
        cout << "| Taza(reg/s) | Bytes | Tiempo(ms) | Ancho de Banda | Bytes+TCP | Ancho de Banda-TCP |\n";
        cout.setf( ios::fixed, ios::floatfield );
        cout << "|" << setw (13) << rate << "|";
        bytes = conf.vregistros.size() * 1 * sizeof(uint16_t);
        rate = ((double)bytes / 1024) * ((double)(G_USEC_PER_SEC / (end - start)));
        cout << std::setw(7) << bytes << "|" << std::setw(12) << std::setprecision(2) << elapsed << "|";
        cout << std::setw (10) << std::setprecision (2) << rate << " KB/s |";

        // TCP:Cabeceras y Datos
        bytes = 12 + 9 + (1 * sizeof(uint16_t));
        bytes = conf.vregistros.size() * bytes;
        cout << std::setw(11) << bytes << "|";
        rate = ((double)bytes / 1024) * ((double)(G_USEC_PER_SEC / (end - start)));
        cout << std::setw (14) << std::setprecision (2) << rate << " KB/s |\n";
        cout << "--------------------------------------------------------------------------------------\n";
#endif

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
	close(conf.fd);
	return(0);
}

/**
* @brief	Realiza la conexion con el dispositivo
* @return	int		Establecimiento de la Comunicacion
*/
int establecer_comunicacion(){
#ifdef _DEBUG_
	cout << "Arrancando, Pid = " << getpid() << endl;
#endif

	modbus_init_tcp(&conf.mb_param, conf.direccion, conf.puerto);

#ifdef _DEBUG_
	//modbus_set_debug(&conf.mb_param, TRUE);
#endif

	if (modbus_connect(&conf.mb_param) == -1){
		mensaje_error = "ERROR Conectando " + string(conf.direccion) + "(" +
			numero2string( conf.puerto, std::dec ) + ")\n";
		escribir_log( "as-mbtcp", LOG_ERR, mensaje_error.c_str() );
		return -1;
	}

#ifdef _DEBUG_
	cout << "Conectado con Dispositivo " << conf.direccion << " OK\n";
#endif
	escribir_log( "as-mbtcp", LOG_ERR, string("Conectado con Dispositivo " + string(conf.direccion)).c_str() );
    return 0;
}

/**
* @brief	Realiza la desconexion con el dispositivo
* @return	int		Cierre de la Comunicacion
*/
int finalizar_comunicacion(){

#ifdef _DEBUG_
	cout << "Cerrando Conexion\n";
#endif

	modbus_close(&conf.mb_param);
	escribir_log( "as-mbtcp", LOG_ERR, "Conexion Cerrada..." );
#ifdef _DEBUG_
	cout << "Conexion Cerrada OK\n";
#endif

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
		cout << "\tProyecto Argos (as-mbtcp) Version: " << AutoVersion::_UBUNTU_VERSION_STYLE << endl;
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
        escribir_log( "as-mbtcp", LOG_ERR, mensaje_error.c_str() );
        throw runtime_error( mensaje_error.c_str() );
    }
    TiXmlHandle hdle(&doc);
    TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
    if (!ele || strcmp(ele->Value(), "cantidad") != 0) {
    	escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <cantidad>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
    }
    int cantidad = atoi( ele->GetText() );
    ele = ele->NextSiblingElement();
    if (!ele || strcmp(ele->Value(),"nombre") != 0) {
    	escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
        throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
    }
    //string nombre = ele->GetText();

	ele = ele->NextSiblingElement();
	for (int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()) {
		if (!ele || strcmp(ele->Value(),"dispositivo") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <dispositivo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <dispositivo>");
		}
		TiXmlAttribute * attr = ele->FirstAttribute();
		if( !attr || strcmp(attr->Name(), "id") != 0){
			escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> id" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> id");
		}
		char id[10];
		strcpy(id, attr->Value());
		attr = attr->Next();
		if( !attr || strcmp(attr->Name(), "modelo") != 0){
			escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <dispositivo> -> modelo" );
			throw runtime_error("Error Obteniendo Atributos de <dispositivo> -> modelo");
		}
		char modelo[100];
		strcpy(modelo, attr->Value());
		TiXmlElement * sub_ele = ele->FirstChildElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nombre>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		string marca_id_nombre = string(modelo) + "_" + string(id) + "_" + string(sub_ele->GetText());
		strcpy(conf.nombre_marca_id,marca_id_nombre.c_str());
		sub_ele = sub_ele->NextSiblingElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "marca") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <marca>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <marca>");
		}
		//strcpy(dispositivo.marca,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "tipo") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
		}
		sub_ele = sub_ele->NextSiblingElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "habilitado") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <habilitado>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <habilitado>");
		}
		conf.habilitado = (bool) atoi( sub_ele->GetText() );
		sub_ele = sub_ele->NextSiblingElement();
		if (!sub_ele || strcmp(sub_ele->Value(), "comunicacion") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <comunicacion>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <comunicacion>");
		}
		TiXmlElement * sub_sub_ele = sub_ele->FirstChildElement();
		if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "tipo") != 0) {
			escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tipo>" );
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
		}
		strcpy(conf.str_tipo,sub_sub_ele->GetText());
		if( strcmp(conf.str_tipo,"MBTCP") != 0 ){
			continue;
		}
		else{
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "direccion") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <direccion>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <direccion>");
			}
			strcpy(conf.direccion,sub_sub_ele->GetText());
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "puerto") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <ruta>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <ruta>");
			}
			conf.puerto = atoi(sub_sub_ele->GetText());
			sub_sub_ele = sub_sub_ele->NextSiblingElement();
			if (!sub_sub_ele || strcmp(sub_sub_ele->Value(), "nodo") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <nodo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nodo>");
			}
			conf.nodo = static_cast<uint8_t> (atoi(sub_sub_ele->GetText()));
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "grupos") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <grupos>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <grupos>");
			}
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "tiempo_muestreo") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <tiempo_muestreo>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <tiempo_muestreo>");
			}
			conf.tiempo_muestreo = atoi( sub_ele->GetText() );
			sub_ele = sub_ele->NextSiblingElement();
			if (!sub_ele || strcmp(sub_ele->Value(), "lectura") != 0) {
				escribir_log( "as-mbtcp", LOG_ERR, "Error Parseando Archivo de Configuracion -> <lectura>" );
				throw runtime_error("Error Parseando Archivo de Configuracion -> <lectura>");
			}
			sub_sub_ele = sub_ele->FirstChildElement();
			int indice = 0;
			registro actual;
			while( sub_sub_ele && strcmp(sub_sub_ele->Value(), "registro") == 0 ){
				if(indice > _MAX_CANT_REGS_){
					escribir_log( "as-mbtcp", LOG_ERR, "Error Superada Maxima la Cantidad de Registros Permitidos" );
					throw runtime_error("Error Superada Maxima la Cantidad de Registros Permitidos");
				}
				attr = sub_sub_ele->FirstAttribute();
				if( !attr || strcmp(attr->Name(), "direccion") != 0){
					escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <registro> -> direccion" );
					throw runtime_error("Error Obteniendo Atributos de <registro> -> direccion");
				}
				strcpy(conf.registros[indice].direccion, attr->Value());
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "tipo") != 0){
					escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tipo" );
					throw runtime_error("Error Obteniendo Atributos de <registro> -> tipo");
				}
				TEXTO2DATO(conf.registros[indice].tipo, attr->Value());
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "cantidad") != 0){
					escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <registro> -> cantidad" );
					throw runtime_error("Error Obteniendo Atributos de <registro> -> cantidad");
				}
				conf.registros[indice].cantidad = atoi(attr->Value());
				attr = attr->Next();
				if( !attr || strcmp(attr->Name(), "tmb") != 0){
					escribir_log( "as-mbtcp", LOG_ERR, "Error Obteniendo Atributos de <registro> -> tmb" );
					throw runtime_error("Error Obteniendo Atributos de <registro> -> tmb");
				}
				TEXTO2DIRMB(conf.registros[indice].tipo_mb, attr->Value());
				attr = attr->Next();
				if( attr && strcmp(attr->Name(), "escritura") == 0){
					conf.registros[indice].escritura = ( atoi( attr->Value() ) == 1 ) ?
						true : false;
				}
				//Como la lectura se hace por bloques, se debe crear el vector de
				//registros desglosando cada bloque
				for( uint32_t j = 0; j < conf.registros[indice].cantidad; j++ ){
					sprintf(actual.direccion, "%06d", atoi(conf.registros[indice].direccion) + j );
					actual.tipo = conf.registros[indice].tipo;
					actual.valor.L = _ERR_DATO_;
					conf.vregistros.push_back( actual );
				}
				indice++;
				sub_sub_ele = sub_sub_ele->NextSiblingElement();
			}
			conf.bloq_count = indice;
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
	//Verifica primero si el registro corresponde con alguna
	//de las direcciones de inicio de los bloques
	for( uint32_t i = 0; i < conf.bloq_count; i++ ){
		if( strcmp( conf.registros[i].direccion,
					reg.direccion ) == 0 ){
			pos = i;
			return conf.registros[i].escritura;
		}
	}

	//En este caso se compara con cada uno de los registros
	//para luego asignar el estado de habilitacion de
	//acuerdo al bloque de registros
	for( uint32_t i = 0; i < conf.vregistros.size(); i++ ){
		if( strcmp( conf.vregistros[i].direccion,
					reg.direccion ) == 0 ){
			pos = i;
			for( uint32_t j = 0; j < conf.bloq_count; j++ ){
				if( conf.registros[j].direccion[0] == reg.direccion[0] ){
					return conf.registros[j].escritura;
				}
			}
			return false;
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
	modbus_param_t		*mbp = &conf.mb_param;
	string              mensaje_hilo_error;


	action.sa_handler = timeout_ack;
	action.sa_flags = 0;
	sigemptyset(&(action.sa_mask));
	sigaction(SIGALRM,&action,NULL);
	sigemptyset( &set );
	sigaddset( &set, SIGALRM );
	pthread_sigmask (SIG_UNBLOCK, &set, NULL);

	while( true ){
		usleep( 200000 );       // Esperar 200 mseg entre cada operacion de escritura
		if( -1 == ( nbytes = mq_receive( conf.qd,
								(char *)&tag_escritura, 8192, &prio) ) ){
			if( errno == EAGAIN ){      // Si no hay mensajes en la Cola, se detiene la ejecucion
				usleep( 500000 );       // por 500 milisegundos y luego se inicia el siguiente ciclo
				continue;
			}
			else{
				mensaje_hilo_error = "[HILO]Error Recibiendo Mensaje de Cola: " + string( strerror( errno ) );
				escribir_log( "as-mbtcp", LOG_ERR, mensaje_hilo_error.c_str() );
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
			escribir_log( "as-mbtcp", LOG_ERR, "[HILO]Imposible Escribir Registro No esta habilitado para escritura" );
			continue;
		}
		else{

			res = intento_escritura(tag_escritura,mbp);
			if(res == -1){
				escribir_log( "as-mbtcp", LOG_WARNING, "[HILO]Error : Esperando 1,5 seg para hacer nuevo intento" );
                usleep( 1500000 );      // Esperar 1,5 seg proxima operacion de escritura
				res = intento_escritura(tag_escritura,mbp);
				if(res == -1){
					escribir_log( "as-mbtcp", LOG_WARNING, "[HILO]Error : Esperando 2 seg para hacer nuevo intento" );
                    usleep( 2000000 );      // Esperar 2 seg proxima operacion de escritura
					res = intento_escritura(tag_escritura,mbp);
					if( res == -1 ){
						escribir_log( "as-mbtcp", LOG_ERR, string("[HILO]Error : Imposible Escribir -> " + string(tag_escritura.direccion)).c_str() );
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
            cout << "[HILO]Culminando Conexion\n";
#endif
        }
    }
}

/**
* @brief	Efectua la operacion de escritura en el dispositivo
* @param	tag_escritura	Valor a escribir
* @param	session			Sesion registrada Ethernet/IP
* @param	connection		Conexion abierta Ethernet/IP
* @return	res				Resultado de la Escritura
*/
int intento_escritura( registro tag_escritura, modbus_param_t * mbp ){
	int			res;
	uint8_t		unByte[8];
	uint16_t	*dosByte;

	switch(tag_escritura.tipo){
		case CARAC_:
			set_bits_from_byte(unByte, 0, uint8_t(tag_escritura.valor.C) );
			res = force_multiple_coils( mbp, conf.nodo,
						atoi( &tag_escritura.direccion[1] ), 8, unByte );
			if( res != 8 ){
				return -1;
			}
		break;
		case CORTO_:
			res = preset_single_register( mbp, conf.nodo,
						atoi( &tag_escritura.direccion[1] ), tag_escritura.valor.S );
			if( res != 1 ){
				return -1;
			}
		break;
		case ENTERO_:
			dosByte = reinterpret_cast<uint16_t *>( &(tag_escritura.valor.I) );
			res = preset_multiple_registers( mbp, conf.nodo,
						atoi( &tag_escritura.direccion[1] ), 2, dosByte );
			if( res != 2 ){
				return -1;
			}
		break;
		case LARGO_:
			dosByte = reinterpret_cast<uint16_t *>( &(tag_escritura.valor.L) );
			res = preset_multiple_registers( mbp, conf.nodo,
						atoi( &tag_escritura.direccion[1] ), 4, dosByte );
			if( res != 4 ){
				return -1;
			}
		break;
		case REAL_:
		/*    No Implementado    */
		break;
		case DREAL_:
		/*    No Implementado    */
		break;
		case BIT_:
			res = force_single_coil( mbp, conf.nodo,
						atoi( &tag_escritura.direccion[1] ),
						tag_escritura.valor.B );
			if( res != 1 ){
				return -1;
			}
		break;
		case TERROR_:
		default:
			escribir_log( "as-mbtcp", LOG_ERR, "[HILO]Imposible Escribir\nTipo de Registro Desconocido" );
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

	escribir_log( "as-mbtcp", LOG_INFO, string("Finalizando Proceso. Senal Recivida: " + numero2string(signum, std::dec)).c_str() );
	modbus_close(&conf.mb_param);

#ifdef _DEBUG_
	cout << "Cerrando Conexion...................." << endl;
#endif

	if( conf.fd != -1 ){
		close( conf.fd );
#ifdef _DEBUG_
		cout << "Cerrando SHMEM......................" << endl;
#endif
	}
	escribir_log( "as-mbtcp", LOG_INFO, "Terminando Servicio..." );
	exit(EXIT_SUCCESS);

}

/**
* @brief	Devuelve la fecha y tiempo actual en microsegndos
* @return	tv			Tiempo transcurrido
*/
uint32_t gettime(void)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (uint32_t) tv.tv_sec * G_USEC_PER_SEC + tv.tv_usec;
}





