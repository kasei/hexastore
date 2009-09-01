#include "store/hexastore/btree.h"
#include <assert.h>

#include "store/hexastore/btree_internal.h"

void _hx_btree_debug_leaves_visitor (hx_btree_node* node, int level, uint32_t branching_size, void* param);
void _hx_btree_debug_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param );
void _hx_btree_free_node_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param );

void _hx_btree_count ( hx_btree_node* node, int level, uint32_t branching_size, void* param );
int _hx_btree_binary_search ( const hx_btree_node* node, const hx_node_id n, int* index );
int _hx_btree_node_insert_nonfull( hx_btree_node* root, hx_node_id key, uintptr_t value, uint32_t branching_size );
int _hx_btree_node_split_child( hx_btree_node* parent, uint32_t index, hx_btree_node* child, uint32_t branching_size );
int _hx_btree_iter_prime ( hx_btree_iter* iter );
hx_btree_node* _hx_btree_node_search_page ( hx_btree_node* root, hx_node_id key, uint32_t branching_size );
int _hx_btree_rebalance( hx_btree_node* node, hx_btree_node* from, int dir, uint32_t branching_size );
int _hx_btree_merge_nodes( hx_btree_node* a, hx_btree_node* b, uint32_t branching_size );
void _hx_btree_node_reset_keys( hx_btree_node* parent );

hx_btree* hx_new_btree ( void* world, uint32_t branching_size ) {
	hx_btree* tree			= (hx_btree*) calloc( 1, sizeof( hx_btree )  );
	if (tree == NULL) {
		fprintf( stderr, "*** Failed to allocate memory for btree in hx_new_btree\n" );
	}
	tree->root				= ((uintptr_t) hx_new_btree_root( branching_size ));
	tree->branching_size	= branching_size;
	// sync(tree)
	return tree;
}

int hx_free_btree ( hx_btree* tree ) {
	hx_btree_node_traverse( (hx_btree_node*) tree->root, NULL, _hx_btree_free_node_visitor, 0, tree->branching_size, NULL );
	free( tree );
	return 0;
}

uintptr_t hx_btree_search ( hx_btree* tree, hx_node_id key ) {
	return hx_btree_node_search( (hx_btree_node*) tree->root, key, tree->branching_size );
}

int hx_btree_insert ( hx_btree* tree, hx_node_id key, uintptr_t value ) {
	hx_btree_node* root	= (hx_btree_node*) tree->root;
	int r	= hx_btree_node_insert( &( root ), key, value, tree->branching_size );
	tree->root	= ((uintptr_t) root);
	return r;
}

int hx_btree_remove ( hx_btree* tree, hx_node_id key ) {
	hx_btree_node* root	= (hx_btree_node*) tree->root;
	int r	= hx_btree_node_remove( &( root ), key, tree->branching_size );
	tree->root	= ((uintptr_t) root);
	return r;
}

void hx_btree_traverse ( hx_btree* tree, hx_btree_node_visitor* before, hx_btree_node_visitor* after, int level, void* param ) {
	hx_btree_node_traverse( (hx_btree_node*) tree->root, before, after, level, tree->branching_size, param );
}

////////////////////////////////////////////////////////////////////////////////

hx_btree_node* hx_new_btree_root ( uint32_t branching_size ) {
	hx_btree_node* root	= hx_new_btree_node( branching_size );
	hx_btree_node_set_flag( root, HX_BTREE_NODE_ROOT );
	hx_btree_node_set_flag( root, HX_BTREE_NODE_LEAF );
	// sync(root)
	return root;
}

hx_btree_node* hx_new_btree_node ( uint32_t branching_size ) {
	hx_btree_node* node	= (hx_btree_node*) calloc( 1, sizeof( hx_btree_node ) + (branching_size * sizeof( hx_btree_child )) );
	if (node == NULL) {
		fprintf( stderr, "*** Failed to allocate memory for btree node in hx_new_btree_node\n" );
	}
	memcpy( &( node->type ), "HXBN", 4 );
	node->used	= (uint32_t) 0;
	// sync(node)
	return node;
}

int hx_free_btree_node ( hx_btree_node* node ) {
	free( node );
	return 0;
}

