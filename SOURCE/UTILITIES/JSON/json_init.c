

#include "privateJsonHeader.h"


/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
void json_init(json_parser *parser) {
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

