#include "storage.h"

/*
 * storage.c - Atomic file serialization and load implementation including Feature F2 History.
 */

/* Time: O(n + q + h + hist), Space: O(1) */
Status storage_save(const SystemContext *ctx, const char *filepath) {
    if (!ctx || !filepath) return ST_ERR_INVALID;

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", filepath);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        return ST_ERR_IO;
    }

    /* 1. System Counters */
    fprintf(f, "[SYSTEM]\n");
    fprintf(f, "%d|%d|%d|%d\n",
            ctx->next_donor_id, ctx->next_request_id, ctx->next_sequence, ctx->fulfilled_count);

    /* 2. Donors Section (in master list order) */
    fprintf(f, "[DONORS]\n");
    const Donor *curr_d = ctx->donor_list_head;
    while (curr_d) {
        char d_str[16];
        date_format(curr_d->last_donation, d_str, sizeof(d_str));
        fprintf(f, "%d|%s|%d|%c|%d|%s|%s|%s|%d\n",
                curr_d->id,
                curr_d->name,
                curr_d->age,
                curr_d->gender,
                (int)curr_d->blood_group,
                curr_d->phone,
                curr_d->city,
                date_is_never(curr_d->last_donation) ? "never" : d_str,
                curr_d->available);
        curr_d = curr_d->next;
    }

    /* 3. Normal Requests Section (FIFO Queue) */
    fprintf(f, "[NORMAL_QUEUE]\n");
    const QueueNode *curr_q = ctx->normal_queue.front;
    while (curr_q) {
        const BloodRequest *r = &curr_q->request;
        char d_str[16];
        date_format(r->request_date, d_str, sizeof(d_str));
        fprintf(f, "%d|%s|%d|%d|%s|%s|%d|%d|%s\n",
                r->id, r->patient_name, (int)r->blood_group, r->units,
                r->hospital, r->city, (int)r->urgency, r->arrival_sequence, d_str);
        curr_q = curr_q->next;
    }

    /* 4. Emergency Requests Section (Heap) */
    fprintf(f, "[EMERGENCY_HEAP]\n");
    for (int i = 0; i < ctx->emergency_heap.size; i++) {
        const BloodRequest *r = &ctx->emergency_heap.elements[i];
        char d_str[16];
        date_format(r->request_date, d_str, sizeof(d_str));
        fprintf(f, "%d|%s|%d|%d|%s|%s|%d|%d|%s\n",
                r->id, r->patient_name, (int)r->blood_group, r->units,
                r->hospital, r->city, (int)r->urgency, r->arrival_sequence, d_str);
    }

    /* 5. Fulfilment History Section (Doubly Linked List) */
    fprintf(f, "[FULFILMENT_HISTORY]\n");
    const HistoryNode *curr_h = ctx->history.head;
    while (curr_h) {
        const FulfilmentRecord *r = &curr_h->record;
        char d_str[16];
        date_format(r->fulfillment_date, d_str, sizeof(d_str));
        fprintf(f, "%d|%s|%d|%d|%s|%s|%s",
                r->request_id, r->patient_name, (int)r->blood_group, r->units,
                r->hospital, r->patient_city, d_str);
        for (int i = 0; i < r->units; i++) {
            fprintf(f, "|%d|%s|%s|%d",
                    r->donor_ids[i], r->donor_names[i], r->donor_cities[i], r->donor_distances_km[i]);
        }
        fprintf(f, "\n");
        curr_h = curr_h->next;
    }

    fprintf(f, "[END]\n");
    fflush(f);
    fclose(f);

    /* Atomic Rename */
    remove(filepath);
    if (rename(tmp_path, filepath) != 0) {
        remove(tmp_path);
        return ST_ERR_IO;
    }

    return ST_OK;
}

