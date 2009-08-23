#include "optimizer/plan.h"

hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_optimizer_plan_type type, void* source, hx_triple* t, int order_count, hx_variablebindings_iter_sorting** order ) {
	hx_optimizer_plan* plan	= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type			= type;
	plan->triple		= t;
	plan->source		= source;
	plan->order_count	= order_count;
	plan->order			= order;
	return plan;
}

int hx_free_optimizer_access_plan ( hx_optimizer_plan* plan ) {
	free( plan );
}

