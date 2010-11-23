#ifndef XGETOPT_H
#define XGETOPT_H
/*Just some wrapper functions around getopt*/

/*maybe just make it a class?*/
#include <getopt.h>

struct xoption;

struct xoption {
	struct xoption *next;
	struct option _option;
	const char *helpmsg;
};
struct option *xmkopts(struct xoption *options);
struct xoption *xoptadd(struct xoption *xoptptr, const char *longopt,
				char shortopt, int has_arg, const char *helpmsg);
struct xoption *xoptions_new();
void xfreeoptions(struct xoptions *xoptions, struct options *options);
#endif // XGETOPT_H
