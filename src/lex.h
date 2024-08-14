#ifndef LEX_H
#define LEX_H

#include <stddef.h>

#include "value.h"

typedef enum {
  TOKEN_LPAREN,
  TOKEN_RPAREN,

  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_SLASH,
  TOKEN_STAR,

  TOKEN_SEMICOLON,

  TOKEN_IDENT,

  TOKEN_NUMBER,
  TOKEN_STRING,

  TOKEN_LET,

  TOKEN_LINE,

  TOKEN_EOF,
  TOKEN_ERROR,
} TokenType;

typedef struct {
  TokenType type;

  const char *start_char;

  ptrdiff_t length;

  // Line where token appear.
  unsigned int line;

  // Token value if any.
  Value value;
} Token;

typedef enum {
  LEXERR_INVALID_NUM,
  LEXERR_INVALID_NUM_RANGE,
  LEXERR_UNTERMINATED_BLOCK_COMMENT,
} LexError;

typedef struct {
  const char *source;

  // First token of current token.
  const char *token_start;

  // Current char being processed.
  const char *current_char;

  // The 1-based line where the token appear.
  unsigned int line;

  Token prev;
  Token current;
  Token next;
} Lexer;

// Initialize lexer to read the given source.
void lexer_init(Lexer *lexer, const char *source);

// Lex a single token and returns it.
Token lex(Lexer *lexer);

// Extract error from a token of type TOKEN_ERROR.
#define lex_token_error(tk) (LexError)((tk).value)

#endif
