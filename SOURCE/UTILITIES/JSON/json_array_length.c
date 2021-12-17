

#include "privateJsonHeader.h"




int json_array_length( char * path ) {
int n;
    n = jsonNavigatePath( path );
    if ( n < 0 ) return n;
    if ( tokens[n].type == json_ARRAY ) {
       return tokens[n].size;
    }
    return NOT_AN_ARRAY;    
}
