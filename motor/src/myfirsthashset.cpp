

#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include "ssutil.h"
#include "sstags.h"
#include "ssthtag.h"
#include "ssshmtag.h"
#include "ssalarma.h"
#include "ssthalarma.h"
#include "ssshmalarma.h"
#include "tinyxml.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <errno.h>
#include <stdexcept>


using namespace std;


bool construir_shm_th(shm_thtag *);
bool construir_shm_tha(shm_thalarma *);

int main(int argc, char * argv[]){

	struct timeval cTime1, cTime2, cTime3, cTime4, cTime5;
	gettimeofday( &cTime1, (struct timezone*)0 );
	int z = 1000;
	int MAX,x;
	bool escritura = false;
	bool imprimir = false;
	bool busqueda = false;
	bool actualizar = false;
	bool leyendo = false;

	cout << "reservando" << endl;

	shm_thtag shm_th(argv[8]);
	shm_thalarma shm_th2(argv[9]);

	if( argc > 3 ){
		z = atoi(argv[1]);
		MAX = atoi(argv[2]);
		escritura = (bool)atoi(argv[3]);
		imprimir = (bool)atoi(argv[4]);
		busqueda = (bool)atoi(argv[5]);
		actualizar = (bool)atoi(argv[6]);
		leyendo = (bool)atoi(argv[7]);
	}
	else{
		z = 1000;
		MAX = 100;
	}

	if(escritura){
		cout << "creando tabla" << endl;
		construir_shm_th(&shm_th);
//		construir_shm_tha(&shm_th2);
	}
	else{
		cout << "referenciando" << endl;
		shm_th.obtener_shm_tablahash();
		shm_th2.obtener_shm_tablahash();
	}


	gettimeofday( &cTime2, (struct timezone*)0 );
    if(cTime1.tv_usec > cTime2.tv_usec){
    	cTime2.tv_usec = 1000000;
    	--cTime2.tv_usec;
	}


	gettimeofday( &cTime3, (struct timezone*)0 );
	if(cTime2.tv_usec > cTime3.tv_usec){
		cTime3.tv_usec = 1000000;
	    --cTime3.tv_usec;
	}

	tags arreglo[10000];
	unsigned int ntags = 10000;
	shm_th.obtener_tags(arreglo,&ntags);

	alarma arreglo2[1000];
	unsigned int nalarmas = 1000;
	shm_th2.obtener_alarmas(arreglo2,&nalarmas);

	if(imprimir){
		cout << "imprimiendo" << endl;
		shm_th.imprimir_shm_tablahash();
		cout << "imprimiendo desde arreglo" << endl;
		for( unsigned int i = 0; i < ntags; i++)
			cout << arreglo[i] << endl;

		cout << "imprimiendo" << endl;
		shm_th2.imprimir_shm_tablahash();
		cout << "imprimiendo desde arreglo" << endl;
		for( unsigned int i = 0; i < nalarmas; i++)
			cout << arreglo2[i] << endl;
	}

	gettimeofday( &cTime4, (struct timezone*)0 );
    if(cTime3.tv_usec > cTime4.tv_usec){
    	cTime4.tv_usec = 1000000;
    	--cTime4.tv_usec;
	}
	tags t3;
	alarma a1;
	char tmp[100];
	Tvalor d;
	d.I = 25;
    if(busqueda){
    	cout << "buscando" << endl;

		for(int i = 0; i<MAX; i++){
			x = rand()%100000;
			sprintf(tmp,"%d",x);
			t3.asignar_propiedades( tmp, d, ENTERO_);
			cout << (shm_th.buscar_tag(&t3) == true ? "t" : "\r" );
		}
    }



	ssdireccion dir;
	Ealarma edo;
    if(actualizar){
    	cout << "actualizando" << endl;
    	for(int i = 0; i<100; i++){
    		usleep(10000);
			//x = rand()%100000;
    		x = i;
			d.I = x;
			t3.asignar_propiedades( "trasl_puente_Vel_motor_lado_dos", d, ENTERO_);
			shm_th.actualizar_tag(&t3);
			//x = rand()%6;
			x = i%6;

			switch(x){
			case 0:
				edo = BAJO_;
				break;
			case 1:
				edo = BBAJO_;
				break;
			case 2:
				edo = ALTO_;
				break;
			default:
				edo = AALTO_;
				break;
			}

			//a1.asignar_propiedades(0,"traslacion_puente",0,"prueba",
			//	"sobrecorriente_motor_lado_dos", edo, ,REAL_,dir,3);
//			shm_th2.actualizar_alarma(&a1);
//			cout << t3 << "\t\t" << a1 << "\r";
			fflush(stdout);
		}

    }

	gettimeofday( &cTime5, (struct timezone*)0 );
    if(cTime4.tv_usec > cTime5.tv_usec){
    	cTime5.tv_usec = 1000000;
    	--cTime5.tv_usec;
	}
	Tvalor val;
	val.I = 800;
	char nombre_t[_LONG_NOMB_TAG_], nombre_a[_LONG_NOMB_ALARM_];
	if(argv[10])
		strcpy( nombre_t, argv[10] );
	else
		strcpy( nombre_t, "trasl_puente_Arr_motor_lado_dos" );

	if(argv[11])
		strcpy( nombre_a, argv[11] );
	else
		strcpy( nombre_a, "sobrecorriente_motor_lado_dos" );

    if(leyendo){
    	cout << "leyendo" << endl;
    	for(int i = 0; i<1000000; i++){
    		t3.asignar_propiedades( nombre_t, d, ENTERO_);
    		shm_th.buscar_tag(&t3);
    		a1.asignar_propiedades(0,"traslacion_puente",0,"motor_lado_dos",
					nombre_a,edo,val,MAYOR_QUE_,ENTERO_);

    		shm_th2.buscar_alarma(&a1);
    		cout << t3 << "\t\t" << a1 << "\r";
    	}
    }


    cout << "cerrando" << endl;

	shm_th.cerrar_shm_tablahash();

	printf("\nTiempo Allocando: %ld s, %ld us. \n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );
	printf("\nTiempo Insertando: %ld s, %ld us. \n", cTime3.tv_sec - cTime2.tv_sec, cTime3.tv_usec - cTime2.tv_usec );
	printf("\nTiempo Iterando: %ld s, %ld us. \n", cTime4.tv_sec - cTime3.tv_sec, cTime4.tv_usec - cTime3.tv_usec );
	printf("\nTiempo Buscando: %ld s, %ld us. \n", cTime5.tv_sec - cTime4.tv_sec, cTime5.tv_usec - cTime4.tv_usec );

}

bool construir_shm_th(shm_thtag * shm ){
	TiXmlDocument doc("/home/ajpina/tags.xml");
	if( !doc.LoadFile() )
		throw runtime_error("Error Cargando Archivo de Configuracion de Tags");

	TiXmlHandle hdle(&doc);
	TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
	}
	int cantidad = atoi( ele->GetText() );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	string nombre = ele->GetText();
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"tag") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
	}
	TiXmlElement * sub_ele;
	char nombre_tag[100], valor_char[100];
	Tvalor valor_tag;
	Tdato tipo_tag;
	shm->crear_shm_tablahash(cantidad);
	tags t;
	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre") != 0){
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"valor_x_defecto") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <valor_x_defecto>");
		}
		strcpy(valor_char,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo>");
		}
		//tipo_tag = *(sub_ele->GetText());
		TEXTO2DATO(tipo_tag,sub_ele->GetText());
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
				valor_tag.L = 0;
		}
		t.asignar_propiedades(nombre_tag,valor_tag,tipo_tag);
		shm->insertar_tag(t);
	}
	return true;
}


