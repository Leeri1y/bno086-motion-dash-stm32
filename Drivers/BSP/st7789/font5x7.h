/*
 * font5x7.h
 */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT5X7_CHARS 256  /* indexed by char code: font5x7[c*5] */
#define FONT5X7_BYTES 5

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t font5x7[FONT5X7_CHARS * FONT5X7_BYTES];

#ifdef __cplusplus
}
#endif

#endif