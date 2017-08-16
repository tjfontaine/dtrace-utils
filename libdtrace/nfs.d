/*
 * Oracle Linux DTrace.
 * Copyright (c) 2009, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

#pragma	D depends_on library ip.d
#pragma	D depends_on library net.d
#pragma	D depends_on module genunix

inline int T_RDMA = 4;
#pragma D binding "1.5" T_RDMA

typedef struct nfsv4opinfo {
	uint64_t noi_xid;	/* unique transation ID */
	cred_t *noi_cred;	/* credentials for operation */
	string noi_curpath;	/* current file handle path (if any) */
} nfsv4opinfo_t;

typedef struct nfsv4cbinfo {
	string nci_curpath;	/* current file handle path (if any) */
} nfsv4cbinfo_t;

#pragma D binding "1.5" translator
translator conninfo_t < struct svc_req *P > {
	ci_protocol = P->rq_xprt->xp_xpc.xpc_type == T_RDMA ? "rdma" :
	    P->rq_xprt->xp_xpc.xpc_netid == "tcp" ? "ipv4" :
	    P->rq_xprt->xp_xpc.xpc_netid == "udp" ? "ipv4" :
	    P->rq_xprt->xp_xpc.xpc_netid == "tcp6" ? "ipv6" :
	    P->rq_xprt->xp_xpc.xpc_netid == "udp6" ? "ipv6" :
	    "<unknown>";

	ci_local = (P->rq_xprt->xp_xpc.xpc_netid == "tcp" ||
	    P->rq_xprt->xp_xpc.xpc_netid == "udp") ?
	    inet_ntoa(&((struct sockaddr_in *)
	    P->rq_xprt->xp_xpc.xpc_lcladdr.buf)->sin_addr.S_un.S_addr) :
	    (P->rq_xprt->xp_xpc.xpc_netid == "tcp6" ||
	    P->rq_xprt->xp_xpc.xpc_netid == "udp6") ?
	    inet_ntoa6(&((struct sockaddr_in6 *)
	    P->rq_xprt->xp_xpc.xpc_lcladdr.buf)->sin6_addr) :
	    "unknown";

	ci_remote = (P->rq_xprt->xp_xpc.xpc_netid == "tcp" ||
	    P->rq_xprt->xp_xpc.xpc_netid == "udp") ?
	    inet_ntoa(&((struct sockaddr_in *)
	    P->rq_xprt->xp_xpc.xpc_rtaddr.buf)->sin_addr.S_un.S_addr) :
	    (P->rq_xprt->xp_xpc.xpc_netid == "tcp6" ||
	    P->rq_xprt->xp_xpc.xpc_netid == "udp6") ?
	    inet_ntoa6(&((struct sockaddr_in6 *)
	    P->rq_xprt->xp_xpc.xpc_rtaddr.buf)->sin6_addr) :
	    "unknown";
};

#pragma D binding "1.5" translator
translator conninfo_t < struct compound_state *P > {
	ci_protocol = P->req->rq_xprt->xp_xpc.xpc_type == T_RDMA ? "rdma" :
	    P->req->rq_xprt->xp_xpc.xpc_netid == "tcp" ? "ipv4" :
	    P->req->rq_xprt->xp_xpc.xpc_netid == "tcp6" ? "ipv6" :
	    "<unknown>";

	ci_local = (P->req->rq_xprt->xp_xpc.xpc_netid == "tcp") ?
	    inet_ntoa(&((struct sockaddr_in *)
	    P->req->rq_xprt->xp_xpc.xpc_lcladdr.buf)->sin_addr.S_un.S_addr) :
	    (P->req->rq_xprt->xp_xpc.xpc_netid == "tcp6") ?
	    inet_ntoa6(&((struct sockaddr_in6 *)
	    P->req->rq_xprt->xp_xpc.xpc_lcladdr.buf)->sin6_addr) :
	    "unknown";

	ci_remote = (P->req->rq_xprt->xp_xpc.xpc_netid == "tcp") ?
	    inet_ntoa(&((struct sockaddr_in *)
	    P->req->rq_xprt->xp_xpc.xpc_rtaddr.buf)->sin_addr.S_un.S_addr) :
	    (P->req->rq_xprt->xp_xpc.xpc_netid == "tcp6") ?
	    inet_ntoa6(&((struct sockaddr_in6 *)
	    P->req->rq_xprt->xp_xpc.xpc_rtaddr.buf)->sin6_addr) :
	    "unknown";

};

#pragma D binding "1.5" translator
translator nfsv4opinfo_t < struct compound_state *P > {
	noi_xid = P->req->rq_xprt->xp_xid;
	noi_cred = P->basecr;
	noi_curpath = (P->vp == NULL) ? "<unknown>" : P->vp->v_path;
};

#pragma D binding "1.5" translator
translator conninfo_t < rfs4_client_t *P > {
	ci_protocol = (P->rc_addr.ss_family == AF_INET) ? "ipv4" : "ipv6";

	ci_local = "<unknown>";

	ci_remote = (P->rc_addr.ss_family == AF_INET) ?
	    inet_ntoa((ipaddr_t *)
	    &((struct sockaddr_in *)&P->rc_addr)->sin_addr) :
	    inet_ntoa6(&((struct sockaddr_in6 *)&P->rc_addr)->sin6_addr);
};

#pragma D binding "1.5" translator
translator nfsv4cbinfo_t < rfs4_deleg_state_t *P > {
	nci_curpath = (P->rds_finfo->rf_vp == NULL) ? "<unknown>" :
	    P->rds_finfo->rf_vp->v_path;
};

typedef struct nfsv3opinfo {
	uint64_t noi_xid;	/* unique transation ID */
	cred_t *noi_cred;	/* credentials for operation */
	string noi_curpath;	/* current file handle path (if any) */
} nfsv3opinfo_t;

typedef struct nfsv3oparg nfsv3oparg_t;

#pragma D binding "1.5" translator
translator nfsv3opinfo_t < nfsv3oparg_t *P > {
	noi_xid = ((struct svc_req *)arg0)->rq_xprt->xp_xid;
	noi_cred = (cred_t *)arg1;
	noi_curpath = (arg2 == 0 || ((vnode_t *)arg2)->v_path == NULL) ?
	    "<unknown>" : ((vnode_t *)arg2)->v_path;
};
