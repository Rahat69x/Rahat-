#ifndef BST_H
#define BST_H

#include "common.h"
#include "donor.h"

/*
 * bst.h - Binary Search Tree index on Donor ID.
 * Layer 3 structure module: does NOT own Donor memory.
 * BSTNode holds a pointer to Donor; freeing a BSTNode does NOT free the Donor.
 * Never includes context.h.
 */

typedef struct BSTNode {
    int id;                 /* Key: Donor unique ID */
    Donor *donor;           /* Pointer to Donor owned by master linked list */
    struct BSTNode *left;   /* Left child (keys < id) */
    struct BSTNode *right;  /* Right child (keys > id) */
} BSTNode;

/* Visitor callback type for in-order traversal */
typedef void (*BSTVisitor)(const Donor *donor, void *user_data);

/*
 * Inserts a donor pointer into the BST keyed by donor->id.
 * Preallocates node. Returns ST_OK on success, ST_ERR_NOMEM if allocation fails,
 * ST_ERR_DUPLICATE if ID already exists, ST_ERR_INVALID on bad arguments.
 */
Status bst_insert(BSTNode **root, Donor *donor);

/*
 * Searches the BST for a donor with the specified ID.
 * Returns pointer to Donor if found, or NULL if not found.
 * Big-O: Time O(log n) average, O(n) worst-case.
 */
Donor *bst_search(const BSTNode *root, int id);

/*
 * Deletes the node with key `id` from the BST.
 * Rebalances/relinks BST structure (standard 2-child in-order successor replacement).
 * Does NOT free the Donor node.
 * Returns ST_OK if deleted, ST_ERR_NOT_FOUND if ID not present.
 */
Status bst_delete(BSTNode **root, int id);

/*
 * In-order traversal of the BST (Left, Root, Right).
 * Invokes visitor callback on each donor in ascending order of ID.
 * Big-O: Time O(n), Space O(h) recursion stack.
 */
void bst_inorder(const BSTNode *root, BSTVisitor visitor, void *user_data);

/*
 * Counts total nodes in the BST recursively.
 */
int bst_count(const BSTNode *root);

/*
 * Deallocates all BSTNode instances in the tree and sets *root to NULL.
 * Does NOT deallocate the underlying Donor records.
 */
void bst_free_all(BSTNode **root);

#endif /* BST_H */
