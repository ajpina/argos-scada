

#include "privateJsonHeader.h"


char * json_type( char * path ) {
int n;
    n = jsonNavigatePath( path );
    if ( n < 0 ) return json_error_string( n );
    return tokenName( tokens[n].type );
}
