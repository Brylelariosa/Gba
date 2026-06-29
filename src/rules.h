#ifndef RULES_H
#define RULES_H

#include "simulation.h"

extern const char *RULE_NAMES[AUTO_COUNT];
extern const char *RULE_DESC[AUTO_COUNT];

/* Returns the B/S string for life-like rules */
void rule_get_bs_string(char *buf, int bufsize);

#endif