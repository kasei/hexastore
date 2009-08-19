#ifndef _BTREE_INTERNAL_H
#define _BTREE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
// #define BRANCHING_SIZE	124
// #define BRANCHING_SIZE	252
// #define BRANCHING_SIZE	510
// #define BRANCHING_SIZE	1022
// #define BRANCHING_SIZE	14
// #define BRANCHING_SIZE	4
*/

typedef enum {
	HX_BTREE_NODE_ROOT	= 1,
	HX_BTREE_NODE_LEAF	= 2
} hx_btree_node_type;

#define hx_btree_node_has_flag(node,flag)	(((node->flags & flag) > 0) ? 1 : 0)

hx_btree_node* hx_new_btree_root ( uint32_t branching_size );
hx_btree_node* hx_new_btree_node ( uint32_t branching_size );
int hx_free_btree_node ( hx_btree_node* node );

list_size_t hx_btree_size ( hx_btree* tree );

int hx_btree_node_debug ( char* string, hx_btree_node* node, uint32_t branching_size );
int hx_btree_tree_debug ( char* string, hx_btree_node* node, uint32_t branching_size );
int hx_btree_node_add_child ( hx_btree_node* node, hx_node_id n, uintptr_t child, uint32_t branching_size );
uintptr_t hx_btree_node_get_child ( hx_btree_node* node, hx_node_id n, uint32_t branching_size );
int hx_btree_node_remove_child ( hx_btree_node* node, hx_node_id n, uint32_t branching_size );

/* setter methods */
hx_btree_node* hx_btree_node_next_neighbor ( hx_btree_node* node );
hx_btree_node* hx_btree_node_prev_neighbor ( hx_btree_node* node );
int hx_btree_node_set_parent ( hx_btree_node* node, hx_btree_node* parent );
int hx_btree_node_set_next_neighbor ( hx_btree_node* node, hx_btree_node* next );
int hx_btree_node_set_prev_neighbor ( hx_btree_node* node, hx_btree_node* prev );


int hx_btree_node_set_flag ( hx_btree_node* node, uint32_t type );
int hx_btree_node_unset_flag ( hx_btree_node* node, uint32_t type );

uintptr_t hx_btree_node_search ( hx_btree_node* root, hx_node_id key, uint32_t branching_size );
int hx_btree_node_insert ( hx_btree_node** _root, hx_node_id key, uintptr_t value, uint32_t branching_size );
int hx_btree_node_remove ( hx_btree_node** _root, hx_node_id key, uint32_t branching_size );
void hx_btree_node_traverse ( hx_btree_node* node, hx_btree_node_visitor* before, hx_btree_node_visitor* after, int level, uint32_t branching_size, void* param );

#ifdef __cplusplus
}
#endif

#endif