list_size_t hx_btree_size ( hx_btree* tree ) {
	list_size_t count	= 0;
	hx_btree_node_traverse( (hx_btree_node*) tree->root, _hx_btree_count, NULL, 0, tree->branching_size, &count );
	return count;
}

int hx_btree_node_set_parent ( hx_btree_node* node, hx_btree_node* parent ) {
	uintptr_t id	= ((uintptr_t) parent);
	node->parent	= id;
	return 0;
}

int hx_btree_node_set_prev_neighbor ( hx_btree_node* node, hx_btree_node* prev ) {
	node->prev	= ((uintptr_t) prev);
	return 0;
}

int hx_btree_node_set_next_neighbor ( hx_btree_node* node, hx_btree_node* next ) {
	node->next	= ((uintptr_t) next);
	return 0;
}

hx_btree_node* hx_btree_node_next_neighbor ( hx_btree_node* node ) {
	return (hx_btree_node*) node->next;
}

hx_btree_node* hx_btree_node_prev_neighbor ( hx_btree_node* node ) {
	return (hx_btree_node*) node->prev;
}

int hx_btree_node_set_flag ( hx_btree_node* node, uint32_t flag ) {
	node->flags	|= flag;
	return 0;
}

int hx_btree_node_unset_flag ( hx_btree_node* node, uint32_t flag ) {
	if (node->flags & flag) {
		node->flags	^= flag;
	}
	return 0;
}

int hx_btree_node_debug ( char* string, hx_btree_node* node, uint32_t branching_size ) {
	fprintf( stderr, "%sNode %d (%p):\n", string, (int) ((uintptr_t) node), (void*) node);
	fprintf( stderr, "%s\tUsed: [%d/%d]\n", string, node->used, branching_size );
	fprintf( stderr, "%s\tFlags: ", string );
	if (node->flags & HX_BTREE_NODE_ROOT)
		fprintf( stderr, "HX_BTREE_NODE_ROOT " );
	if (node->flags & HX_BTREE_NODE_LEAF)
		fprintf( stderr, "HX_BTREE_NODE_LEAF" );
	fprintf( stderr, "\n" );
	int i;
	for (i = 0; i < node->used; i++) {
		fprintf( stderr, "%s\t- %d -> %d\n", string, (int) node->ptr[i].key, (int) node->ptr[i].child );
	}
	return 0;
}

int hx_btree_tree_debug ( char* string, hx_btree_node* node, uint32_t branching_size ) {
	hx_btree_node_traverse( node, _hx_btree_debug_visitor, NULL, 0, branching_size, string );
	return 0;
}

int _hx_btree_binary_search ( const hx_btree_node* node, const hx_node_id n, int* index ) {
	int low		= 0;
	int high	= node->used - 1;
	while (low <= high) {
		int mid	= low + (high - low) / 2;
		if (node->ptr[mid].key > n) {
			high	= mid - 1;
		} else if (node->ptr[mid].key < n) {
			low	= mid + 1;
		} else {
			*index	= mid;
			return 0;
		}
	}
	*index	= low;
	return -1;
}

int hx_btree_node_add_child ( hx_btree_node* node, hx_node_id n, uintptr_t child, uint32_t branching_size ) {
	int i;
	int r	= _hx_btree_binary_search( node, n, &i );
	if (r == 0) {
		// already in list. do nothing.
		return 1;
	} else {
		// not found. need to add at index i
		if (node->used >= branching_size) {
			fprintf( stderr, "*** Cannot add child to already-full node\n" );
			return 2;
		}
		
		int k;
		for (k = node->used - 1; k >= i; k--) {
			node->ptr[ k+1 ]	= node->ptr[ k ];
// 			node->keys[k + 1]	= node->keys[k];
// 			node->children[k + 1]	= node->children[k];
		}
		
		node->ptr[i].key	= n;
		node->ptr[i].child	= child;
// 		node->keys[i]		= n;
// 		node->children[i]	= child;
		node->used++;
	}
	// sync(node)
	return 0;
}

uintptr_t hx_btree_node_get_child ( hx_btree_node* node, hx_node_id n, uint32_t branching_size ) {
	int i;
	int r	= _hx_btree_binary_search( node, n, &i );
	if (r == 0) {
		// found
		return node->ptr[ i ].child;
	} else {
		// not found. need to add at index i
		return 0;
	}
}

