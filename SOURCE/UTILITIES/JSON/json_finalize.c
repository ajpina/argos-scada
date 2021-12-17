

#include "privateJsonHeader.h"


void json_finalize( ) {
    free(jsonString);
    free(tokens);
}
