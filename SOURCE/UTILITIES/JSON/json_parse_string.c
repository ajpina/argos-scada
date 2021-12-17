

#include "privateJsonHeader.h"


/**
 * Fills next token with JSON string.
 */
int json_parse_string(json_parser *parser, const char *js,
                             const size_t len, jsontok_t *tokens,
                             const size_t num_tokens) {
  jsontok_t *token;

  int start = parser->pos;

  parser->pos++;

  /* Skip starting quote */
  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c = js[parser->pos];

    /* Quote: end of string */
    if (c == '\"') {
      if (tokens == NULL) {
        return 0;
      }
      token = json_alloc_token(parser, tokens, num_tokens);
      if (token == NULL) {
        parser->pos = start;
        return json_ERROR_NOMEM;
      }
      json_fill_token(token, json_STRING, start + 1, parser->pos);
      token->parent = parser->toksuper;  // GWC
      return 0;
    }

    /* Backslash: Quoted symbol expected */
    if (c == '\\' && parser->pos + 1 < len) {
      int i;
      parser->pos++;
      switch (js[parser->pos]) {
      /* Allowed escaped symbols */
      case '\"':
      case '/':
      case '\\':
      case 'b':
      case 'f':
      case 'r':
      case 'n':
      case 't':
        break;
      /* Allows escaped symbol \uXXXX */
      case 'u':
        parser->pos++;
        for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0';
             i++) {
          /* If it isn't a hex character we have an error */
          if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) ||   /* 0-9 */
                (js[parser->pos] >= 65 && js[parser->pos] <= 70) ||   /* A-F */
                (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
            parser->pos = start;
            return json_ERROR_INVAL;
          }
          parser->pos++;
        }
        parser->pos--;
        break;
      /* Unexpected symbol */
      default:
        parser->pos = start;
        return json_ERROR_INVAL;
      }
    }
  }
  parser->pos = start;
  return json_ERROR_PART;
}

