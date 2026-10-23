/**
 * include/admin.h
 * Administrator credentials, role checks, and administrative actions (F3).
 */

#ifndef ADMIN_H
#define ADMIN_H

#include "types.h"
#include <stdbool.h>

void admin_seed_default(Bank *bank);
Admin *admin_find(Bank *bank, const char *admin_id);
bool admin_authenticate(Bank *bank, const char *admin_id, const char *passphrase,
                        Admin **out_admin, char *err_msg, size_t err_len);
bool admin_change_passphrase(Bank *bank, Admin *adm, const char *new_passphrase);
bool admin_unlock(Bank *bank, const char *super_id, const char *target_id);
bool admin_create_admin(Bank *bank, const char *super_id, const char *new_id,
                        const char *name, AdminRole role, const char *initial_pass);

#endif /* ADMIN_H */
