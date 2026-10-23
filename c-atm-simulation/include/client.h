/**
 * include/client.h
 * Remote client implementation of BankApi using ATMP/1 socket transport (Section 4.3, F8).
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "bank_api.h"
#include <stdbool.h>

BankApi bank_api_create_remote(const char *host, int port);

#endif /* CLIENT_H */
