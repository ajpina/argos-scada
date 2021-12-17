

#include "privateJsonHeader.h"


int json_tree( char * jsonPath, jsonCallback cb, void * userData )  {  
int  n;

     n = jsonNavigatePath( jsonPath );
     if ( n < 0 ) return n;
     walkTree( n, 0, cb, userData);
     return 0;
}