int hx_btree_node_remove_child ( hx_btree_node* node, hx_node_id n, uint32_t branching_size ) {
	int i;
	int r	= _hx_btree_binary_search( node, n, &i );
	if (r == 0) {
		// found
		int k;
		for (k = i; k < node->used; k++) {
			node->ptr[k]		= node->ptr[k+1];
// 			node->keys[ k ]		= node->keys[ k + 1 ];
// 			node->children[ k ]	= node->children[ k + 1 ];
		}
		node->used--;
		// sync(node)
		return 0;
	} else {
		// not found. need to add at index i
		return 1;
	}
}

uintptr_t hx_btree_node_search ( hx_btree_node* root, hx_node_id key, uint32_t branching_size ) {
	hx_btree_node* u	= _hx_btree_node_search_page( root, key, branching_size );
	int i;
	int r	= _hx_btree_binary_search( u, key, &i );
// 	fprintf( stderr, "looking for %d\n", (int) key );
// 	hx_btree_node_debug( "> ", w, u, branching_size );
	if (r == 0) {
		// found
		return u->ptr[i].child;
	} else {
		// not found
		return 0;
	}
}

hx_btree_node* _hx_btree_node_search_page ( hx_btree_node* root, hx_node_id key, uint32_t branching_size ) {
	hx_btree_node* u	= root;
	while (!hx_btree_node_has_flag(u, HX_BTREE_NODE_LEAF)) {
//		fprintf( stderr, "node is not a leaf... (flags: %x)\n", u->flags );
		int i;
		for (i = 0; i < u->used - 1; i++) {
			if (key <= u->ptr[i].key) {
				uintptr_t id	= u->ptr[i].child;
//				fprintf( stderr, "descending to child %d\n", (int) id );
				u	= (hx_btree_node*) id;
				goto NEXT;
			}
		}
//		fprintf( stderr, "decending to last child\n" );
		u	= (hx_btree_node*) u->ptr[ u->used - 1 ].child;
NEXT:	1;
	}
	
	return u;
}

int hx_btree_node_insert ( hx_btree_node** _root, hx_node_id key, uintptr_t value, uint32_t branching_size ) {
	hx_btree_node* root	= *_root;
	if (root->used == branching_size) {
		hx_btree_node* s	= hx_new_btree_node( branching_size );
		{
			hx_btree_node_set_flag( s, HX_BTREE_NODE_ROOT );
			hx_btree_node_unset_flag( root, HX_BTREE_NODE_ROOT );
			hx_btree_node_set_parent( root, s );
			hx_node_id key	= root->ptr[ branching_size - 1 ].key;
			uintptr_t rid	= ((uintptr_t) root);
			hx_btree_node_add_child( s, key, rid, branching_size );
			_hx_btree_node_split_child( s, 0, root, branching_size );
			*_root	= s;
			// sync(s)
		}
		return _hx_btree_node_insert_nonfull( s, key, value, branching_size );
	} else {
		return _hx_btree_node_insert_nonfull( root, key, value, branching_size );
	}
}

int _hx_btree_node_insert_nonfull( hx_btree_node* node, hx_node_id key, uintptr_t value, uint32_t branching_size ) {
	if (hx_btree_node_has_flag( node, HX_BTREE_NODE_LEAF )) {
		return hx_btree_node_add_child( node, key, value, branching_size );
	} else {
		int i;
		hx_btree_node* u	= NULL;
		for (i = 0; i < node->used - 1; i++) {
			if (key <= node->ptr[i].key) {
				uintptr_t id	= node->ptr[i].child;
				u	= (hx_btree_node*) id;
				break;
			}
		}
		if (u == NULL) {
			i	= node->used - 1;
			u	= (hx_btree_node*) node->ptr[ i ].child;
		}
		
		if (u->used == branching_size) {
			_hx_btree_node_split_child( node, i, u, branching_size );
			if (key > node->ptr[i].key) {
				i++;
				u	= (hx_btree_node*) node->ptr[ i ].child;
			}
		}
		
		if (key > node->ptr[i].key) {
			node->ptr[i].key	= key;
		}
		// sync(node)
		
		return _hx_btree_node_insert_nonfull( u, key, value, branching_size );
	}
}