/* Time: O(lines), Space: O(n + hist) heap memory */
Status storage_load(SystemContext *ctx, const char *filepath) {
    if (!ctx || !filepath) return ST_ERR_INVALID;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        return ST_ERR_NOT_FOUND;
    }

    SystemContext temp;
    context_init(&temp);

    char line[1024];
    int section = 0; /* 0=None, 1=System, 2=Donors, 3=NormalQueue, 4=EmergencyHeap, 5=History */
    Date today = date_get_today();
    int parse_error = 0;

    while (fgets(line, sizeof(line), f)) {
        utils_trim_whitespace(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        if (strcmp(line, "[SYSTEM]") == 0) {
            section = 1;
            continue;
        } else if (strcmp(line, "[DONORS]") == 0) {
            section = 2;
            continue;
        } else if (strcmp(line, "[NORMAL_QUEUE]") == 0) {
            section = 3;
            continue;
        } else if (strcmp(line, "[EMERGENCY_HEAP]") == 0) {
            section = 4;
            continue;
        } else if (strcmp(line, "[FULFILMENT_HISTORY]") == 0) {
            section = 5;
            continue;
        } else if (strcmp(line, "[END]") == 0) {
            break;
        }

        if (section == 1) {
            int next_d, next_r, next_s, fulf;
            if (sscanf(line, "%d|%d|%d|%d", &next_d, &next_r, &next_s, &fulf) == 4) {
                temp.next_donor_id = next_d;
                temp.next_request_id = next_r;
                temp.next_sequence = next_s;
                temp.fulfilled_count = fulf;
            } else {
                parse_error = 1;
                break;
            }
        } else if (section == 2) {
            char name[MAX_NAME_LEN], phone[MAX_PHONE_LEN], city[MAX_CITY_LEN], date_str[32];
            int id, age, bg_int, avail;
            char gender;

            char *tok = strtok(line, "|");
            if (!tok) { parse_error = 1; break; }
            id = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(name, tok, sizeof(name));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            age = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            gender = tok[0];

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            bg_int = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(phone, tok, sizeof(phone));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(city, tok, sizeof(city));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(date_str, tok, sizeof(date_str));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            avail = atoi(tok);

            Date last_don;
            if (date_parse_user(date_str, &last_don, today) != ST_OK) {
                parse_error = 1;
                break;
            }

            BloodGroup bg = (BloodGroup)bg_int;
            if (donor_validate_fields(name, age, gender, bg, phone, city, last_don, avail, today) != ST_OK) {
                parse_error = 1;
                break;
            }

            Donor *d = NULL;
            if (donor_new(&d, id, name, age, gender, bg, phone, city, last_don, avail) != ST_OK) {
                parse_error = 1;
                break;
            }

            donor_list_append(&temp.donor_list_head, &temp.donor_list_tail, d);
            bst_insert(&temp.donor_id_bst, d);
            ht_insert(&temp.phone_table, d);
            buckets_insert(&temp.bg_buckets, d);
            temp.donor_count++;
        } else if (section == 3 || section == 4) {
            int id, bg_int, units, urg_int, seq;
            char pat_name[MAX_NAME_LEN], hosp[MAX_HOSPITAL_LEN], city[MAX_CITY_LEN], date_str[32];

            char *tok = strtok(line, "|");
            if (!tok) { parse_error = 1; break; }
            id = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(pat_name, tok, sizeof(pat_name));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            bg_int = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            units = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(hosp, tok, sizeof(hosp));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(city, tok, sizeof(city));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            urg_int = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            seq = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(date_str, tok, sizeof(date_str));

            Date r_date;
            date_parse_user(date_str, &r_date, today);

            BloodRequest req;
            request_init(&req, id, pat_name, (BloodGroup)bg_int, units, hosp, city, (UrgencyLevel)urg_int, seq, r_date);

            if (section == 3) {
                queue_enqueue(&temp.normal_queue, &req);
            } else {
                heap_insert(&temp.emergency_heap, &req);
            }
        } else if (section == 5) {
            /* Fulfilment Record */
            FulfilmentRecord f_rec;
            char *tok = strtok(line, "|");
            if (!tok) { parse_error = 1; break; }
            f_rec.request_id = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(f_rec.patient_name, tok, sizeof(f_rec.patient_name));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            f_rec.blood_group = (BloodGroup)atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            f_rec.units = atoi(tok);

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(f_rec.hospital, tok, sizeof(f_rec.hospital));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            utils_copy_string(f_rec.patient_city, tok, sizeof(f_rec.patient_city));

            tok = strtok(NULL, "|");
            if (!tok) { parse_error = 1; break; }
            date_parse_user(tok, &f_rec.fulfillment_date, today);

            for (int i = 0; i < f_rec.units; i++) {
                tok = strtok(NULL, "|");
                if (!tok) break;
                f_rec.donor_ids[i] = atoi(tok);

                tok = strtok(NULL, "|");
                if (!tok) break;
                utils_copy_string(f_rec.donor_names[i], tok, sizeof(f_rec.donor_names[i]));

                tok = strtok(NULL, "|");
                if (!tok) break;
                utils_copy_string(f_rec.donor_cities[i], tok, sizeof(f_rec.donor_cities[i]));

                tok = strtok(NULL, "|");
                if (!tok) break;
                f_rec.donor_distances_km[i] = atoi(tok);
            }

            history_append(&temp.history, &f_rec);
        }
    }

    fclose(f);

    if (parse_error) {
        context_destroy(&temp);
        return ST_ERR_FORMAT;
    }

    context_destroy(ctx);
    *ctx = temp;

    return ST_OK;
}
