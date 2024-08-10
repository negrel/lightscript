#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lightscript.h"
#include "ls_buffer.h"
#include "ls_compiler.h"
#include "ls_options.h"
#include "ls_utf8.h"
#include "ls_value.h"

// The buffer size used to format a compile error message, excluding the header
// with the module name and error location. Using a hardcoded buffer for this
// is kind of hairy, but fortunately we can control what the longest possible
// message is and handle that. Ideally, we'd use `snprintf()`, but that's not
// available in standard C++98.
#define ERROR_MESSAGE_SIZE (80 + MAX_VARIABLE_NAME + 15)

typedef enum {
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,

  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,

  TOKEN_LEFT_BRACKET,
  TOKEN_RIGHT_BRACKET,

  TOKEN_AMP,
  TOKEN_AMPAMP,
  TOKEN_BANG,
  TOKEN_CARET,
  TOKEN_COLON,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_ELLIPSIS,
  TOKEN_HASH,
  TOKEN_LINE,
  TOKEN_MINUS,
  TOKEN_PERCENT,
  TOKEN_PIPE,
  TOKEN_PIPEPIPE,
  TOKEN_PLUS,
  TOKEN_QUESTION,
  TOKEN_SLASH,
  TOKEN_STAR,
  TOKEN_TILDE,

  TOKEN_EQ,
  TOKEN_LT,
  TOKEN_LTLT,
  TOKEN_GT,
  TOKEN_GTGT,
  TOKEN_LTEQ,
  TOKEN_GTEQ,
  TOKEN_EQEQ,
  TOKEN_BANGEQ,

  TOKEN_BREAK,
  TOKEN_CONTINUE,
  TOKEN_ELSE,
  TOKEN_FOR,
  TOKEN_IF,
  TOKEN_RETURN,
  TOKEN_WHILE,
  TOKEN_LET,
  TOKEN_CONST,

  TOKEN_NULL,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NUMBER,
  TOKEN_STRING,

  TOKEN_IDENT,

  TOKEN_ERROR,

  TOKEN_EOF,
} TokenType;

// Token define a lexically valid piece of LightScript code.
typedef struct {
  TokenType type;

  // The beginning of the token, pointing directly into the source.
  const char *start;
  // The length of the token in bytes.
  ptrdiff_t length;

  // The 1-based line where the token appears.
  ptrdiff_t line;

  // The parsed value if the token is a literal.
  LsValue value;
} Token;

// Parser define a lexer and parser in a single type.
typedef struct {
  LsVM *vm;

  // The source code being parsed.
  const char *source;

  // The current character being lexed in [source].
  const char *current_char;

  // The beginning of the currently-being-lexed token in [source].
  const char *token_start;

  // The 1-based line number of [current_char].
  ptrdiff_t current_line;

  // The upcoming token.
  Token next;

  // The most recently lexed token.
  Token current;

  // The most recently consumed/advanced token.
  Token previous;

  bool has_error;
  bool print_errors;
} Parser;

typedef struct {
  const char *identifier;
  size_t length;
  TokenType token_type;
} Keyword;

// The table of reserved words and their associated token types.
static Keyword keywords[] = {
    {"break", 5, TOKEN_BREAK}, {"continue", 8, TOKEN_CONTINUE},
    {"else", 4, TOKEN_ELSE},   {"false", 5, TOKEN_FALSE},
    {"for", 3, TOKEN_FOR},     {"if", 2, TOKEN_IF},
    {"null", 4, TOKEN_NULL},   {"return", 6, TOKEN_RETURN},
    {"true", 4, TOKEN_TRUE},   {"while", 5, TOKEN_WHILE},
    {NULL, 0, TOKEN_EOF} // Sentinel to mark the end of the array.
};

static void print_error(Parser *parser, int line, const char *label,
                        const char *format, va_list args) {
  parser->has_error = true;
  if (!parser->print_errors)
    return;

  // Only report errors if there is a LsErrorFn to handle them.
  if (parser->vm->config.on_error == NULL)
    return;

  // Format the label and message.
  char message[ERROR_MESSAGE_SIZE];
  int length = sprintf(message, "%s: ", label);
  length += vsprintf(message + length, format, args);
  assert(length < ERROR_MESSAGE_SIZE && "Error should not exceed buffer.");

  // TODO: replace "main" with real module name.
  parser->vm->config.on_error(parser->vm, LS_ERROR_COMPILE, "main", line,
                              message);
}