int hx_btree_node_remove ( hx_btree_node** _root, hx_node_id key, uint32_t branching_size ) {
//	fprintf( stderr, "removing node %d from btree...\n", (int) key );
	// recurse to leaf node
//	fprintf( stderr, "remove> recurse to leaf node\n" );
	
	hx_btree_node* root	= *_root;
	hx_btree_node* node	= _hx_btree_node_search_page( root, key, branching_size );
	int i	= -1;
	int r	= _hx_btree_binary_search( node, key, &i );
	
	// if node doesn't have key, return 1
//	fprintf( stderr, "remove> if node doesn't have key, return 1\n" );
	if (r != 0) {
// 		fprintf( stderr, "node %d not found in btree\n", (int) key );
		return 1;
	}
	
//	fprintf( stderr, "remove> REMOVED NODE FROM LEAF\n" );
	
	int k;
REMOVE_NODE:
	// [3] remove entry from node
//	fprintf( stderr, "remove> [3] remove entry from node\n" );
//	fprintf( stderr, "      > removing node from parent with (%d/%d) slots used\n", (int) node->used, branching_size );
	for (k = i; k < node->used; k++) {
		node->ptr[k]		= node->ptr[k+1];
// 		node->keys[ k ]		= node->keys[ k + 1 ];
// 		node->children[ k ]	= node->children[ k + 1 ];
	}
	node->used--;
	// sync(node)
	
	// if node doesn't underflow return 0
//	fprintf( stderr, "remove> if node doesn't underflow return 0\n" );
	if (node->used >= branching_size/2) {
		return 0;
	}
	
//	fprintf( stderr, "remove> UNDERFLOW DETECTED\n" );
	
	// if current node is root
//	fprintf( stderr, "remove> if current node is root..." );
	if (node == root) {
//		fprintf( stderr, "yes\n" );
		if (hx_btree_node_has_flag(root, HX_BTREE_NODE_LEAF)) {
			// node is root and leaf -- we're done
//			fprintf( stderr, "remove> node is root and leaf -- we're done\n" );
			return 0;
		}
		
		// if root has only 1 child
		if (root->used == 1) {
			// make new root the current root's only child
//			fprintf( stderr, "remove> make new root the current root's only child\n" );
//			fprintf( stderr, "removing unnecessary root %p...\n", (void*) root );
			hx_btree_node* newroot	= (hx_btree_node*) root->ptr[0].child;
// 			hx_btree_node_debug( "new root>\t", s, newroot, branching_size );
//			fprintf( stderr, "setting new root to %p...\n", (void*) newroot );
			*_root	= newroot;
			hx_free_btree_node( root );
			root	= newroot;
			hx_btree_node_set_flag( root, HX_BTREE_NODE_ROOT );
			// sync(root)
		}
		return 0;
	} else {
//		fprintf( stderr, "no\n" );
	}
	
	// check number of entries in both left and right neighbors
//	fprintf( stderr, "remove> check number of entries in both left and right neighbors\n" );
	hx_btree_node* prev	= hx_btree_node_prev_neighbor( node );
	hx_btree_node* next	= hx_btree_node_next_neighbor( node );
	
// 	if (prev != NULL) {
// 		fprintf( stderr, "      > prev: (%d/%d)\n", (int) prev->used, (int) branching_size );
// 	}
// 	if (next != NULL) {
// 		fprintf( stderr, "      > next: (%d/%d)\n", (int) next->used, (int) branching_size );
// 	}
	
	int prev_minimal	= 1;
	int next_minimal	= 1;
	if (prev != NULL && prev->used > branching_size/2) {
		prev_minimal	= 0;
	}
	if (next != NULL && next->used > branching_size/2) {
		next_minimal	= 0;
	}
	
	// if both are minimal, continue, else balance current node:
	// shift over half of a neighbor’s surplus keys, adjust anchor, done
//	fprintf( stderr, "remove> if both are minimal, continue, else balance current node\n" );
	if (!(prev_minimal == 1 && next_minimal == 1)) {
//		fprintf( stderr, "remove> both are NOT minimal\n" );
		int rebalanced	= 0;
		if (prev != NULL && prev->used > branching_size/2) {
//			fprintf( stderr, "remove> rebalancing with previous node\n" );
			_hx_btree_rebalance( node, prev, 1, branching_size );
			rebalanced	= 1;
		}
		if ((!rebalanced) && next != NULL && next->used > branching_size/2) {
//			fprintf( stderr, "remove> rebalancing with next node\n" );
			_hx_btree_rebalance( node, next, 0, branching_size );
			rebalanced	= 1;
		}
		if (rebalanced == 1) {
			return 0;
		} else {
			fprintf( stderr, "*** rebalancing should have occurred, but didn't\n" );
		}
	}
	
	
	
	
	
	// merge with neighbor whose anchor is the current node's parent
//	fprintf( stderr, "remove> merge with neighbor whose anchor is the current node's parent\n" );
//	fprintf( stderr, "      > prev: %p\tnext: %p\n", (void*) prev, (void*) next );
	int merged	= 0;
	uintptr_t removed_nodeid	= 0;
	if (prev != NULL) {
//		fprintf( stderr, "node parent: %d, prev parent: %d\n", (int) node->parent, (int) prev->parent );
		if (prev->parent == node->parent) {
//			fprintf( stderr, "remove> merging with previous node (%d)\n", (int) ((uintptr_t) prev ));
			_hx_btree_merge_nodes( node, prev, branching_size );
			removed_nodeid	= ((uintptr_t) prev);
			merged	= 1;
		}
	}
	if ((!merged) && next != NULL) {
//		fprintf( stderr, "node parent: %d, next parent: %d\n", (int) node->parent, (int) next->parent );
		if (next->parent == node->parent) {
//			fprintf( stderr, "remove> merging with next node (%d)\n", (int) ((uintptr_t) next ));
			removed_nodeid	= ((uintptr_t) next);
			_hx_btree_merge_nodes( node, next, branching_size );
			merged	= 1;
		}
	}
	
	if (!merged) {
		fprintf( stderr, "*** merge should have occurred, but didn't\n" );
	}
	
	// re-wind to parent, continue at [3] (removing the just-removed node from the parent)
	int found_parent_index	= 0;
//	fprintf( stderr, "remove> re-wind to parent, continue at [3] (removing the just-removed node from the parent)\n" );
	hx_btree_node* parent	= (hx_btree_node*) node->parent;
	int j;
	for (j = 0; j < parent->used; j++) {
		if (parent->ptr[j].child == removed_nodeid) {
			found_parent_index	= 1;
			i	= j;
//			fprintf( stderr, "removed node: %d\n", (int) removed_nodeid );
//			fprintf( stderr, "\t(at index %d)\n", j );
			break;
		}
	}
	
	if (!found_parent_index) {
		fprintf( stderr, "*** didn't find node %d as a child of node %d\n", (int) removed_nodeid, (int) node->parent );
	}
	
	node	= parent;
	goto REMOVE_NODE;
}

