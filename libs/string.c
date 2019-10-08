#include "string.h"

int strlen(char *s)
{
	int i;
	for (i = 0; s[i]; ++i)
		;
	return i;
}

int strcmp(char *s, char *t)
{
    int i = 0;
    while(s[i] == t[i] && s[i])
        ++i;
    return s[i]-t[i];
}

void strcpy(char *dest, char *src)
{
	while (*src)
	{
		*dest = *src;
		++dest;
		++src;
	}

	*dest = '\0';
}

void memset(void *dest, uint8_t val, uint32_t len)
{
	uint8_t *dst = (uint8_t *)dest;

	for (; len != 0; len--)
	{
		*dst++ = val;
	}
}

int separate(char *s, char separator)
{
    // find the first apprearance of separator, return the index
	// if no, return -1
	// '\0' is not supported
    int i;
    for(i = 0; s[i] && s[i] != separator; ++i)
        ;
    return s[i] ? i : -1;
}