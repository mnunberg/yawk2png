#include "xgetopt.h"
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct xoption *xoptions_new()
{
	struct xoption *ret = (struct xoption*)calloc(sizeof(struct xoption), 1);
	return ret;
}

struct xoption *xoptadd(struct xoption *xoptptr, const char *longopt,
						char shortopt, int has_arg, const char *helpmsg)
{
	struct xoption *opt = xoptions_new();
	opt->helpmsg = helpmsg;
	opt->_option.name = longopt;
	opt->_option.val = shortopt;
	opt->_option.has_arg = has_arg;
	xoptptr->next = opt;
	return opt;
}

struct option *mkopts(struct xoption *rootptr, int noptions)
{
	int len = noptions, i = 0;
	struct xoption *ptr;
	if(!len) /*wasn't specified :( */ {
		for(ptr = rootptr; ptr; ptr = ptr->next, len++);
	}
	struct option *ret = (struct option*)malloc(len * sizeof(struct option) + 1);
	ptr = rootptr;
	for(i=0; i<len; i++) {
		ret[i] = ptr->_option;
		ptr = ptr->next;
	}
	assert(!ptr);
	ptr = xoptions_new();
	ret[i] = ptr->_option;
	return ret;
}

const char *usage_string(struct xoption *rootptr)
{
/*allocate a sufficiently sized buffer...*/
	char *ret = (char*)malloc(1024); /*really, more than enough*/
	int nleft = 1024;

	struct xoption *ptr = rootptr;
	for(;ptr; ptr = ptr->next) {
		strncat(ret, ptr->helpmsg, nleft);
		nleft -= strlen(ptr->helpmsg);
	}
	return ret;
}