// take half the extra nodes from the `from' node, and add them to `node'.
// dir specifies from which end of `from' to take from:
//     dir=0 => shift from the beginning
//     dir=1 => pop off the end
int _hx_btree_rebalance( hx_btree_node* node, hx_btree_node* from, int dir, uint32_t branching_size ) {
	if (from->used < branching_size/2) {
		fprintf( stderr, "*** trying to rebalance with an already underfull node\n" );
		return 1;
	}
	
// 	fprintf( stderr, "before rebalancing key count: %d, %d\n", node->used, from->used );
	
	int min		= branching_size/2;
	int extra	= from->used - min;
	int take	= (extra+1)/2;
	if (dir == 1) {
		int i;
		for (i = from->used - take - 1; i < from->used; i++) {
// 			fprintf( stderr, "rebalancing>\t%d\n", i );
			hx_btree_node_add_child( node, from->ptr[i].key, from->ptr[i].child, branching_size );
			if (!hx_btree_node_has_flag( from, HX_BTREE_NODE_LEAF )) {
				hx_btree_node* child	= (hx_btree_node*) from->ptr[i].child;
				hx_btree_node_set_parent( child, node );
				// sync(child)
			}
		}
	} else {
		int i;
		for (i = 0; i < take; i++) {
// 			fprintf( stderr, "rebalancing>\t%d\n", i );
			hx_btree_node_add_child( node, from->ptr[i].key, from->ptr[i].child, branching_size );
			if (!hx_btree_node_has_flag( from, HX_BTREE_NODE_LEAF )) {
				hx_btree_node* child	= (hx_btree_node*) from->ptr[i].child;
				hx_btree_node_set_parent( child, node );
				// sync(child)
			}
		}
		// now shift the remaining nodes in `from' over
		for (i = 0; i < from->used - take; i++) {
// 			fprintf( stderr, "rebalancing>\t[%d] = [%d]\n", i, i+take );
			from->ptr[i]		= from->ptr[i+take];
// 			from->keys[i]		= from->keys[i+take];
// 			from->children[i]	= from->children[i+take];
		}
	}
	
	// refresh the key list (this could be more efficient by only updating the two keys that were possibly affected)
	hx_btree_node* parent	= (hx_btree_node*) node->parent;
	_hx_btree_node_reset_keys( parent );
	
	from->used	-= take;
	
	// sync(from)
	// sync(parent)
	
// 	fprintf( stderr, "rebalanced key count: %d, %d\n", node->used, from->used );
	return 0;
}

