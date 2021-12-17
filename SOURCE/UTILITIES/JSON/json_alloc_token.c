

#include "privateJsonHeader.h"



/**
 * Allocates a fresh unused token from the token pool.
 */
jsontok_t *json_alloc_token(json_parser *parser, jsontok_t *tokens,
                                   const size_t num_tokens) {
  jsontok_t *tok;
  if (parser->toknext >= num_tokens) {
    return NULL;
  }
  tok = &tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
  tok->parent = -1;  // GWC
  tok->child  = -1;  // GWC
  tok->sibling = -1; // GWC
  return tok;
}

