#include "mentok/engine/graphpattern.h"
#include "mentok/engine/expr.h"
#include "mentok/engine/bgp.h"
#include "mentok/engine/filter.h"
#include "mentok/engine/nestedloopjoin.h"

hx_variablebindings_iter* hx_graphpattern_execute ( hx_execution_context* ctx, hx_graphpattern* pat ) {
	hx_model* hx	= ctx->hx;
	int i;
	void** vp;
	hx_expr* e;
	hx_graphpattern *gp		= NULL;
	hx_graphpattern *gp2	= NULL;
	hx_variablebindings_iter *iter, *iter2, *iter3;
	hx_graphpattern** p;
	switch (pat->type) {
		case HX_GRAPHPATTERN_BGP:
			return ctx->bgp_exec_func( ctx, pat->data, ctx->bgp_exec_func_thunk );
		case HX_GRAPHPATTERN_GROUP:
			p		= (hx_graphpattern**) pat->data;
			iter	= hx_graphpattern_execute( ctx, p[0] );
			for (i = 1; i < pat->arity; i++) {
				gp2		= (hx_graphpattern*) p[i];
				iter2	= hx_graphpattern_execute( ctx, gp2 );
				iter	= hx_new_mergejoin_iter( iter, iter2 );
			}
			return iter;
		case HX_GRAPHPATTERN_FILTER:
			vp		= (void**) pat->data;
			e		= (hx_expr*) vp[0];
			gp		= (hx_graphpattern*) vp[1];
			iter2	= hx_graphpattern_execute( ctx, gp );
			iter	= hx_new_filter_iter( iter2, e, ctx );
			return iter;
		case HX_GRAPHPATTERN_OPTIONAL:
			vp		= (void**) pat->data;
			gp		= (hx_graphpattern*) vp[0];
			iter	= hx_graphpattern_execute( ctx, gp );
			gp2		= (hx_graphpattern*) vp[1];
			iter2	= hx_graphpattern_execute( ctx, gp2 );
			iter3	= hx_new_nestedloopjoin_iter2( iter, iter2, 1 );
			return iter3;
		case HX_GRAPHPATTERN_UNION:
			vp		= (void**) pat->data;
			gp		= (hx_graphpattern*) vp[0];
			iter	= hx_graphpattern_execute( ctx, gp );
			gp2		= (hx_graphpattern*) vp[1];
			iter2	= hx_graphpattern_execute( ctx, gp2 );
			iter3	= hx_new_union_iter2( ctx, iter, iter2 );
			return iter3;
		case HX_GRAPHPATTERN_GRAPH:
			fprintf( stderr, "*** GRAPH graph patterns are not implemented in hx_graphpattern_execute\n" );
			return NULL;
		default:
			fprintf( stderr, "*** Unrecognized graph pattern type '%c' in hx_graphpattern_execute\n", pat->type );
			return NULL;
	};
}

hx_graphpattern* hx_graphpattern_substitute_variables ( hx_graphpattern* orig, hx_variablebindings* b, hx_store* store ) {
	void** vp;
	hx_expr *e, *e2;
	hx_graphpattern *gp1, *gp2;
	switch (orig->type) {
		case HX_GRAPHPATTERN_BGP:
			return hx_new_graphpattern(
					HX_GRAPHPATTERN_BGP,
					hx_bgp_substitute_variables( (hx_bgp*) orig->data, b, store )
				);
		case HX_GRAPHPATTERN_FILTER:
			vp		= (void**) orig->data;
			e		= (hx_expr*) vp[0];
			e2		= hx_expr_substitute_variables( e, b, store );
			gp1		= (hx_graphpattern*) vp[1];
			gp2		= hx_graphpattern_substitute_variables( gp1, b, store );
			return hx_new_graphpattern( HX_GRAPHPATTERN_FILTER,	e2, gp2 );
		case HX_GRAPHPATTERN_UNION:
		case HX_GRAPHPATTERN_OPTIONAL:
		case HX_GRAPHPATTERN_GROUP:
		case HX_GRAPHPATTERN_GRAPH:
			fprintf( stderr, "*** Unimplemented graph pattern type '%c' in hx_graphpattern_substitute_variables\n", orig->type );
			return NULL;
		default:
			fprintf( stderr, "*** Unrecognized graph pattern type '%c' in hx_graphpattern_substitute_variables\n", orig->type );
			return NULL;
	}
}

