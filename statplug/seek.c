/*
 * Author: Luis Useche (August 2008)
 * Email: luis@cs.fiu.edu
 *
 * BSD License
 * Copyright (c) 2008, Luis Useche
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 *     * Neither the name of Luis Useche nor the names of its
 *       contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <asm/types.h>
#include <stdlib.h>

#include <blktrace_api.h>
#include <blktrace.h>
#include <plugins.h>
#include <utils.h>
#include <list_plugins.h>

#include <reqsize.h>

#define DECL_ASSIGN_SEEK(name,data)				\
	struct seek_data *name = (struct seek_data *)data

struct seek_data 
{
	__s64 lastpos;
	struct reqsize_data *req_dat;
	
	__u64 max;
	__u64 min;
	__u64 total;
	__u64 seeks;
};

static void C(struct blk_io_trace *t, void *data)
{
	DECL_ASSIGN_SEEK(seek,data);
	
	__u64 blks = t_blks(t);
	
	if(seek->lastpos > 0) {
		if((__u64)seek->lastpos != t->sector) {
			__u64 distance = llabs(seek->lastpos - t->sector);
			seek->total += distance;
			seek->max = MAX(seek->max,distance);
			seek->min = MIN(seek->min,distance);
			seek->seeks++;
		}
	}
	
	seek->lastpos = t->sector + blks;
}

void seek_add(void *data1, const void *data2) 
{
	DECL_ASSIGN_SEEK(seek1,data1);
	DECL_ASSIGN_SEEK(seek2,data2);
	
	seek1->min = MIN(seek1->min,seek2->min);
	seek1->max = MAX(seek2->max,seek2->max);
	seek1->total += seek2->total;
	seek1->seeks += seek2->seeks;
}

void seek_print_results(const void *data)
{
	DECL_ASSIGN_SEEK(seek,data);
	
	if(seek->seeks) {
		printf("Seq.: %.2f%%\n",
		       (1-(((double)seek->seeks)/seek->req_dat->total_size))*100);
		printf("Seeks #: %llu min: %llu avg: %f max: %llu (blks)\n",
		       seek->seeks,
		       seek->min,
		       ((double)seek->total)/seek->seeks,
		       seek->max);
	}
}

void seek_init(struct plugin *p, struct plugin_set *ps)
{
	struct seek_data *seek = p->data = g_new0(struct seek_data,1);
	seek->lastpos = -1;
	seek->max = 0;
	seek->min = ~0;
	seek->total = 0;
	seek->seeks = 0;
	seek->req_dat = (struct reqsize_data *)ps->plugs[REQ_SIZE_IND].data;
}

void seek_destroy(struct plugin *p)
{
	g_free(p->data);
}

void seek_ops_init(struct plugin_ops *po)
{
	po->add = seek_add;
	po->print_results = seek_print_results;
	
	g_tree_insert(po->event_tree,(gpointer)__BLK_TA_COMPLETE,C);
}
