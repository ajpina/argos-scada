/**
* @file	as-multicast.cpp
* @brief		<b>Tipo Abstracto de Datos para el Itercambio de Datos
				Entre Servidores y Clientes (Usando Multicast)</b><br>
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

#include "as-multicast.h"
#include "as-util.h"
#include <stdexcept>
#include <errno.h>

using namespace std;
using namespace argos;

/**
* @brief 	Crea un objeto Multicast para Intercambias Tags o Alarmas
* @return 	referencia al objeto
*/
mcast::mcast(){
}

/**
* @brief 	Crea un objeto Multicast para Intercambias Tags o Alarmas
* @param	s		Productor o Consumidor
* @param	p		Puerto
* @param	g		Ip del Grupo
* @param	t		Time To Live
* @param	l		Replicar Localhost
* @return 	referencia al objeto
*/
mcast::mcast(bool s, int p, char * g, short t, short l){
	puerto = p;
	strcpy(grupo,g);
	servidor = s;
	ttl = t;
	loop = l;
	iniciar_mcast();
}

/**
* @brief	Reservar los recursos necesarios para iniciar la
*			Comunicacion Multicast como Productor o Consumidor
*/
void mcast::iniciar_mcast(){

	if(servidor){
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			escribir_log( "as-datos", LOG_ERR, "Abriendo Socket" );
			throw runtime_error("Abriendo Socket");
		}
		bzero((char *)&direccion, sizeof(direccion));
		direccion.sin_family = AF_INET;
		direccion.sin_port = htons(puerto);

		if(inet_aton(grupo, &direccion.sin_addr) < 0 ){
			escribir_log( "as-datos", LOG_ERR, "Definiendo Direccion" );
			throw runtime_error("Definiendo Direccion");
		}

		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
			escribir_log( "as-datos", LOG_ERR, "TTL Multicast" );
			throw runtime_error("TTL Multicast");
		}
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0 ) {
			escribir_log( "as-datos", LOG_ERR, "LOOP Multicast" );
			throw runtime_error("LOOP Multicast");
		}
	}
	else{
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			escribir_log( "as-cliente", LOG_ERR, "Abriendo Socket" );
			throw runtime_error("Abriendo Socket");
		}
		bzero((char *)&direccion, sizeof(direccion));
		direccion.sin_family = AF_INET;
		direccion.sin_port = htons(puerto);

		if(inet_aton(grupo, &direccion.sin_addr) < 0 ){
			escribir_log( "as-cliente", LOG_ERR, "Definiendo Direccion" );
			throw runtime_error("Definiendo Direccion");
		}

		if(bind(sock, (struct sockaddr *) &direccion, sizeof(direccion)) < 0) {
			escribir_log( "as-cliente", LOG_ERR, "Enlazando socket" );
			throw runtime_error("Enlazando socket");
		}

		requisitos.imr_multiaddr.s_addr = inet_addr(grupo);
		requisitos.imr_interface.s_addr = htonl(INADDR_ANY);
		if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,&requisitos, sizeof(requisitos)) < 0) {
			escribir_log( "as-cliente", LOG_ERR, "Uniendose a Multicast" );
			throw runtime_error("Uniendose a Multicast");
		}
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
			escribir_log( "as-cliente", LOG_ERR, "TTL Multicast" );
			throw runtime_error("TTL Multicast");
		}
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0 ) {
			escribir_log( "as-cliente", LOG_ERR, "LOOP Multicast" );
			throw runtime_error("LOOP Multicast");
		}
	}
}

/**
* @brief	Destructor del Objeto Multicast
*/
mcast::~mcast(){
}

/**
* @brief 	Asigna el numero de puerto al objeto Multicast
* @param 	p		Puerto
*/
void mcast::asignar_puerto(int p){
	puerto = p;
}

/**
* @brief 	Asigna la direccion IP del Grupo Multicast
* @param 	g		IP del Grupo
*/
void mcast::asignar_grupo(const char *g){
	strcpy(grupo, g);
}

/**
* @brief 	Asigna el valor de Time To Live
* @param 	t		Numero de TTL
*/
void mcast::asignar_ttl(short t){
	ttl = t;
}

/**
* @brief 	Especifica si existe Replicacion en lo
* @param 	l		Habilitar lo
*/
void mcast::asignar_loop(short l){
	loop = l;
}

/**
* @brief 	Especifica si opera como Productor o Consumidor del
*			Grupo Multicast
* @param 	s		Es Servidor
*/
void mcast::hacer_servidor(bool s){
	servidor = s;
}

