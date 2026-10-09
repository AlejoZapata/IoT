#ifndef DS1307_H
#define DS1307_H

/**
 * @file ds1307.h
 * @brief Interfaz para inicializar y acceder al reloj de tiempo real DS1307.
 */

#include <stdint.h>

/** @brief Fecha y hora leidas del DS1307. */
typedef struct {
  int sec;   /**< Segundos, de 0 a 59. */
  int min;   /**< Minutos, de 0 a 59. */
  int hour;  /**< Hora en formato de 24 horas, de 0 a 23. */
  int day;   /**< Dia de la semana, segun el valor almacenado en el RTC. */
  int date;  /**< Dia del mes, de 1 a 31. */
  int month; /**< Mes, de 1 a 12. */
  int year;  /**< Ano de dos digitos, de 0 a 99. */
} ds1307_time_t;

/**
 * @brief Configura e instala el bus I2C usado por el DS1307.
 *
 * Usa I2C_NUM_0, SDA en GPIO 8, SCL en GPIO 9 y una frecuencia de 100 kHz.
 * Esta funcion no devuelve el estado de las operaciones I2C.
 */
void ds1307_init(void);

/**
 * @brief Escribe la fecha y hora en el DS1307.
 *
 * Los valores se convierten a BCD antes de escribirlos. El ano se almacena
 * como dos digitos y el dia de la semana usa el valor especificado por el
 * llamador.
 *
 * @param sec Segundos (0-59).
 * @param min Minutos (0-59).
 * @param hour Hora en formato de 24 horas (0-23).
 * @param day Dia de la semana segun la convencion de la aplicacion.
 * @param date Dia del mes (1-31).
 * @param month Mes (1-12).
 * @param year Ano de dos digitos (0-99).
 * @note La funcion no devuelve el estado de la escritura I2C.
 */
void ds1307_set_time(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day,
                     uint8_t date, uint8_t month, uint8_t year);

/**
 * @brief Lee los registros de fecha y hora del DS1307.
 *
 * Decodifica los valores BCD y los guarda en @p time. La funcion presupone
 * que la lectura I2C se completa correctamente y no informa errores de bus.
 *
 * @param[out] time Estructura donde se guardaran los valores; no puede ser NULL.
 */
void ds1307_get_time(ds1307_time_t *time);

#endif