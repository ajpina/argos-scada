

#include "privateJsonHeader.h"


/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * All that follows is "new code" to augment the basic parsing functions
 *                                GWC
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

char * tokenName( int type ) {
   switch(type) {
     case json_UNDEFINED : return "UNDEFINED"; break;
     case json_OBJECT    : return "OBJECT";    break;
     case json_ARRAY     : return "ARRAY";     break;
     case json_STRING    : return "STRING";    break;
     case json_PRIMITIVE : return "PRIMITIVE"; break;
   }
   return "";   // Silence gcc warning 
}

