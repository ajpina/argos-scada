

#include "privateJsonHeader.h"


char * json_extract ( char * path ) {
int p;
    p = jsonNavigatePath( path );
    if ( p < 0 ) return NULL;
    return getTokenValue( tokens[p].start, tokens[p].end );
}
