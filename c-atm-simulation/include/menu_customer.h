/**
 * include/menu_customer.h
 * Customer terminal interaction menu driven strictly by BankApi (Section 4.4).
 */

#ifndef MENU_CUSTOMER_H
#define MENU_CUSTOMER_H

#include "bank_api.h"

void menu_customer_run(BankApi *api, const Config *cfg);

#endif /* MENU_CUSTOMER_H */
