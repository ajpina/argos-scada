/**
* @file	as-emulmbtcp.cpp
* @brief		<b>Proceso encargado de Emular un Dispositivo de Hardware con
				Protocolo ModBus TCP, de esta manera se podra simular el
				Sistema SCADA por software sin necesidad de contar con un
				instrumento.<br>
*				Este proceso utiliza la biblioteca de funciones:<br>
*				libmodbus-2.0.3 de Stéphane Raimbault stephane.raimbault@gmail.com</b><br>
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

#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <climits>
#include <iostream>

#include "modbus.h"


modbus_mapping_t	mb_mapping;
uint16_t			tabla_ir[MAX_REGISTERS] = {0x0000};
uint8_t				tabla_is[MAX_REGISTERS] = {0x00};
volatile float		seem = FLT_MIN;

const uint16_t 		_NB_POINTS_SPECIAL_ = 0x2;
const uint32_t		_PUERTO_ = 1503;
#define				_IP_SERVIDOR_		"127.0.0.1"

using namespace std;

/**
* @brief	Invocada por un temporizador ciclico para actualizar los registros
* @param	int		Senal capturada
*/
void actualizar_registros(int sig)
{
    float		seno = sin(seem);
    uint8_t		unByte = uint8_t(seno * SCHAR_MAX);
    uint16_t	dosByte = uint16_t(seno * SHRT_MAX);

	for(int i = MAX_REGISTERS - 1; i > 0; i--){
		tabla_ir[i] = tabla_ir[i-1];
	}
	tabla_ir[0] = dosByte;
	for(int i = MAX_STATUS - 1; i > 0; i--){
		tabla_is[i] = tabla_is[i-1];
	}
	tabla_is[0] = unByte;
	//set_bits_from_byte(tabla_is, 0 , unByte);

	if( seem > (float)(FLT_MAX - 100.0) ){
		seem = FLT_MIN;
	}
    seem += 0.1;
	alarm(1);
}

/**
* @brief	Funci&oacute;n Principal
* @param	argc	Cantidad de Argumentos
* @param	argv	Lista de Argumentos
* @return	int		Finalizaci&oacute;n del Proceso
*/
int main(void)
{
        int socket;
        modbus_param_t mb_param;
        int ret;

        modbus_init_tcp(&mb_param, _IP_SERVIDOR_, _PUERTO_);
        //modbus_set_debug(&mb_param, TRUE);

		/* 0X */
        mb_mapping.nb_coil_status = MAX_STATUS;
        mb_mapping.tab_coil_status =
                (uint8_t *) malloc(MAX_STATUS * sizeof(uint8_t));
        memset(mb_mapping.tab_coil_status, 0,
               MAX_STATUS * sizeof(uint8_t));
        if (mb_mapping.tab_coil_status == NULL){
        	printf("Memory allocation failed\n");
            exit(1);
        }

        /* 1X */
        mb_mapping.nb_input_status = MAX_STATUS;
        mb_mapping.tab_input_status = tabla_is;

        /* 4X */
        mb_mapping.nb_holding_registers = MAX_REGISTERS;
        mb_mapping.tab_holding_registers =
                (uint16_t *) malloc(MAX_REGISTERS * sizeof(uint16_t));
        memset(mb_mapping.tab_holding_registers, 0,
               MAX_REGISTERS * sizeof(uint16_t));
        if (mb_mapping.tab_holding_registers == NULL) {
			free(mb_mapping.tab_coil_status);
			printf("Memory allocation failed\n");
            exit(1);
        }

        /* 3X */
        mb_mapping.nb_input_registers = MAX_REGISTERS;
        mb_mapping.tab_input_registers = tabla_ir;

		struct sigaction sact;
		sigemptyset( &sact.sa_mask );
		sact.sa_flags = 0;
		sact.sa_handler = actualizar_registros;
		sigaction( SIGALRM, &sact, NULL );

reinicio:
        socket = modbus_init_listen_tcp(&mb_param);
		alarm(1);
        while (1) {
                uint8_t query[MAX_MESSAGE_LENGTH];
                int query_size;

                ret = modbus_listen(&mb_param, query, &query_size);
                if (ret == 0) {
                        if (((query[HEADER_LENGTH_TCP + 4] << 8) + query[HEADER_LENGTH_TCP + 5])
                            == _NB_POINTS_SPECIAL_) {
                                /* Change the number of values (offset
                                   TCP = 6) */
                                query[HEADER_LENGTH_TCP + 4] = 0;
                                query[HEADER_LENGTH_TCP + 5] = MAX_REGISTERS;
                        }

                        modbus_manage_query(&mb_param, query, query_size, &mb_mapping);
                } else if (ret == CONNECTION_CLOSED) {
                        /* Conexion cerrada por el cliente */
                        alarm(0);
                        close(socket);
						modbus_close(&mb_param);
                        sleep(10);
                        goto reinicio;
                } else {
                        printf("Error en modbus_listen (%d)\n", ret);
                }
        }

        close(socket);
        free(mb_mapping.tab_coil_status);
        free(mb_mapping.tab_holding_registers);
        modbus_close(&mb_param);

        return 0;
}