/**
* @brief	Produce un Arreglo de Tags para enviar a
*			el grupo Multicast
* @param	msg				Paquete ha ser enviado al Grupo
* @param	ntags			Cantidad de Tags en el Paquete
* @param	ntags_x_msg		Cantidad de Tags que se envian en cada mensaje
* @param	frec			Tiempo entre mensajes
*/
void mcast::enviar_tags(Ttags_portable * msg, unsigned int ntags, unsigned int ntags_x_msg, unsigned long frec){

	unsigned int inicio = 0;
	ntags_x_msg = ntags_x_msg > _MAX_TAM_ARR_TAGS_ ? _MAX_TAM_ARR_TAGS_ : ntags_x_msg;
	unsigned int fin = ntags_x_msg < ntags ? ntags_x_msg : ntags;
	bool envio_completado = false;
	paquete_tags pqte;
	pqte.tipo = _TIPO_TAG_;
	int cnt;
	while(!envio_completado){
		memcpy(pqte.arreglo, msg + inicio, ntags_x_msg*(sizeof(Ttags_portable)));
		cnt = sendto(sock, &pqte, ntags_x_msg*(sizeof(Ttags_portable)) + 8*sizeof(char), 0, (struct sockaddr *) &direccion, sizeof(direccion));
		if (cnt < 0) {
			escribir_log( "as-datos", LOG_ERR, string("Enviando Paquete de Tags: " + string(strerror(errno))).c_str() );
			throw runtime_error("Enviando Paquete");
		}
#ifdef _DEBUG_
		for(unsigned int i = 0; i < ntags_x_msg; i++){
			switch(pqte.arreglo[i].tipo){
                case CARAC_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << static_cast<palabra_t> (pqte.arreglo[i].valor_campo.C) << "  tipo: CARACTER";
                break;
                case CORTO_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.S << "  tipo: CORTO";
                break;
                case ENTERO_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.I << "  tipo: ENTERO";
                break;
                case LARGO_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.L << "  tipo: LARGO";
                break;
                case REAL_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.F << "  tipo: REAL";
                break;
                case DREAL_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.D << "  tipo: DREAL";
                break;
                case BIT_:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.B << "  tipo: BIT";
                break;
                case TERROR_:
                default:
                    cout << "TAG " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  valor: " << pqte.arreglo[i].valor_campo.L << "  tipo: DESCONOCIDO";
            }
            cout << endl;
		}
#endif
		inicio = fin;
		if(inicio >= ntags)
			envio_completado = true;
		fin += ntags_x_msg;
		fin = fin > ntags ? ntags : fin;
		usleep(frec);
	}
}

/**
* @brief	Produce un Arreglo de Alarmas para enviar a
*			el grupo Multicast
* @param	msg				Paquete ha ser enviado al Grupo
* @param	nalarmas		Cantidad de Alarmas en el Paquete
* @param	nalarmas_x_msg	Cantidad de Alarmas que se envian en cada mensaje
* @param	frec			Tiempo entre mensajes
*/
void mcast::enviar_alarmas(Talarmas_portable * msg, unsigned int nalarmas,	unsigned int nalarmas_x_msg, unsigned long frec){
	unsigned int inicio = 0;
	nalarmas_x_msg = nalarmas_x_msg > _MAX_TAM_ARR_ALARM_ ? _MAX_TAM_ARR_ALARM_ : nalarmas_x_msg;
	unsigned int fin = nalarmas_x_msg < nalarmas ? nalarmas_x_msg : nalarmas;
	bool envio_completado = false;
	paquete_alarmas pqte;
	pqte.tipo = _TIPO_ALARMA_;
	int cnt;
	while(!envio_completado){
		memcpy(pqte.arreglo, msg + inicio, nalarmas_x_msg*sizeof(Talarmas_portable));
		cnt = sendto(sock, &pqte, nalarmas_x_msg*(sizeof(Talarmas_portable)) + 8*sizeof(char), 0, (struct sockaddr *) &direccion, sizeof(direccion));
		if (cnt < 0) {
			escribir_log( "as-datos", LOG_ERR, string("Enviando Paquete de Alarmas: " + string(strerror(errno))).c_str() );
			throw runtime_error("Enviando Paquete");
		}
#ifdef _DEBUG_
		for(unsigned int i = 0; i < nalarmas_x_msg; i++){
			switch(pqte.arreglo[i].estado){
                case BAJO_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: BAJO";
                break;
                case BBAJO_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: BAJO BAJO";
                break;
                case ALTO_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: ALTO";
                break;
                case AALTO_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: ALTO ALTO";
                break;
                case ACTIVA_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: ACTIVA";
                break;
                case DESACT_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: DESACTIVA";
                break;
                case RECON_:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: RECONOCIDA";
                break;
                case EERROR_:
                default:
                    cout << "ALARMA " << inicio + i  << "\tnombre: " << pqte.arreglo[i].nombre << "  estado: DESCONOCIDO";
            }
            cout << endl;
		}
#endif
		inicio = fin;
		if(inicio >= nalarmas)
			envio_completado = true;
		fin += nalarmas_x_msg;
		fin = fin > nalarmas ? nalarmas : fin;
		usleep(frec);
	}
}

/**
* @brief	Consumir un Mensaje desde el grupo Multicast
* @param	msg		Buffer para el Mensaje Recibido
* @param	tam		Tamano del Buffer
* @return	cnt		Bytes Recibidos
*/
int mcast::recibir_msg(void * msg, unsigned int tam){
	int longitud = sizeof(direccion);
	int cnt = recvfrom(sock, msg, tam, 0, (struct sockaddr *) &direccion, (socklen_t *) &longitud);
	if(cnt<0){
		throw runtime_error("Recibiendo Paquete");
	}
	return cnt;
}