// Outputs a lexical error.
static void lex_error(Parser *parser, const char *format, ...) {
  va_list args;
  va_start(args, format);
  print_error(parser, parser->current_line, "Error", format, args);
  va_end(args);
}

// Returns the current character the parser is sitting on.
static char peek_char(Parser *parser) { return *parser->current_char; }

// Returns the character following current character.
static char peek_next_char(Parser *parser) {
  return *parser->current_char != '\0' ? *(parser->current_char + 1) : '\0';
}

// Advances the parser forward one character.
static char next_char(Parser *parser) {
  char c = peek_char(parser);
  parser->current_char++;
  if (c == '\n')
    parser->current_line++;
  return c;
}

// Skips the rest of a block comment.
static void skip_block_comment(Parser *parser) {
  int nesting = 1;
  while (nesting > 0) {
    if (peek_char(parser) == '\0') {
      lex_error(parser, "Unterminated block comment.");
      return;
    }

    if (peek_char(parser) == '/' && peek_next_char(parser) == '*') {
      next_char(parser);
      next_char(parser);
      nesting++;
      continue;
    }

    if (peek_char(parser) == '*' && peek_next_char(parser) == '/') {
      next_char(parser);
      next_char(parser);
      nesting--;
      continue;
    }

    // Regular comment character.
    next_char(parser);
  }
}

// Skips the rest of a line comment.
static void skip_line_comment(Parser *parser) {
  while (peek_char(parser) != '\n' && peek_char(parser) != '\0') {
    next_char(parser);
  }
}

// If current character is [c], consumes it and returns `true`, otherwise
// returns false.
static bool match_char(Parser *parser, char c) {
  if (peek_char(parser) != c)
    return false;

  next_char(parser);
  return true;
}

// Sets the parser's next token to the given [type] and current character
// range.
static void prepare_token(Parser *parser, TokenType type) {
  parser->next.type = type;
  parser->next.start = parser->token_start;
  parser->next.length = parser->current_char - parser->token_start;
  parser->next.line = parser->current_line;

  // Make line tokens appear on the line containing the "\n".
  if (type == TOKEN_LINE)
    parser->next.line--;
}

static inline void prepare_2char_token(Parser *parser, char next, TokenType two,
                                       TokenType one) {
  prepare_token(parser, match_char(parser, next) ? two : one);
}

