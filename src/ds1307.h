#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

typedef struct {
  int sec, min, hour;
  int day, date, month, year;
} ds1307_time_t;

void ds1307_init(void);

void ds1307_set_time(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day,
                     uint8_t date, uint8_t month, uint8_t year);

// Estructura
void ds1307_get_time(ds1307_time_t *time);

#endif