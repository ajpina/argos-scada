

#include "privateJsonHeader.h"


char * getTokenValue( int S, int E ) {
char * X = NULL;
int    n;

       n = E - S;
       X = (char *) calloc( n + 1, 1 );
       strncpy(X, jsonString + S, n );
       return X;
}