void _hx_btree_node_reset_keys( hx_btree_node* parent ) {
	int i;
	for (i = 0; i < parent->used; i++) {
		hx_btree_node* child	= (hx_btree_node*) parent->ptr[i].child;
		parent->ptr[i].key	= child->ptr[ child->used - 1 ].key;
	}
}

// merge data from node b into node a
int _hx_btree_merge_nodes( hx_btree_node* a, hx_btree_node* b, uint32_t branching_size ) {
	if (a->parent != b->parent) {
		fprintf( stderr, "*** trying to merge nodes with different parents!\n" );
		return 1;
	}
	
	if ((a->used + b->used) > branching_size) {
		fprintf( stderr, "*** trying to merge nodes would result in an overflow!\n" );
		return 1;
	}
	
	int i;
	for (i = 0; i < b->used; i++) {
		hx_btree_node_add_child( a, b->ptr[i].key, b->ptr[i].child, branching_size );
		if (!hx_btree_node_has_flag( a, HX_BTREE_NODE_LEAF )) {
			hx_btree_node* child	= (hx_btree_node*) b->ptr[i].child;
			hx_btree_node_set_parent( child, a );
			// sync(child)
		}
	}
	
	a->next	= b->next;
	// sync(a)
	
	hx_btree_node* c	= (hx_btree_node*) a->next;
	if (c != NULL) {
		c->prev	= ((uintptr_t) a);
		// sync(c)
	}
	
	// refresh the key list (this could be more efficient by only updating the two keys that were possibly affected)
	hx_btree_node* parent	= (hx_btree_node*) a->parent;
	_hx_btree_node_reset_keys( parent );
	// sync(parent)
	
	return 0;
}

int _hx_btree_node_split_child( hx_btree_node* parent, uint32_t index, hx_btree_node* child, uint32_t branching_size ) {
	hx_btree_node* z	= hx_new_btree_node( branching_size );
	hx_btree_node* next	= hx_btree_node_next_neighbor( child );
	hx_btree_node_set_prev_neighbor( z, child );
	hx_btree_node_set_next_neighbor( z, next );
	hx_btree_node_set_next_neighbor( child, z );
	if (next != NULL) {
		hx_btree_node_set_prev_neighbor( next, z );
		// sync(next)
	}
	
	hx_btree_node_set_parent( z, parent );
	if (hx_btree_node_has_flag( child, HX_BTREE_NODE_LEAF )) {
		hx_btree_node_set_flag( z, HX_BTREE_NODE_LEAF );
	}
	z->used	= 0;
	int to_move		= child->used / 2;
	int child_index	= child->used - to_move;
	int j;
	for (j = 0; j < to_move; j++) {
		z->ptr[j]		= child->ptr[ child_index ];
// 		z->keys[j]		= child->keys[ child_index ];
// 		z->children[j]	= child->children[ child_index ];
		child_index++;
		z->used++;
		child->used--;
	}
	
//	uintptr_t cid	= ((uintptr_t) child);
	uintptr_t zid	= ((uintptr_t) z);
	
	hx_node_id ckey	= child->ptr[ child->used - 1 ].key;
	hx_node_id zkey	= z->ptr[ z->used - 1 ].key;
	
	parent->ptr[index].key	= ckey;
	hx_btree_node_add_child( parent, zkey, zid, branching_size );
	
	// sync(parent)
	// sync(z)
	// sync(child)
	
	return 0;
}

