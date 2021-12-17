

#include "privateJsonHeader.h"




char * arrayStep( char * Path, int * pos ) {
int    j;
char * X;

     j = *pos + 1;
     while ( ( Path[j] != '\0' ) && ( Path[j] >= '0' ) && ( Path[j] <= '9' ) ) {
        j++;
     }
     X = (char *) calloc( j - *pos, 1 );
     strncpy(X,Path + *pos + 1, j - *pos - 1 );
     *pos = j + 1;
     return X;
}
