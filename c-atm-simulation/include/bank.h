/**
 * include/bank.h
 * Root state management and demo account seeding (Section 4.1).
 */

#ifndef BANK_H
#define BANK_H

#include "types.h"
#include <stdbool.h>

void bank_init(Bank *bank);
void bank_seed_demo(Bank *bank);
void bank_destroy(Bank *bank);

#endif /* BANK_H */