void hx_btree_node_traverse ( hx_btree_node* node, hx_btree_node_visitor* before, hx_btree_node_visitor* after, int level, uint32_t branching_size, void* param ) {
	if (before != NULL) before( node, level, branching_size, param );
	if (!hx_btree_node_has_flag( node, HX_BTREE_NODE_LEAF )) {
		int i;
		for (i = 0; i < node->used; i++) {
			hx_btree_node* c	= (hx_btree_node*) node->ptr[i].child;
			hx_btree_node_traverse( c, before, after, level + 1, branching_size, param );
		}
	}
	if (after != NULL) after( node, level, branching_size, param );
}

void _hx_btree_debug_leaves_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param ) {
	if (hx_btree_node_has_flag( node, HX_BTREE_NODE_LEAF )) {
		fprintf( stderr, "LEVEL %d\n", level );
		hx_btree_node_debug( "", node, branching_size );
	}
}

void _hx_btree_debug_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param ) {
	hx_btree_node_debug( (char*) param, node, branching_size );
}

void _hx_btree_count ( hx_btree_node* node, int level, uint32_t branching_size, void* param ) {
	list_size_t* count	= (list_size_t*) param;
	if (hx_btree_node_has_flag( node, HX_BTREE_NODE_LEAF )) {
		*count	+= node->used;
	}
}

void _hx_btree_free_node_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param ) {
	hx_free_btree_node( node );
}

hx_btree_iter* hx_btree_new_iter ( hx_btree* tree ) {
//	hx_btree_node* root	= (hx_btree_node*) tree->root;
	hx_btree_iter* iter	= (hx_btree_iter*) calloc( 1, sizeof( hx_btree_iter ) );
	if (iter == NULL) {
		fprintf( stderr, "*** Failed to allocate memory for btree iterator in hx_btree_new_iter\n" );
	}
	iter->started	= 0;
	iter->finished	= 0;
	iter->tree		= tree;
	iter->page		= NULL;
	iter->index		= 0;
	return iter;
}

int hx_free_btree_iter ( hx_btree_iter* iter ) {
	free( iter );
	return 0;
}


int _hx_btree_iter_prime ( hx_btree_iter* iter ) {
	iter->started	= 1;
	hx_btree_node* p	= (hx_btree_node*) iter->tree->root;
	while (!hx_btree_node_has_flag(p, HX_BTREE_NODE_LEAF)) {
		if (p->used > 0) {
			p	= (hx_btree_node*) p->ptr[0].child;
		} else {
			iter->finished	= 1;
			return 1;
		}
	}
	iter->page	= p;
	iter->index	= 0;
	return 0;
}

int hx_btree_iter_finished ( hx_btree_iter* iter ) {
	if (iter->started == 0) {
		_hx_btree_iter_prime( iter );
	}
	return iter->finished;
}

int hx_btree_iter_current ( hx_btree_iter* iter, hx_node_id* n, uintptr_t* v ) {
	if (iter->started == 0) {
		_hx_btree_iter_prime( iter );
	}
	if (iter->finished == 1) {
		return 1;
	}
	*n	= iter->page->ptr[ iter->index ].key;
	if (v != NULL) {
		*v	= iter->page->ptr[ iter->index ].child;
	}
	return 0;
}

int hx_btree_iter_next ( hx_btree_iter* iter ) {
	if (hx_btree_iter_finished(iter)) {
		return 1;
	}
	iter->index++;
	if (iter->index >= iter->page->used) {
		iter->page	= hx_btree_node_next_neighbor( iter->page );
		iter->index	= 0;
		if (iter->page == NULL) {
			iter->finished	= 1;
			return 1;
		}
	}
	return 0;
}

int hx_btree_iter_seek( hx_btree_iter* iter, hx_node_id key ) {
	if (iter->started == 0) {
		_hx_btree_iter_prime( iter );
	}
	
	hx_btree_node* u	= _hx_btree_node_search_page( (hx_btree_node*) iter->tree->root, key, iter->tree->branching_size );
	int i;
	int r	= _hx_btree_binary_search( u, key, &i );
	iter->page	= u;
	iter->index	= i;
	if (r == 0) {
		return 0;
	} else {
		return 1;
	}
}


