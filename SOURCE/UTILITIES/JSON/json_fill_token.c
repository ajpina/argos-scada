

#include "privateJsonHeader.h"


/**
 * Fills token type and boundaries.
 */
void json_fill_token(jsontok_t *token, const jsontype_t type,
                            const int start, const int end) {
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

