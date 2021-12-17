#include "privateJsonHeader.h"
#include <stdio.h>

void json_loadDom( char * fName ) {
FILE * F;
int    i;

   F = fopen( fName, "r" );
   fread( ( void * )&i, sizeof( int ), 1, F );
   if ( i != imax ) {
      free( tokens );
      imax = i;
      tokens = ( jsontok_t * ) calloc( imax, sizeof( jsontok_t ) );
   }
   fread( ( void * )tokens, sizeof( jsontok_t ), imax, F );
   fclose( F );
}
