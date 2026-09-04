#ifndef _DS3231_MISC_H
#define _DS3231_MISC_H

struct ds3231_t;

int ds3231_misc_register(struct ds3231_t *data);
void ds3231_misc_unregister(struct ds3231_t *data);

#endif
