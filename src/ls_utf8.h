#ifndef LS_UTF8_H_INCLUDE
#define LS_UTF8_H_INCLUDE

#include <stddef.h>
#include <stdint.h>

// Returns the number of bytes needed to encode [value] in UTF-8.
//
// Returns 0 if [value] is too large to encode.
size_t ls_utf8_encode_bytes_len(int value);

// Encodes value as a series of bytes in [bytes], which is assumed to be large
// enough to hold the encoded result.
//
// Returns the number of written bytes.
size_t ls_utf8_encode(int value, uint8_t *bytes);

#endif
