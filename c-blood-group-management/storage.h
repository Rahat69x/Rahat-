#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"
#include "context.h"

/*
 * storage.h - Human-readable file persistence and atomic serialization.
 * Layer 5 module: coordinates saving and loading SystemContext state.
 * Hard constraints:
 *  - Atomic write via temporary file (bgms_data.tmp -> rename to bgms_data.txt).
 *  - Robust line reading and field validation on every record during load.
 *  - Clean error reporting on corrupted or partial files without leaking memory.
 */

#define DEFAULT_DATA_FILE "bgms_data.txt"

/*
 * Atomically saves the complete SystemContext to disk.
 * Writes to a temporary file, flushes, closes, and renames to target filepath.
 * Returns ST_OK on success, ST_ERR_IO on write failure.
 */
Status storage_save(const SystemContext *ctx, const char *filepath);

/*
 * Loads the complete SystemContext from disk.
 * Clears existing context only after file header and initial validation succeed.
 * Reconstructs all 6 structures: master list, BST ID index, Phone hash table,
 * classified buckets, normal FIFO queue, and emergency heap.
 * Returns ST_OK on success, ST_ERR_NOT_FOUND if file doesn't exist,
 * ST_ERR_FORMAT on syntax/corruption error.
 */
Status storage_load(SystemContext *ctx, const char *filepath);

#endif /* STORAGE_H */
