#include <stdbool.h>
#include <stdlib.h>

#include "ctype.h"
#include "errno.h"
#include "lex.h"
#include "utils.h"
#include "value.h"

// Initialize lexer to read the given source.
void lexer_init(Lexer *l, const char *source) {
  l->source = source;
  l->token_start = source;
  l->current_char = source;
  l->line = 1;
}

// Read current char.
static inline char peek_char(Lexer *l) { return *l->current_char; }

// Read next char.
static inline char peek_next_char(Lexer *l) {
  if (peek_char(l) == '\0')
    return '\0';

  return l->current_char[1];
}

// Read current char and move cursor.
static inline char next_char(Lexer *l) {
  char c = peek_char(l);
  l->current_char++;
  if (c == '\n')
    l->line++;

  log_debug("lexer next char: %c (%d)\n", c, c);
  return c;
}

// If current char match expected, consume it and return true.
static inline bool next_char_if_match(Lexer *l, char c) {
  if (peek_char(l) != c)
    return false;
  next_char(l);
  return true;
}

static inline void prepare_token(Lexer *l, TokenType type) {
  l->next.start_char = l->token_start;
  l->next.length = l->current_char - l->token_start;
  l->next.line = l->line;
  l->next.type = type;
  l->next.value = LS_NULL;

  // Make line tokens appear on the line containing the "\n".
  if (type == TOKEN_LINE)
    l->next.line--;
}

static inline void prepare_number_token(Lexer *l, int base) {
  // Prepare token.
  prepare_token(l, TOKEN_NUMBER);

  // Compute token value.
  errno = 0;
  switch (base) {
  case 10:
    l->next.value = strtod(l->token_start, NULL);
    break;
  case 16:
    l->next.value = strtoll(l->token_start, NULL, 16);
    break;

  default:
    unreachable;
  }

  // Handler error.
  if (errno == ERANGE) {
    prepare_token(l, TOKEN_ERROR);
    l->next.value = LEXERR_INVALID_NUM_RANGE;
    return;
  } else if (errno != 0) {
    unreachable;
  }
}

static inline bool is_hex(char c) {
  return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'));
}

static inline void lex_hex_number(Lexer *l) {
  while (is_hex(peek_char(l)))
    next_char(l);

  prepare_number_token(l, 16);
}

// Lexes a number literal and store it in l->next.
static inline void lex_number(Lexer *l) {
  // Consume digits.
  while (isdigit(peek_char(l)))
    next_char(l);

  // Decimal number, continue
  if (peek_char(l) == '.') {
    next_char(l);
    while (isdigit(peek_char(l)))
      next_char(l);
  }

  // Scientific notation.
  if (next_char_if_match(l, 'e') || next_char_if_match(l, 'E')) {
    // +/-
    if (!next_char_if_match(l, '+')) {
      next_char_if_match(l, '-');
    }

    if (!isdigit(peek_char(l))) {
      prepare_token(l, TOKEN_ERROR);
      l->next.value = LEXERR_INVALID_NUM;
      return;
    }

    // Consume digits.
    while (isdigit(peek_char(l)))
      next_char(l);
  }

  prepare_number_token(l, 10);
}

// Lex a single token and returns it.
Token lex(Lexer *l) {
  l->prev = l->current;
  l->current = l->next;

  while (true) {
    l->token_start = l->current_char;
    char c = next_char(l);

    switch (c) {
    case '\0':
      prepare_token(l, TOKEN_EOF);
      // Prevent reading past end of string.
      l->current_char--;
      break;

    case '\t':
    case ' ':
      continue;

    case ';':
      prepare_token(l, TOKEN_SEMICOLON);
      break;

    case '\n':
      prepare_token(l, TOKEN_LINE);
      break;

    case '+':
      prepare_token(l, TOKEN_PLUS);
      break;

      // Negative number and minus.
    case '-':
      if (isdigit(peek_char(l)))
        lex_number(l);
      else
        prepare_token(l, TOKEN_MINUS);
      break;

    default:
      if (isdigit(c)) {
        if (next_char_if_match(l, 'x') && is_hex(peek_char(l)))
          lex_hex_number(l);
        else
          lex_number(l);
        break;
      }

      unreachable;
    }

    log_debug("lex return token of type %d\n", l->next.type);
    return l->next;
  }
}
