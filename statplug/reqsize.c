#include <asm/types.h>
#include <glib.h>
#include <stdio.h>

#include <blktrace_api.h>
#include <plugins.h>
#include <utils.h>

#define DECL_ASSIGN_REQSIZE(name,data)		\
	struct reqsize_data *name = (struct reqsize_data *)data

struct reqsize_data 
{
	__u64 min;
	__u64 max;
	__u64 total_size;
	__u64 reqs;
};

void C(struct blk_io_trace *t, void *data) 
{
	DECL_ASSIGN_REQSIZE(rsd,data);
	
	__u64 blks = BYTES_TO_BLKS(t->bytes);
	
	if(blks) {
		rsd->min = MIN(rsd->min, blks);
		rsd->max = MAX(rsd->max, blks);
		rsd->total_size += blks;
		rsd->reqs++;
	}
}

void add(void *data1, const void *data2) 
{
	DECL_ASSIGN_REQSIZE(rsd1,data1);
	DECL_ASSIGN_REQSIZE(rsd2,data2);
	
	rsd1->min = MIN(rsd1->min,rsd2->min);
	rsd1->max = MAX(rsd1->max,rsd2->max);
	rsd1->total_size += rsd2->total_size;
	rsd1->reqs += rsd2->reqs;
}

void print_results(const void *data)
{
	DECL_ASSIGN_REQSIZE(rsd,data);
	
	printf("Reqs. #: %lld min: %lld avg: %f max: %lld\n",
	       rsd->reqs,
	       rsd->min,
	       ((double)rsd->total_size)/rsd->reqs,
	       rsd->max);
}

void reqsize_init(struct plugin *p) 
{
	p->data = g_new(struct reqsize_data,1);
}

void reqsize_ops_init(struct plugin_ops *po)
{
	po->add = add;
	po->print_results = print_results;
	
	po->event_ht = g_hash_table_new(g_int_hash, g_int_equal);
	/* association of event int and function */
	g_hash_table_insert(po->event_ht,(gpointer)BLK_TA_COMPLETE,C);
}

void reqsize_ops_destroy(struct plugin_ops *po) 
{
	g_hash_table_destroy(po->event_ht);
}

void reqsize_destroy(struct plugin *p)
{
	g_free(p->data);
}
