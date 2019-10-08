#ifndef INCLUDE_STRING_H_
#define INCLUDE_STRING_H_

#include "type.h"

int strlen(char *src);
int strcmp(char *s, char *t);
void strcpy(char *dest, char *src);
void memcpy(uint8_t *dest, uint8_t *src, uint32_t len);
int separate(char *s, char separator);

#endif