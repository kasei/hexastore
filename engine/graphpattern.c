#include "engine/graphpattern.h"

hx_variablebindings_iter* hx_graphpattern_execute ( hx_graphpattern* pat, hx_hexastore* hx ) {
	int i;
	void** vp;
	hx_expr* e;
	hx_graphpattern *gp		= NULL;
	hx_graphpattern *gp2	= NULL;
	hx_variablebindings_iter *iter, *iter2, *iter3;
	hx_graphpattern** p;
	hx_nodemap* map	= hx_get_nodemap( hx );
	switch (pat->type) {
		case HX_GRAPHPATTERN_BGP:
			return hx_bgp_execute( pat->data, hx );
		case HX_GRAPHPATTERN_GROUP:
			p		= (hx_graphpattern**) pat->data;
			iter	= hx_graphpattern_execute( p[0], hx );
			for (i = 1; i < pat->arity; i++) {
				gp2		= (hx_graphpattern*) p[i];
				iter2	= hx_graphpattern_execute( gp2, hx );
				iter	= hx_new_mergejoin_iter( iter, iter2 );
			}
			return iter;
		case HX_GRAPHPATTERN_FILTER:
			vp		= (void**) pat->data;
			e		= (hx_expr*) vp[0];
			gp		= (hx_graphpattern*) vp[1];
			iter2	= hx_graphpattern_execute( gp, hx );
			iter	= hx_new_filter_iter( iter2, e, map );
			return iter;
		case HX_GRAPHPATTERN_GRAPH:
			fprintf( stderr, "*** GRAPH graph patterns are not implemented in hx_graphpattern_execute\n" );
			return NULL;
		case HX_GRAPHPATTERN_OPTIONAL:
			vp		= (void**) pat->data;
			gp		= (hx_graphpattern*) vp[0];
			iter	= hx_graphpattern_execute( gp, hx );
			gp2		= (hx_graphpattern*) vp[1];
			iter2	= hx_graphpattern_execute( gp2, hx );
			iter3	= hx_new_nestedloopjoin_iter2( iter, iter2, 1 );
			return iter3;
		case HX_GRAPHPATTERN_UNION:
			fprintf( stderr, "*** Unimplemented graph pattern type '%c' in hx_graphpattern_execute\n", pat->type );
			return NULL;
		default:
			fprintf( stderr, "*** Unrecognized graph pattern type '%c' in hx_graphpattern_execute\n", pat->type );
			return NULL;
	};
}