static void lex_ident_or_keyword(Parser *parser) {
  ByteBuffer string;
  ls_byte_buffer_init(&string);
  ls_byte_buffer_write(parser->vm, &string, peek_char(parser));

  while (isalnum(peek_next_char(parser))) {
    char c = next_char(parser);
    ls_byte_buffer_write(parser->vm, &string, c);
  }

  TokenType ttype = TOKEN_IDENT;

  // Update the type if it's a keyword.
  size_t length = parser->current_char - parser->token_start;
  for (int i = 0; keywords[i].identifier != NULL; i++) {
    if (length == keywords[i].length &&
        memcmp(parser->token_start, keywords[i].identifier, length) == 0) {
      ttype = keywords[i].token_type;
      break;
    }
  }

  parser->next.value =
      ls_new_string_length(parser->vm, (char *)string.data, string.length);

  ls_byte_buffer_clear(parser->vm, &string);
  prepare_token(parser, ttype);
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int8_t lex_hex_digit(Parser *parser) {
  char c = next_char(parser);
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  // Don't consume it if it isn't expected. Keeps us from reading past the end
  // of an unterminated string.
  parser->current_char--;
  return -1;
}

// Lexes the numeric value of the current token.
static void prepare_number_token(Parser *parser, bool isHex) {
  errno = 0;

  if (isHex) {
    parser->next.value =
        ls_num2val((double)strtoll(parser->token_start, NULL, 16));
  } else {
    parser->next.value = ls_num2val(strtod(parser->token_start, NULL));
  }

  if (errno == ERANGE) {
    lex_error(parser, "Number literal was too large (%d).", sizeof(long int));
    parser->next.value = ls_num2val(0);
  }

  // We don't check that the entire token is consumed after calling strtoll()
  // or strtod() because we've already scanned it ourselves and know it's valid.
  prepare_token(parser, TOKEN_NUMBER);
}

static void lex_number(Parser *parser) {
  while (isdigit(peek_char(parser)))
    next_char(parser);

  // See if it has a floating point. Make sure there is a digit after the "."
  // so we don't get confused by method calls on number literals.
  if (peek_char(parser) == '.' && isdigit(peek_next_char(parser))) {
    next_char(parser);
    while (isdigit(peek_char(parser)))
      next_char(parser);
  }

  // See if the number is in scientific notation.
  if (match_char(parser, 'e') || match_char(parser, 'E')) {
    // Allow a single positive/negative exponent symbol.
    if (!match_char(parser, '+')) {
      match_char(parser, '-');
    }

    if (!isdigit(peek_char(parser))) {
      lex_error(parser, "Unterminated scientific notation.");
    }

    while (isdigit(peek_char(parser)))
      next_char(parser);
  }

  prepare_number_token(parser, false);
}

// Finishes lexing a hexadecimal number literal.
static void lex_hex_number(Parser *parser) {
  // Skip past the `x` used to denote a hexadecimal literal.
  next_char(parser);

  // Iterate over all the valid hexadecimal digits found.
  while (lex_hex_digit(parser) != -1)
    continue;

  prepare_number_token(parser, true);
}

// Reads [digits] hex digits in a string literal and returns their number value.
static uint8_t lex_hex_escape(Parser *parser, size_t digits,
                              const char *description) {
  int value = 0;
  for (size_t i = 0; i < digits; i++) {
    if (peek_char(parser) == '"' || peek_char(parser) == '\0') {
      lex_error(parser, "Incomplete %s escape sequence.", description);

      // Don't consume it if it isn't expected. Keeps us from reading past the
      // end of an unterminated string.
      parser->current_char--;
      break;
    }

    int digit = lex_hex_digit(parser);
    if (digit == -1) {
      lex_error(parser, "Invalid %s escape sequence.", description);
      break;
    }

    value = (value * 16) | digit;
  }

  return value;
}

// Reads a hex digit Unicode escape sequence in a string literal.
static void lex_unicode_escape(Parser *parser, ByteBuffer *string,
                               size_t length) {
  int value = lex_hex_escape(parser, length, "Unicode");

  // Grow the buffer enough for the encoded result.
  size_t bytes_len = ls_utf8_encode_bytes_len(value);
  if (bytes_len != 0) {
    ls_byte_buffer_fill(parser->vm, string, 0, bytes_len);
    ls_utf8_encode(value, string->data + string->length - bytes_len);
  }
}

static void lex_string(Parser *parser) {
  ByteBuffer string;
  TokenType type = TOKEN_STRING;
  ls_byte_buffer_init(&string);

  for (;;) {
    char c = next_char(parser);
    if (c == '"')
      break;
    if (c == '\r')
      continue;

    if (c == '\0') {
      lex_error(parser, "Unterminated string.");

      // Don't consume it if it isn't expected. Keeps us from reading past the
      // end of an unterminated string.
      parser->current_char--;
      break;
    }

    if (c == '\\') {
      switch (next_char(parser)) {
      case '"':
        ls_byte_buffer_write(parser->vm, &string, '"');
        break;
      case '\\':
        ls_byte_buffer_write(parser->vm, &string, '\\');
        break;
      case '%':
        ls_byte_buffer_write(parser->vm, &string, '%');
        break;
      case '0':
        ls_byte_buffer_write(parser->vm, &string, '\0');
        break;
      case 'a':
        ls_byte_buffer_write(parser->vm, &string, '\a');
        break;
      case 'b':
        ls_byte_buffer_write(parser->vm, &string, '\b');
        break;
      case 'e':
        ls_byte_buffer_write(parser->vm, &string, '\33');
        break;
      case 'f':
        ls_byte_buffer_write(parser->vm, &string, '\f');
        break;
      case 'n':
        ls_byte_buffer_write(parser->vm, &string, '\n');
        break;
      case 'r':
        ls_byte_buffer_write(parser->vm, &string, '\r');
        break;
      case 't':
        ls_byte_buffer_write(parser->vm, &string, '\t');
        break;
      case 'u':
        lex_unicode_escape(parser, &string, 4);
        break;
      case 'U':
        lex_unicode_escape(parser, &string, 8);
        break;
      case 'v':
        ls_byte_buffer_write(parser->vm, &string, '\v');
        break;
      case 'x':
        ls_byte_buffer_write(parser->vm, &string,
                             (uint8_t)lex_hex_escape(parser, 2, "byte"));
        break;

      default:
        lex_error(parser, "Invalid escape character '%c'.",
                  *(parser->current_char - 1));
        break;
      }
    } else {
      ls_byte_buffer_write(parser->vm, &string, c);
    }
  }

  parser->next.value =
      ls_new_string_length(parser->vm, (char *)string.data, string.length);

  ls_byte_buffer_clear(parser->vm, &string);

  prepare_token(parser, type);
}

// Lex the next token and store it in [parser.next].
static void next_token(Parser *parser) {
  parser->previous = parser->current;
  parser->current = parser->next;

  // If we are out of tokens, don't try to tokenize any more. We *do* still
  // copy the TOKEN_EOF to previous so that code that expects it to be consumed
  // will still work.
  if (parser->next.type == TOKEN_EOF)
    return;
  if (parser->current.type == TOKEN_EOF)
    return;

  while (peek_char(parser) != '\0') {
    parser->token_start = parser->current_char;

    char c = next_char(parser);
    switch (c) {
    case '(':
      prepare_token(parser, TOKEN_LEFT_PAREN);
      return;

    case ')':
      prepare_token(parser, TOKEN_RIGHT_PAREN);
      return;

    case '[':
      prepare_token(parser, TOKEN_LEFT_BRACKET);
      return;
    case ']':
      prepare_token(parser, TOKEN_RIGHT_BRACKET);
      return;
    case '{':
      prepare_token(parser, TOKEN_LEFT_BRACE);
      return;
    case '}':
      prepare_token(parser, TOKEN_RIGHT_BRACE);
      return;
    case ':':
      prepare_token(parser, TOKEN_COLON);
      return;
    case ',':
      prepare_token(parser, TOKEN_COMMA);
      return;
    case '*':
      prepare_token(parser, TOKEN_STAR);
      return;
    case '%':
      prepare_token(parser, TOKEN_PERCENT);
      return;
    case '#': {
      // Ignore shebang on the first line.
      if (parser->current_line == 1 && peek_char(parser) == '!' &&
          peek_next_char(parser) == '/') {
        skip_line_comment(parser);
        break;
      }
      // Otherwise we treat it as a token
      prepare_token(parser, TOKEN_HASH);
      return;
    }
    case '^':
      prepare_token(parser, TOKEN_CARET);
      return;
    case '+':
      prepare_token(parser, TOKEN_PLUS);
      return;
    case '-':
      prepare_token(parser, TOKEN_MINUS);
      return;
    case '~':
      prepare_token(parser, TOKEN_TILDE);
      return;
    case '?':
      prepare_token(parser, TOKEN_QUESTION);
      return;

    case '|':
      prepare_2char_token(parser, '|', TOKEN_PIPEPIPE, TOKEN_PIPE);
      return;
    case '&':
      prepare_2char_token(parser, '&', TOKEN_AMPAMP, TOKEN_AMP);
      return;
    case '=':
      prepare_2char_token(parser, '=', TOKEN_EQEQ, TOKEN_EQ);
      return;
    case '!':
      prepare_2char_token(parser, '=', TOKEN_BANGEQ, TOKEN_BANG);
      return;

    case '.':
      if (match_char(parser, '.')) {
        prepare_2char_token(parser, '.', TOKEN_ELLIPSIS, TOKEN_DOTDOT);
        return;
      }

      prepare_token(parser, TOKEN_DOT);
      return;

    case '/':
      if (match_char(parser, '/')) {
        skip_line_comment(parser);
        break;
      }

      if (match_char(parser, '*')) {
        skip_block_comment(parser);
        break;
      }

      prepare_token(parser, TOKEN_SLASH);
      return;

    case '<':
      if (match_char(parser, '<')) {
        prepare_token(parser, TOKEN_LTLT);
      } else {
        prepare_2char_token(parser, '=', TOKEN_LTEQ, TOKEN_LT);
      }
      return;

    case '>':
      if (match_char(parser, '>')) {
        prepare_token(parser, TOKEN_GTGT);
      } else {
        prepare_2char_token(parser, '=', TOKEN_GTEQ, TOKEN_GT);
      }
      return;

    case '\n':
      prepare_token(parser, TOKEN_LINE);
      return;

    case ' ':
    case '\r':
    case '\t':
      // Skip forward until we run out of whitespace.
      while (peek_char(parser) == ' ' || peek_char(parser) == '\r' ||
             peek_char(parser) == '\t') {
        next_char(parser);
      }
      break;

    case '"': {
      lex_string(parser);
      return;
    }

    case '0':
      if (peek_char(parser) == 'x') {
        lex_hex_number(parser);
        return;
      }

      prepare_number_token(parser, false);
      return;

    default:
      if (isalpha(c)) {
        lex_ident_or_keyword(parser);
      } else if (isdigit(c)) {
        lex_number(parser);
      } else {
        if (c >= 32 && c <= 126) {
          lex_error(parser, "Invalid character '%c'.", c);
        } else {
          // Don't show non-ASCII values since we didn't UTF-8 decode the
          // bytes. Since there are no non-ASCII byte values that are
          // meaningful code units in LightScript, the lexer works on raw bytes,
          // even though the source code and console output are UTF-8.
          lex_error(parser, "Invalid byte 0x%x.", (uint8_t)c);
        }
        parser->next.type = TOKEN_ERROR;
        parser->next.length = 0;
      }
      return;
    }
  }

  // If we get here, we're out of source, so just make EOF tokens.
  parser->token_start = parser->current_char;
  prepare_token(parser, TOKEN_EOF);
}

typedef struct {
  // The name of the local variable. This points directly into the original
  // source code string.
  const char *name;

  // The length of the local variable's name.
  int length;

  // The depth in the scope chain that this variable was declared at. Zero is
  // the outermost scope--parameters for a method, or the first local block in
  // top level code. One is the scope within that, etc.
  int depth;

  // If this local variable is being used as an upvalue.
  bool is_upvalue;
} Local;

typedef struct {
  // True if this upvalue is capturing a local variable from the enclosing
  // function. False if it's capturing an upvalue.
  bool is_local;

  // The index of the local or upvalue being captured in the enclosing function.
  size_t index;
} CompilerUpvalue;

struct ls_compiler {
  Parser *parser;

  // The compiler for the function enclosing this one, or NULL if it's the
  // top level.
  struct ls_compiler *parent_compiler;

  // The currently in scope local variables.
  Local locals[MAX_LOCALS];

  // The number of local variables currently in scope.
  size_t locals_count;

  // The upvalues that this function has captured from outer scopes. The count
  // of them is stored in [numUpvalues].
  CompilerUpvalue upvalues[MAX_UPVALUES];
};

// Returns the type of the current token.
static TokenType peek_token(LsCompiler *compiler) {
  return compiler->parser->current.type;
}

// Returns the type of the next token.
static TokenType peek_next_token(LsCompiler *compiler) {
  return compiler->parser->next.type;
}

// Consumes the current token if its type is [expected]. Returns true if a
// token was consumed.
static bool match_token(LsCompiler *compiler, TokenType expected) {
  if (peek_token(compiler) != expected)
    return false;

  next_token(compiler->parser);
  return true;
}

LsObj *ls_compile(LsVM *vm, const char *source) {
  // Foo.
}
