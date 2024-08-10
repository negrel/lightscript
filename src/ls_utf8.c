#include "ls_utf8.h"
#include "ls_utils.h"

size_t ls_utf8_encode_bytes_len(int32_t value) {
  if (value <= 0x7f)
    return 1;
  if (value <= 0x7ff)
    return 2;
  if (value <= 0xffff)
    return 3;
  if (value <= 0x10ffff)
    return 4;
  return 0;
}

size_t ls_utf8_encode(int32_t value, uint8_t *bytes) {
  if (value <= 0x7f) {
    // Single byte (i.e. fits in ASCII).
    *bytes = value & 0x7f;
    return 1;
  } else if (value <= 0x7ff) {
    // Two byte sequence: 110xxxxx 10xxxxxx.
    *bytes = 0xc0 | ((value & 0x7c0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 2;
  } else if (value <= 0xffff) {
    // Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
    *bytes = 0xe0 | ((value & 0xf000) >> 12);
    bytes++;
    *bytes = 0x80 | ((value & 0xfc0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 3;
  } else if (value <= 0x10ffff) {
    // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    *bytes = 0xf0 | ((value & 0x1c0000) >> 18);
    bytes++;
    *bytes = 0x80 | ((value & 0x3f000) >> 12);
    bytes++;
    *bytes = 0x80 | ((value & 0xfc0) >> 6);
    bytes++;
    *bytes = 0x80 | (value & 0x3f);
    return 4;
  }

  // Invalid Unicode value. See: http://tools.ietf.org/html/rfc3629
  UNREACHABLE();
  return 0;
}
