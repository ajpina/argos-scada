

#include "privateJsonHeader.h"




/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * These routines are the high-level interface to this library. They are 
 * modelled after JSON support functions in most commercial and open-source
 * databases and they require that the DOM is built ...
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

char * json_error_string( int e ) {
     switch ( e ) {
        case PATH_START_ERROR   :  
                                  return "Path does not start with $ or @";
                                  break;
        case NOT_AN_ARRAY       :
                                  return "The tree location is not an array";
                                  break;
        case BAD_SUBSCRIPT      :
                                  return "The array doesn not have that many elements";
                                  break;
        case BAD_PATH           :
                                  return "Syntax error(s) in the path expression";
                                  break;
        case NO_CHILDREN        :
                                  return "The node in question has no descendants";
                                  break;
        case NO_SIBLING         :
                                  return "The node in question has no siblings";
                                  break;
        default                 :
                                  return "Unknown error code";
                                  break;
     } 
}

