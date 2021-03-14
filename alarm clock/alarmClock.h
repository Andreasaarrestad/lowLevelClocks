#ifndef O3_H
#define O3_H

// Type-definisjoner fra std-bibliotekene
#include <stdint.h>
#include <stdbool.h>

// Type-aliaser
typedef uint32_t word;
typedef uint8_t  byte;

// Prototyper for bibliotekfunksjoner
void init(void);
void lcd_write(char* string);
void int_to_string(char *timestamp, unsigned int offset, int i);
void time_to_string(char *timestamp, int h, int m, int s);

// Prototyper
// legg prototyper for dine funksjoner her

#endif
