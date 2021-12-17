

#include "privateJsonHeader.h"




int json_each( char * jsonPath, jsonCallback cb, void * userData )  {  
int  n;

     n = jsonNavigatePath( jsonPath );
     if ( n < 0 ) return n;
     getNVpairs( n, 0, cb, userData);
     return 0;
}
