#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

// NaN tagged value.
typedef double Value;

// A mask that selects the sign bit.
#define SIGN_BIT ((uint64_t)1 << 63)

// The bits that must be set to indicate a quiet NaN.
#define QNAN ((uint64_t)0x7ffc000000000000)

// Masks out the tag bits used to identify the singleton value.
#define MASK_TAG (7)

// Tag values for the different singleton values.
#define LS_TAG_NAN (0)
#define LS_TAG_NULL (1)
#define LS_TAG_FALSE (2)
#define LS_TAG_TRUE (3)

#define LS_NULL ((Value)(uint64_t)(QNAN | LS_TAG_NULL))
#define LS_FALSE ((Value)(uint64_t)(QNAN | LS_TAG_FALSE))
#define LS_TRUE ((Value)(uint64_t)(QNAN | LS_TAG_TRUE))

#endif
