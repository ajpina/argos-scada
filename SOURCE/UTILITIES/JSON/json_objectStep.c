

#include "privateJsonHeader.h"


char * objectStep( char * Path, int * pos ) {
int    j;
char * X;

     j = *pos + 1;
     while ( ( Path[j] != '\0' ) && ( Path[j] != '.' ) && ( Path[j] != '[' ) ) {
        j++;
     }
     X = (char *) calloc( j - *pos, 1 );
     strncpy(X,Path + *pos + 1, j - *pos - 1 );
     *pos = j;
     return X;
}
