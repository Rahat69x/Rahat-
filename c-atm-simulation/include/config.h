/**
 * include/config.h
 * Hand-parsed key-value configuration loader for config.ini (Section 3).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include "types.h"

void config_set_defaults(Config *cfg);
bool config_load(const char *filepath, Config *cfg);
bool config_save(const char *filepath, const Config *cfg);

#endif /* CONFIG_H */
