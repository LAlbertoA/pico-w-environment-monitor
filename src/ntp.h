#ifndef NTP_H
#define NTP_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

void ntp_init(void);
void ntp_poll(void);
bool ntp_hast_time(void);
time_t ntp_get_epoch(void);
void ntp_get_time_str(char *buf, size_t n);
void ntp_get_date_str(char *buf, size_t n);
void ntp_get_datetime_str(char *buf, size_t n);

#endif