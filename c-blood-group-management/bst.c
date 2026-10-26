#include "bst.h"

/*
 * bst.c - Binary Search Tree implementation for Donor ID indexing.
 */

/* Time: O(1), Space: O(1) */
static BSTNode *bst_create_node(Donor *donor) {
    if (!donor) return NULL;
    BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) return NULL;
    node->id = donor->id;
    node->donor = donor;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/* Time: O(log n) avg, O(n) worst; Space: O(h) recursion stack */
static Status bst_insert_recursive(BSTNode **node_ptr, Donor *donor, BSTNode *preallocated) {
    if (*node_ptr == NULL) {
        *node_ptr = preallocated;
        return ST_OK;
    }

    if (donor->id < (*node_ptr)->id) {
        return bst_insert_recursive(&((*node_ptr)->left), donor, preallocated);
    } else if (donor->id > (*node_ptr)->id) {
        return bst_insert_recursive(&((*node_ptr)->right), donor, preallocated);
    } else {
        /* Duplicate ID not permitted */
        return ST_ERR_DUPLICATE;
    }
}

/* Time: O(log n) avg, O(n) worst; Space: O(h) stack */
Status bst_insert(BSTNode **root, Donor *donor) {
    if (!root || !donor) {
        return ST_ERR_INVALID;
    }

    /* Prepare phase: preallocate node up front */
    BSTNode *node = bst_create_node(donor);
    if (!node) {
        return ST_ERR_NOMEM;
    }

    /* Commit phase: insert into tree */
    Status st = bst_insert_recursive(root, donor, node);
    if (st != ST_OK) {
        /* Rollback: free preallocated node if key was duplicate */
        free(node);
        return st;
    }

    return ST_OK;
}

/* Time: O(log n) avg, O(n) worst; Space: O(1) iterative */
Donor *bst_search(const BSTNode *root, int id) {
    const BSTNode *curr = root;
    while (curr) {
        if (id == curr->id) {
            return curr->donor;
        } else if (id < curr->id) {
            curr = curr->left;
        } else {
            curr = curr->right;
        }
    }
    return NULL;
}

/* Time: O(h), Space: O(1) */
static BSTNode *bst_find_min(BSTNode *root) {
    BSTNode *curr = root;
    while (curr && curr->left) {
        curr = curr->left;
    }
    return curr;
}

/* Time: O(log n) avg, O(n) worst; Space: O(h) recursion stack */
static Status bst_delete_recursive(BSTNode **node_ptr, int id) {
    if (*node_ptr == NULL) {
        return ST_ERR_NOT_FOUND;
    }

    if (id < (*node_ptr)->id) {
        return bst_delete_recursive(&((*node_ptr)->left), id);
    } else if (id > (*node_ptr)->id) {
        return bst_delete_recursive(&((*node_ptr)->right), id);
    } else {
        /* Node found - perform deletion */
        BSTNode *target = *node_ptr;

        /* Case 1: Leaf node (no children) */
        if (target->left == NULL && target->right == NULL) {
            free(target);
            *node_ptr = NULL;
            return ST_OK;
        }
        /* Case 2: One child (right only) */
        else if (target->left == NULL) {
            *node_ptr = target->right;
            free(target);
            return ST_OK;
        }
        /* Case 2: One child (left only) */
        else if (target->right == NULL) {
            *node_ptr = target->left;
            free(target);
            return ST_OK;
        }
        /* Case 3: Two children */
        else {
            /* In-order successor: minimum node in right subtree */
            BSTNode *successor = bst_find_min(target->right);
            target->id = successor->id;
            target->donor = successor->donor;
            /* Delete successor node from right subtree */
            return bst_delete_recursive(&(target->right), successor->id);
        }
    }
}

/* Time: O(log n) avg, O(n) worst; Space: O(h) recursion stack */
Status bst_delete(BSTNode **root, int id) {
    if (!root || !*root || id <= 0) {
        return ST_ERR_NOT_FOUND;
    }
    return bst_delete_recursive(root, id);
}

/* Time: O(n), Space: O(h) recursion stack */
void bst_inorder(const BSTNode *root, BSTVisitor visitor, void *user_data) {
    if (!root || !visitor) return;
    bst_inorder(root->left, visitor, user_data);
    visitor(root->donor, user_data);
    bst_inorder(root->right, visitor, user_data);
}

/* Time: O(n), Space: O(h) recursion stack */
int bst_count(const BSTNode *root) {
    if (!root) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}

/* Time: O(n), Space: O(h) recursion stack (post-order traversal) */
void bst_free_all(BSTNode **root) {
    if (!root || !*root) return;
    bst_free_all(&((*root)->left));
    bst_free_all(&((*root)->right));
    free(*root);
    *root = NULL;
}
