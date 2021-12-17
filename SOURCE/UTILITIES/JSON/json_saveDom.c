#include "privateJsonHeader.h"
#include <stdio.h>

void json_saveDom( char * fName ) {
FILE * F;

   F = fopen( fName, "w" );
   fwrite( ( const void * )&imax, sizeof( int ), 1, F );
   fwrite( ( const void * )tokens, sizeof( jsontok_t ), imax, F );
   fclose( F );
}