bool construir_shm_tha(shm_thalarma * shm ){
	TiXmlDocument doc("/home/ajpina/alarmas.xml");
	if( !doc.LoadFile() )
		throw runtime_error("Error Cargando Archivo de Configuracion de Alarmas");

	TiXmlHandle hdle(&doc);
	TiXmlElement * ele = hdle.FirstChildElement().FirstChildElement().Element();
	if(!ele || strcmp(ele->Value(), "cantidad") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <cantidad>");
	}
	int cantidad = atoi( ele->GetText() );
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"nombre") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
	}
	string nombre_alar = ele->GetText();
	ele = ele->NextSiblingElement();
	if(!ele || strcmp(ele->Value(),"alarma") != 0){
		throw runtime_error("Error Parseando Archivo de Configuracion -> <tag>");
	}
	TiXmlElement * sub_ele;
	char	grupo[_LONG_NOMB_GALARM_],
			nombre[_LONG_NOMB_ALARM_],
			nombre_tag[_LONG_NOMB_TAG_];
	Ealarma estado;
	Tdato tipo_tag;
	ssdireccion registro;
	Tvalor val;
	short bit;

	shm->crear_shm_tablahash(cantidad);
	alarma a;
	for(int i = 0; i < cantidad; i++ , ele = ele->NextSiblingElement()){
		sub_ele = ele->FirstChildElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "grupo") != 0){
				throw runtime_error("Error Parseando Archivo de Configuracion -> <grupo>");
		}
		strcpy(grupo,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"nombre") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre>");
		}
		strcpy(nombre,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"estado_x_defecto") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <estado_x_defecto>");
		}
		//estado = texto2estado(sub_ele->GetText());
		TEXTO2ESTADO(estado,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(), "nombre_tag") != 0){
				throw runtime_error("Error Parseando Archivo de Configuracion -> <nombre_tag>");
		}
		strcpy(nombre_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"tipo_tag") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <tipo_tag>");
		}
		//tipo_tag = texto2dato(sub_ele->GetText());
		TEXTO2DATO(tipo_tag,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"direccion") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <direccion>");
		}
		strcpy(registro.D,sub_ele->GetText());
		sub_ele = sub_ele->NextSiblingElement();
		if(!sub_ele || strcmp(sub_ele->Value(),"bit") != 0){
			throw runtime_error("Error Parseando Archivo de Configuracion -> <bit>");
		}
		bit = static_cast<short>(atoi(sub_ele->GetText()));

		a.asignar_propiedades(0, grupo, 0, "sg", nombre,	estado, val, MAYOR_QUE_, ENTERO_);
		shm->insertar_alarma(a);
	}
	return true;
}

