/**
 * src/features/admin.c
 * Administrator management, authentication, role enforcement (F3).
 */

#include "admin.h"
#include "pin.h"
#include "audit.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

void admin_seed_default(Bank *bank) {
    if (!bank || bank->admin_count > 0) return;

    Admin *adm = &bank->admins[bank->admin_count++];
    memset(adm, 0, sizeof(Admin));
    snprintf(adm->id, sizeof(adm->id), "ADMIN001");
    snprintf(adm->name, sizeof(adm->name), "System Superadmin");
    adm->role = ROLE_SUPERADMIN;
    adm->is_locked = false;
    adm->failed_attempts = 0;
    adm->must_change_pass = true;

    pin_generate_salt(adm->passphrase_salt);
    pin_hash_derive("admin123", adm->passphrase_salt, 10000, adm->passphrase_hash);
}

Admin *admin_find(Bank *bank, const char *admin_id) {
    if (!bank || !admin_id) return NULL;
    for (size_t i = 0; i < bank->admin_count; ++i) {
        if (strcmp(bank->admins[i].id, admin_id) == 0) {
            return &bank->admins[i];
        }
    }
    return NULL;
}

bool admin_authenticate(Bank *bank, const char *admin_id, const char *passphrase,
                        Admin **out_admin, char *err_msg, size_t err_len) {
    if (!bank || !admin_id || !passphrase) return false;
    Admin *adm = admin_find(bank, admin_id);
    if (!adm) {
        if (err_msg) snprintf(err_msg, err_len, "Admin ID not recognized");
        return false;
    }
    if (adm->is_locked) {
        if (err_msg) snprintf(err_msg, err_len, "Admin account is LOCKED due to excessive failed attempts");
        return false;
    }

    if (!pin_verify(passphrase, adm->passphrase_salt, 10000, adm->passphrase_hash)) {
        adm->failed_attempts++;
        if (adm->failed_attempts >= bank->config.admin_max_tries) {
            adm->is_locked = true;
            if (err_msg) snprintf(err_msg, err_len, "Admin account has been LOCKED. Contact SUPERADMIN.");
        } else {
            if (err_msg) snprintf(err_msg, err_len, "Invalid passphrase (%d/%d attempts)",
                                  adm->failed_attempts, bank->config.admin_max_tries);
        }
        return false;
    }

    adm->failed_attempts = 0;
    if (out_admin) *out_admin = adm;
    return true;
}

bool admin_change_passphrase(Bank *bank, Admin *adm, const char *new_passphrase) {
    (void)bank;
    if (!adm || !new_passphrase || strlen(new_passphrase) < 6) return false;
    pin_generate_salt(adm->passphrase_salt);
    pin_hash_derive(new_passphrase, adm->passphrase_salt, 10000, adm->passphrase_hash);
    adm->must_change_pass = false;
    return true;
}

bool admin_unlock(Bank *bank, const char *super_id, const char *target_id) {
    Admin *super = admin_find(bank, super_id);
    if (!super || super->role != ROLE_SUPERADMIN) return false;
    Admin *target = admin_find(bank, target_id);
    if (!target) return false;
    target->is_locked = false;
    target->failed_attempts = 0;
    return true;
}

bool admin_create_admin(Bank *bank, const char *super_id, const char *new_id,
                        const char *name, AdminRole role, const char *initial_pass) {
    if (!bank || bank->admin_count >= MAX_ADMINS) return false;
    Admin *super = admin_find(bank, super_id);
    if (!super || super->role != ROLE_SUPERADMIN) return false;
    if (admin_find(bank, new_id) != NULL) return false;

    Admin *adm = &bank->admins[bank->admin_count++];
    memset(adm, 0, sizeof(Admin));
    snprintf(adm->id, sizeof(adm->id), "%s", new_id);
    snprintf(adm->name, sizeof(adm->name), "%s", name);
    adm->role = role;
    adm->is_locked = false;
    adm->must_change_pass = true;

    pin_generate_salt(adm->passphrase_salt);
    pin_hash_derive(initial_pass, adm->passphrase_salt, 10000, adm->passphrase_hash);
    return true;
}
