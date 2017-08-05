/*
 * Oracle Linux DTrace.
 * Copyright © 2003, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

#ifndef	_DT_LIST_H
#define	_DT_LIST_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_list {
	struct dt_list *dl_prev;
	struct dt_list *dl_next;
} dt_list_t;

#define	dt_list_prev(elem)	((void *)(((dt_list_t *)(elem))->dl_prev))
#define	dt_list_next(elem)	((void *)(((dt_list_t *)(elem))->dl_next))

extern void dt_list_append(dt_list_t *, void *);
extern void dt_list_prepend(dt_list_t *, void *);
extern void dt_list_insert(dt_list_t *, void *, void *);
extern void dt_list_delete(dt_list_t *, void *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_LIST_H */
