/* DO NOT EDIT: automatically built by dist/s_win32. */
/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2001
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id$
 *
 * db.h include file layout:
 *	General.
 *	Database Environment.
 *	Locking subsystem.
 *	Logging subsystem.
 *	Shared buffer cache (mpool) subsystem.
 *	Transaction subsystem.
 *	Access methods.
 *	Access method cursors.
 *	Dbm/Ndbm, Hsearch historic interfaces.
 */

#ifndef _DB_H_
#define	_DB_H_

#ifndef __NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <stdio.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * XXX
 * Handle function prototypes and the keyword "const".  This steps on name
 * space that DB doesn't control, but all of the other solutions are worse.
 *
 * XXX
 * While Microsoft's compiler is ANSI C compliant, it doesn't have _STDC_
 * defined by default, you specify a command line flag or #pragma to turn
 * it on.  Don't do that, however, because some of Microsoft's own header
 * files won't compile.
 */
#undef	__P
#if defined(__STDC__) || defined(__cplusplus) || defined(_MSC_VER) || defined(__MINGW32__)
#define	__P(protos)	protos		/* ANSI C prototypes */
#else
#define	const
#define	__P(protos)	()		/* K&R C preprocessor */
#endif

/*
 * Berkeley DB version information.
 */
#define	DB_VERSION_MAJOR	4
#define	DB_VERSION_MINOR	0
#define	DB_VERSION_PATCH	14
#define	DB_VERSION_STRING	"Sleepycat Software: Berkeley DB 4.0.14: (November 18, 2001)"

/*
 * !!!
 * Berkeley DB uses specifically sized types.  If they're not provided by
 * the system, typedef them here.
 *
 * We protect them against multiple inclusion using __BIT_TYPES_DEFINED__,
 * as does BIND and Kerberos, since we don't know for sure what #include
 * files the user is using.
 *
 * !!!
 * We also provide the standard u_int, u_long etc., if they're not provided
 * by the system.
 */
#ifndef	__BIT_TYPES_DEFINED__
#define	__BIT_TYPES_DEFINED__
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;
#endif

#if !defined(_WINSOCKAPI_)
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif
#if defined(_WIN64) || defined(_M_ALPHA)
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

/* Basic types that are exported or quasi-exported. */
typedef	u_int32_t	db_pgno_t;	/* Page number type. */
typedef	u_int16_t	db_indx_t;	/* Page offset type. */
#define	DB_MAX_PAGES	0xffffffff	/* >= # of pages in a file */

typedef	u_int32_t	db_recno_t;	/* Record number type. */
#define	DB_MAX_RECORDS	0xffffffff	/* >= # of records in a tree */

typedef u_int32_t	db_timeout_t;	/* Type of a timeout. */

/*
 * Region offsets are currently limited to 32-bits.  I expect that's going
 * to have to be fixed in the not-too-distant future, since we won't want to
 * split 100Gb memory pools into that many different regions.
 */
typedef u_int32_t roff_t;

/*
 * Forward structure declarations, so we can declare pointers and
 * applications can get type checking.
 */
struct __db;		typedef struct __db DB;
struct __db_bt_stat;	typedef struct __db_bt_stat DB_BTREE_STAT;
struct __db_dbt;	typedef struct __db_dbt DBT;
struct __db_env;	typedef struct __db_env DB_ENV;
struct __db_h_stat;	typedef struct __db_h_stat DB_HASH_STAT;
struct __db_ilock;	typedef struct __db_ilock DB_LOCK_ILOCK;
struct __db_lock_stat;	typedef struct __db_lock_stat DB_LOCK_STAT;
struct __db_lock_u;	typedef struct __db_lock_u DB_LOCK;
struct __db_lockreq;	typedef struct __db_lockreq DB_LOCKREQ;
struct __db_log_cursor;	typedef struct __db_log_cursor DB_LOGC;
struct __db_log_stat;	typedef struct __db_log_stat DB_LOG_STAT;
struct __db_lsn;	typedef struct __db_lsn DB_LSN;
struct __db_mpool;	typedef struct __db_mpool DB_MPOOL;
struct __db_mpool_fstat;typedef struct __db_mpool_fstat DB_MPOOL_FSTAT;
struct __db_mpool_stat;	typedef struct __db_mpool_stat DB_MPOOL_STAT;
struct __db_mpoolfile;	typedef struct __db_mpoolfile DB_MPOOLFILE;
struct __db_preplist;	typedef struct __db_preplist DB_PREPLIST;
struct __db_qam_stat;	typedef struct __db_qam_stat DB_QUEUE_STAT;
struct __db_rep;	typedef struct __db_rep DB_REP;
struct __db_txn;	typedef struct __db_txn DB_TXN;
struct __db_txn_active;	typedef struct __db_txn_active DB_TXN_ACTIVE;
struct __db_txn_stat;	typedef struct __db_txn_stat DB_TXN_STAT;
struct __db_txnmgr;	typedef struct __db_txnmgr DB_TXNMGR;
struct __dbc;		typedef struct __dbc DBC;
struct __dbc_internal;	typedef struct __dbc_internal DBC_INTERNAL;
struct __fh_t;		typedef struct __fh_t DB_FH;
struct __key_range;	typedef struct __key_range DB_KEY_RANGE;
struct __mpoolfile;	typedef struct __mpoolfile MPOOLFILE;
struct __mutex_t;	typedef struct __mutex_t DB_MUTEX;

/* Key/data structure -- a Data-Base Thang. */
struct __db_dbt {
	/*
	 * data/size must be fields 1 and 2 for DB 1.85 compatibility.
	 */
	void	 *data;			/* Key/data */
	u_int32_t size;			/* key/data length */

	u_int32_t ulen;			/* RO: length of user buffer. */
	u_int32_t dlen;			/* RO: get/put record length. */
	u_int32_t doff;			/* RO: get/put record offset. */

#define	DB_DBT_APPMALLOC	0x001	/* Callback allocated memory. */
#define	DB_DBT_ISSET		0x002	/* Lower level calls set value. */
#define	DB_DBT_MALLOC		0x004	/* Return in malloc'd memory. */
#define	DB_DBT_PARTIAL		0x008	/* Partial put/get. */
#define	DB_DBT_REALLOC		0x010	/* Return in realloc'd memory. */
#define	DB_DBT_USERMEM		0x020	/* Return in user's memory. */
#define	DB_DBT_DUPOK		0x040	/* Insert if duplicate. */
	u_int32_t flags;
};

/*
 * Common flags --
 *	Interfaces which use any of these common flags should never have
 *	interface specific flags in this range.
 */
#define	DB_CREATE	      0x000001	/* Create file as necessary. */
#define	DB_CXX_NO_EXCEPTIONS  0x000002	/* C++: return error values. */
#define	DB_FORCE	      0x000004	/* Force (anything). */
#define	DB_NOMMAP	      0x000008	/* Don't mmap underlying file. */
#define	DB_RDONLY	      0x000010	/* Read-only (O_RDONLY). */
#define	DB_RECOVER	      0x000020	/* Run normal recovery. */
#define	DB_THREAD	      0x000040	/* Applications are threaded. */
#define	DB_TXN_NOSYNC	      0x000080	/* Do not sync log on commit. */
#define	DB_USE_ENVIRON	      0x000100	/* Use the environment. */
#define	DB_USE_ENVIRON_ROOT   0x000200	/* Use the environment if root. */

/*
 * Common flags --
 *	The dirty-read flag is used by many interfaces: DB->cursor, DB->get,
 *	DB->join, DB->open, DBcursor->c_get and DB_ENV->txn_begin.
 */
#define	DB_DIRTY_READ	    0x01000000	/* Dirty Read. */

/*
 * Flags private to db_env_create.
 */
#define	DB_CLIENT	      0x000400	/* Open for a client environment. */

/*
 * Flags private to db_create.
 */
#define	DB_XA_CREATE	      0x000400	/* Open in an XA environment. */

/*
 * Flags private to DB_ENV->open.
 */
#define	DB_INIT_CDB	      0x000400	/* Concurrent Access Methods. */
#define	DB_INIT_LOCK	      0x000800	/* Initialize locking. */
#define	DB_INIT_LOG	      0x001000	/* Initialize logging. */
#define	DB_INIT_MPOOL	      0x002000	/* Initialize mpool. */
#define	DB_INIT_TXN	      0x004000	/* Initialize transactions. */
#define	DB_JOINENV	      0x008000  /* Initialize all subsystems present. */
#define	DB_LOCKDOWN	      0x010000	/* Lock memory into physical core. */
#define	DB_PRIVATE	      0x020000	/* DB_ENV is process local. */
#define	DB_RECOVER_FATAL      0x040000	/* Run catastrophic recovery. */
#define	DB_SYSTEM_MEM	      0x080000	/* Use system-backed memory. */

/*
 * Flags private to DB->open (and by extension, __log_register_int).
 */
#define	DB_EXCL		      0x000400	/* Exclusive open (O_EXCL). */
#define	DB_FCNTL_LOCKING      0x000800	/* UNDOC: fcntl(2) locking. */
#define	DB_RDWRMASTER	      0x001000  /* UNDOC: allow subdb master open R/W */
#define	DB_TRUNCATE	      0x002000	/* Discard existing DB (O_TRUNC). */
#define	DB_EXTENT	      0x004000  /* UNDOC: dealing with an extent. */
#define	DB_APPLY_LOGREG	      0x008000  /* UNDOC: log_register in replicat. */

/*
 * Flags private to DB_ENV->txn_begin.
 */
#define	DB_TXN_NOWAIT	      0x000400	/* Do not wait for locks in this TXN. */
#define	DB_TXN_SYNC	      0x000800	/* Always sync log on commit. */

/*
 * Flags private to DB_ENV->set_flags.
 */
#define	DB_CDB_ALLDB	      0x000400	/* Set CDB locking per environment. */
#define	DB_NOLOCKING	      0x001000	/* Set locking/mutex behavior. */
#define	DB_NOPANIC	      0x002000	/* Set panic state per DB_ENV. */
#define	DB_PANIC_ENVIRONMENT  0x004000	/* Set panic state per environment. */
#define	DB_REGION_INIT	      0x008000	/* Set panic state per DB_ENV. */
#define	DB_YIELDCPU	      0x010000	/* Yield the CPU (a lot). */

/*
 * Flags private to DB->set_feedback's callback.
 */
#define	DB_UPGRADE	      0x000400	/* Upgrading. */
#define	DB_VERIFY	      0x000800  /* Verifying. */

/*
 * Flags private to DB_MPOOLFILE->open.
 */
#define	DB_ODDFILESIZE	      0x000400	/* Truncate file to N * pgsize. */

/*
 * Flags private to DB->set_flags.
 *
 * DB->set_flags does not share common flags and so values start at 0x01.
 */
#define	DB_DUP		      0x000001	/* Btree, Hash: duplicate keys. */
#define	DB_DUPSORT	      0x000002	/* Btree, Hash: duplicate keys. */
#define	DB_RECNUM	      0x000004	/* Btree: record numbers. */
#define	DB_RENUMBER	      0x000008	/* Recno: renumber on insert/delete. */
#define	DB_REVSPLITOFF	      0x000010	/* Btree: turn off reverse splits. */
#define	DB_SNAPSHOT	      0x000020	/* Recno: snapshot the input. */

/*
 * Flags private to the _stat methods.
 */
#define	DB_STAT_CLEAR	      0x000001	/* Clear stat after returning values. */

/*
 * Flags private to DB->join.
 *
 * DB->join does not share common flags and so values start at 0x01.
 */
#define	DB_JOIN_NOSORT	      0x000001  /* Don't try to optimize join. */

/*
 * Flags private to DB->verify.
 *
 * DB->verify does not share common flags and so values start at 0x01.
 */
#define	DB_AGGRESSIVE	      0x000001  /* Salvage whatever could be data.*/
#define	DB_NOORDERCHK	      0x000002  /* Skip sort order/hashing check. */
#define	DB_ORDERCHKONLY	      0x000004  /* Only perform the order check. */
#define	DB_PR_PAGE	      0x000008  /* Show page contents (-da). */
#define	DB_PR_RECOVERYTEST    0x000010  /* Recovery test (-dr). */
#define	DB_SALVAGE	      0x000020  /* Salvage what looks like data. */
/*
 * !!!
 * These must not go over 0x8000, or they will collide with the flags
 * used by __bam_vrfy_subtree.
 */

/*
 * Flags private to DB->set_rep_transport's send callback.
 */
#define	DB_REP_PERMANENT	0x0001	/* Important--app. may want to flush. */

/*******************************************************
 * Shared subsystem information.
 *
 * The following are shared between the general environment and a particular
 * subsystem, and so need to come first.
 *******************************************************/
#define	DB_FILE_ID_LEN		20	/* Unique file ID length. */

typedef enum {
	DB_TXN_ABORT=0,			/* Public. */
	DB_TXN_APPLY=1,			/* Internal. */
	DB_TXN_BACKWARD_ROLL=2,		/* Public. */
	DB_TXN_FORWARD_ROLL=3,		/* Public. */
	DB_TXN_OPENFILES=4,		/* Internal. */
	DB_TXN_POPENFILES=5		/* Internal. */
} db_recops;

/*
 * Simple R/W lock modes and for multi-granularity intention locking.
 *
 * !!!
 * These values are NOT random, as they are used as an index into the lock
 * conflicts arrays, i.e., DB_LOCK_IWRITE must be == 3, and DB_LOCK_IREAD
 * must be == 4.
 */
typedef enum {
	DB_LOCK_NG=0,			/* Not granted. */
	DB_LOCK_READ=1,			/* Shared/read. */
	DB_LOCK_WRITE=2,		/* Exclusive/write. */
	DB_LOCK_WAIT=3,			/* Wait for event */
	DB_LOCK_IWRITE=4,		/* Intent exclusive/write. */
	DB_LOCK_IREAD=5,		/* Intent to share/read. */
	DB_LOCK_IWR=6,			/* Intent to read and write. */
	DB_LOCK_DIRTY=7,		/* Dirty Read. */
	DB_LOCK_WWRITE=8		/* Was Written. */
} db_lockmode_t;

typedef	u_int32_t	db_reptype_t;	/* Replication log record type. */

/*******************************************************
 * Environment.
 *******************************************************/
#define	DB_REGION_MAGIC	0x120897	/* Environment magic number. */

#define	DB_UNDO(op)	((op) == DB_TXN_ABORT || (op) == DB_TXN_BACKWARD_ROLL)
#define	DB_REDO(op)	((op) == DB_TXN_FORWARD_ROLL || (op) == DB_TXN_APPLY)

/* Database Environment handle. */
struct __db_env {
	/*******************************************************
	 * Public: owned by the application.
	 *******************************************************/
	FILE		*db_errfile;	/* Error message file stream. */
	const char	*db_errpfx;	/* Error message prefix. */
					/* Callbacks. */
	void (*db_errcall) __P((const char *, char *));
	void (*db_feedback) __P((DB_ENV *, int, int));
	void (*db_paniccall) __P((DB_ENV *, int));
	int  (*db_recovery_init) __P((DB_ENV *));

					/* App-specified alloc functions. */
	void *(*db_malloc) __P((size_t));
	void *(*db_realloc) __P((void *, size_t));
	void (*db_free) __P((void *));

	/*
	 * Currently, the verbose list is a bit field with room for 32
	 * entries.  There's no reason that it needs to be limited, if
	 * there are ever more than 32 entries, convert to a bit array.
	 */
#define	DB_VERB_CHKPOINT	0x0001	/* List checkpoints. */
#define	DB_VERB_DEADLOCK	0x0002	/* Deadlock detection information. */
#define	DB_VERB_RECOVERY	0x0004	/* Recovery information. */
#define	DB_VERB_REPLICATION	0x0008	/* Replication information. */
#define	DB_VERB_WAITSFOR	0x0010	/* Dump waits-for table. */
	u_int32_t	 verbose;	/* Verbose output. */

	void		*app_private;	/* Application-private handle. */

	/* Locking. */
	u_int8_t	*lk_conflicts;	/* Two dimensional conflict matrix. */
	u_int32_t	 lk_modes;	/* Number of lock modes in table. */
	u_int32_t	 lk_max;	/* Maximum number of locks. */
	u_int32_t	 lk_max_lockers;/* Maximum number of lockers. */
	u_int32_t	 lk_max_objects;/* Maximum number of locked objects. */
	u_int32_t	 lk_detect;	/* Deadlock detect on all conflicts. */
	db_timeout_t	 lk_timeout;	/* Lock timeout period. */

	/* Logging. */
	u_int32_t	 lg_bsize;	/* Buffer size. */
	u_int32_t	 lg_max;	/* Maximum file size. */
	u_int32_t	 lg_regionmax;	/* Region size. */

	/* Memory pool. */
	u_int32_t	 mp_gbytes;	/* Cachesize: GB. */
	u_int32_t	 mp_bytes;	/* Cachesize: Bytes. */
	size_t		 mp_size;	/* DEPRECATED: Cachesize: bytes. */
	int		 mp_ncache;	/* Number of cache regions. */
	size_t		 mp_mmapsize;	/* Maximum file size for mmap. */

	int		 rep_eid;	/* environment id. */

	/* Transactions. */
	u_int32_t	 tx_max;	/* Maximum number of transactions. */
	time_t		 tx_timestamp;	/* Recover to specific timestamp. */
	db_timeout_t	 tx_timeout;	/* Timeout for transactions. */
	int (*tx_recover)		/* Dispatch function for recovery. */
	    __P((DB_ENV *, DBT *, DB_LSN *, db_recops));

	/*******************************************************
	 * Private: owned by DB.
	 *******************************************************/
	int		 panic_errval;	/* Panic causing errno. */

					/* User files, paths. */
	char		*db_home;	/* Database home. */
	char		*db_log_dir;	/* Database log file directory. */
	char		*db_tmp_dir;	/* Database tmp file directory. */

	char	       **db_data_dir;	/* Database data file directories. */
	int		 data_cnt;	/* Database data file slots. */
	int		 data_next;	/* Next Database data file slot. */

	int		 db_mode;	/* Default open permissions. */

	void		*reginfo;	/* REGINFO structure reference. */
	DB_FH		*lockfhp;	/* fcntl(2) locking file handle. */

	int	      (**dtab)		/* Dispatch table */
			    __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t		 dtab_size;	/* Slots in the dispatch table. */

	void		*cl_handle;	/* RPC: remote client handle. */
	long		 cl_id;		/* RPC: remote client env id. */

	int		 db_ref;	/* db reference count. */

	long		 shm_key;	/* shmget(2) key. */
	u_int32_t	 tas_spins;	/* test-and-set spins. */

	/*
	 * List of open DB handles for this DB_ENV, used for cursor
	 * adjustment.  Must be protected for multi-threaded support.
	 *
	 * !!!
	 * As this structure is allocated in per-process memory, the
	 * mutex may need to be stored elsewhere on architectures unable
	 * to support mutexes in heap memory, e.g. HP/UX 9.
	 *
	 * !!!
	 * Explicit representation of structure in queue.h.
	 * LIST_HEAD(dblist, __db);
	 */
	DB_MUTEX	*dblist_mutexp;	/* Mutex. */
	struct {
		struct __db *lh_first;
	} dblist;

	/*
	 * XA support.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_env) links;
	 */
	struct {
		struct __db_env *tqe_next;
		struct __db_env **tqe_prev;
	} links;
	int		 xa_rmid;	/* XA Resource Manager ID. */
	DB_TXN		*xa_txn;	/* XA Current transaction. */

	void	*cj_internal;		/* C++/Java private. */

					/* DB_ENV Methods. */
	int  (*close) __P((DB_ENV *, u_int32_t));
	void (*err) __P((const DB_ENV *, int, const char *, ...));
	void (*errx) __P((const DB_ENV *, const char *, ...));
	int  (*open) __P((DB_ENV *, const char *, u_int32_t, int));
	int  (*remove) __P((DB_ENV *, const char *, u_int32_t));
	int  (*set_data_dir) __P((DB_ENV *, const char *));
	int  (*set_alloc) __P((DB_ENV *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	void (*set_errcall) __P((DB_ENV *, void (*)(const char *, char *)));
	void (*set_errfile) __P((DB_ENV *, FILE *));
	void (*set_errpfx) __P((DB_ENV *, const char *));
	int  (*set_feedback) __P((DB_ENV *, void (*)(DB_ENV *, int, int)));
	int  (*set_flags) __P((DB_ENV *, u_int32_t, int));
	int  (*set_paniccall) __P((DB_ENV *, void (*)(DB_ENV *, int)));
	int  (*set_recovery_init) __P((DB_ENV *, int (*)(DB_ENV *)));
	int  (*set_rpc_server) __P((DB_ENV *,
		void *, const char *, long, long, u_int32_t));
	int  (*set_shm_key) __P((DB_ENV *, long));
	int  (*set_tas_spins) __P((DB_ENV *, u_int32_t));
	int  (*set_tmp_dir) __P((DB_ENV *, const char *));
	int  (*set_verbose) __P((DB_ENV *, u_int32_t, int));

	void *lg_handle;		/* Log handle and methods. */
	int  (*set_lg_bsize) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_dir) __P((DB_ENV *, const char *));
	int  (*set_lg_max) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_regionmax) __P((DB_ENV *, u_int32_t));
	int  (*log_archive) __P((DB_ENV *, char **[], u_int32_t));
	int  (*log_cursor) __P((DB_ENV *, DB_LOGC **, u_int32_t));
	int  (*log_file) __P((DB_ENV *, const DB_LSN *, char *, size_t));
	int  (*log_flush) __P((DB_ENV *, const DB_LSN *));
	int  (*log_put) __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
	int  (*log_register) __P((DB_ENV *, DB *, const char *));
	int  (*log_stat) __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
	int  (*log_unregister) __P((DB_ENV *, DB *));

	void *lk_handle;		/* Lock handle and methods. */
	int  (*set_lk_conflicts) __P((DB_ENV *, u_int8_t *, int));
	int  (*set_lk_detect) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_locks) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_lockers) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_objects) __P((DB_ENV *, u_int32_t));
	int  (*lock_detect) __P((DB_ENV *, u_int32_t, u_int32_t, int *));
	int  (*lock_dump_region) __P((DB_ENV *, char *, FILE *));
	int  (*lock_get) __P((DB_ENV *,
		u_int32_t, u_int32_t, const DBT *, db_lockmode_t, DB_LOCK *));
	int  (*lock_put) __P((DB_ENV *, DB_LOCK *));
	int  (*lock_id) __P((DB_ENV *, u_int32_t *));
	int  (*lock_id_free) __P((DB_ENV *, u_int32_t));
	int  (*lock_id_set) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*lock_stat) __P((DB_ENV *, DB_LOCK_STAT **, u_int32_t));
	int  (*lock_vec) __P((DB_ENV *,
		u_int32_t, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
	int  (*lock_downgrade) __P((DB_ENV *,
		DB_LOCK *, db_lockmode_t, u_int32_t));

	void *mp_handle;		/* Mpool handle and methods. */
	int  (*set_mp_mmapsize) __P((DB_ENV *, size_t));
	int  (*set_cachesize) __P((DB_ENV *, u_int32_t, u_int32_t, int));
	int  (*memp_dump_region) __P((DB_ENV *, char *, FILE *));
	int  (*memp_fcreate) __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
	int  (*memp_register) __P((DB_ENV *, int,
		int (*)(DB_ENV *, db_pgno_t, void *, DBT *),
		int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
	int  (*memp_stat) __P((DB_ENV *,
		DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
	int  (*memp_sync) __P((DB_ENV *, DB_LSN *));
	int  (*memp_trickle) __P((DB_ENV *, int, int *));

	void *rep_handle;		/* Replication handle and methods. */
	int  (*rep_elect) __P((DB_ENV *, int, int, u_int32_t, int *));
	int  (*rep_process_message) __P((DB_ENV *, DBT *, DBT *, int *));
	int  (*rep_start) __P((DB_ENV *, DBT *, u_int32_t));
	int  (*set_rep_election) __P((DB_ENV *,
		u_int32_t, u_int32_t, u_int32_t, u_int32_t));
	int  (*set_rep_timeout) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_rep_transport) __P((DB_ENV *, int,
		int (*) (DB_ENV *, const DBT *, const DBT *, int, u_int32_t)));

	void *tx_handle;		/* Txn handle and methods. */
	int  (*set_tx_max) __P((DB_ENV *, u_int32_t));
	int  (*set_tx_recover) __P((DB_ENV *,
		int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
	int  (*set_tx_timestamp) __P((DB_ENV *, time_t *));
	int  (*txn_begin) __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
	int  (*txn_checkpoint) __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
	int  (*txn_id_set) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*txn_recover) __P((DB_ENV *,
		DB_PREPLIST *, long, long *, u_int32_t));
	int  (*txn_stat) __P((DB_ENV *, DB_TXN_STAT **, u_int32_t));
	int  (*set_timeout) __P((DB_ENV *, db_timeout_t, u_int32_t));

#define	DB_TEST_PREOPEN		 1	/* before __os_open */
#define	DB_TEST_POSTOPEN	 2	/* after __os_open */
#define	DB_TEST_POSTLOGMETA	 3	/* after logging meta in btree */
#define	DB_TEST_POSTLOG		 4	/* after logging all pages */
#define	DB_TEST_POSTSYNC	 5	/* after syncing the log */
#define	DB_TEST_PREDESTROY	 6	/* before destroy op */
#define	DB_TEST_POSTDESTROY	 7	/* after destroy op */
	int		 test_abort;	/* Abort value for testing. */
	int		 test_copy;	/* Copy value for testing. */

#define	DB_ENV_CDB		0x000001 /* DB_INIT_CDB. */
#define	DB_ENV_CDB_ALLDB	0x000002 /* CDB environment wide locking. */
#define	DB_ENV_CREATE		0x000004 /* DB_CREATE set. */
#define	DB_ENV_DBLOCAL		0x000008 /* DB_ENV allocated for private DB. */
#define	DB_ENV_LOCKDOWN		0x000010 /* DB_LOCKDOWN set. */
#define	DB_ENV_NOLOCKING	0x000020 /* DB_NOLOCKING set. */
#define	DB_ENV_NOMMAP		0x000040 /* DB_NOMMAP set. */
#define	DB_ENV_NOPANIC		0x000080 /* Removing env, okay if panic set */
#define	DB_ENV_OPEN_CALLED	0x000100 /* DB_ENV->open called (paths OK). */
#define	DB_ENV_PRIVATE		0x000200 /* DB_PRIVATE set. */
#define	DB_ENV_REGION_INIT	0x000400 /* DB_REGION_INIT set. */
#define	DB_ENV_REP_CLIENT	0x000800 /* Replication client. */
#define	DB_ENV_REP_LOGSONLY	0x001000 /* Log files only replication site. */
#define	DB_ENV_REP_MASTER	0x002000 /* Replication master. */
#define	DB_ENV_RPCCLIENT	0x004000 /* DB_CLIENT set. */
#define	DB_ENV_RPCCLIENT_GIVEN	0x008000 /* User-supplied RPC client struct */
#define	DB_ENV_STANDALONE	0x010000 /* Test: freestanding environment. */
#define	DB_ENV_SYSTEM_MEM	0x020000 /* DB_SYSTEM_MEM set. */
#define	DB_ENV_THREAD		0x040000 /* DB_THREAD set. */
#define	DB_ENV_TXN_NOSYNC	0x080000 /* DB_TXN_NOSYNC set. */
#define	DB_ENV_USER_ALLOC	0x100000 /* User allocated the structure. */
#define	DB_ENV_YIELDCPU		0x200000 /* DB_YIELDCPU set. */
	u_int32_t flags;
};

/*******************************************************
 * Locking.
 *******************************************************/
#define	DB_LOCKVERSION	1

/*
 * Deadlock detector modes; used in the DB_ENV structure to configure the
 * locking subsystem.
 */
#define	DB_LOCK_NORUN		0
#define	DB_LOCK_DEFAULT		1	/* Default policy. */
#define	DB_LOCK_EXPIRE		2	/* Only expire locks, no detection. */
#define	DB_LOCK_MAXLOCKS	3	/* Abort txn with maximum # of locks. */
#define	DB_LOCK_MINLOCKS	4	/* Abort txn with minimum # of locks. */
#define	DB_LOCK_MINWRITE	5	/* Abort txn with minimum writelocks. */
#define	DB_LOCK_OLDEST		6	/* Abort oldest transaction. */
#define	DB_LOCK_RANDOM		7	/* Abort random transaction. */
#define	DB_LOCK_YOUNGEST	8	/* Abort youngest transaction. */

/* Flag values for lock_vec(), lock_get(). */
#define	DB_LOCK_FREE_LOCKER	0x001	/* Internal: Free locker as well. */
#define	DB_LOCK_NOWAIT		0x002	/* Don't wait on unavailable lock. */
#define	DB_LOCK_RECORD		0x004	/* Internal: record lock. */
#define	DB_LOCK_SET_TIMEOUT	0x008	/* Internal: set lock timeout. */
#define	DB_LOCK_SWITCH		0x010	/* Internal: switch existing lock. */
#define	DB_LOCK_UPGRADE		0x020	/* Internal: upgrade existing lock. */

/*
 * Request types.
 */
typedef enum {
	DB_LOCK_DUMP=0,			/* Display held locks. */
	DB_LOCK_GET=1,			/* Get the lock. */
	DB_LOCK_GET_TIMEOUT=2,		/* Get lock with a timeout. */
	DB_LOCK_INHERIT=3,		/* Pass locks to parent. */
	DB_LOCK_PUT=4,			/* Release the lock. */
	DB_LOCK_PUT_ALL=5,		/* Release locker's locks. */
	DB_LOCK_PUT_OBJ=6,		/* Release locker's locks on obj. */
	DB_LOCK_PUT_READ=7,		/* Release locker's read locks. */
	DB_LOCK_TIMEOUT=8,		/* Force a txn to timeout. */
	DB_LOCK_UPGRADE_WRITE=9		/* Upgrade writes for dirty reads. */
} db_lockop_t;

/*
 * Status of a lock.
 */
typedef enum  {
	DB_LSTAT_ABORTED=1,		/* Lock belongs to an aborted txn. */
	DB_LSTAT_ERR=2,			/* Lock is bad. */
	DB_LSTAT_EXPIRED=3,		/* Lock has expired. */
	DB_LSTAT_FREE=4,		/* Lock is unallocated. */
	DB_LSTAT_HELD=5,		/* Lock is currently held. */
	DB_LSTAT_PENDING=6,		/* Lock was waiting and has been
					 * promoted; waiting for the owner
					 * to run and upgrade it to held. */
	DB_LSTAT_WAITING=7		/* Lock is on the wait queue. */
}db_status_t;

/* Lock statistics structure. */
struct __db_lock_stat {
	u_int32_t st_lastid;		/* Last allocated locker ID. */
	u_int32_t st_maxlocks;		/* Maximum number of locks in table. */
	u_int32_t st_maxlockers;	/* Maximum num of lockers in table. */
	u_int32_t st_maxobjects;	/* Maximum num of objects in table. */
	u_int32_t st_nmodes;		/* Number of lock modes. */
	u_int32_t st_nlocks;		/* Current number of locks. */
	u_int32_t st_maxnlocks;		/* Maximum number of locks so far. */
	u_int32_t st_nlockers;		/* Current number of lockers. */
	u_int32_t st_maxnlockers;	/* Maximum number of lockers so far. */
	u_int32_t st_nobjects;		/* Current number of objects. */
	u_int32_t st_maxnobjects;	/* Maximum number of objects so far. */
	u_int32_t st_nconflicts;	/* Number of lock conflicts. */
	u_int32_t st_nrequests;		/* Number of lock gets. */
	u_int32_t st_nreleases;		/* Number of lock puts. */
	u_int32_t st_nnowaits;		/* Number of requests that would have
					   waited, but NOWAIT was set. */
	u_int32_t st_ndeadlocks;	/* Number of lock deadlocks. */
	u_int32_t st_nlocktimeouts;	/* Number of lock timeouts. */
	u_int32_t st_ntxntimeouts;	/* Number of transaction timeouts. */
	u_int32_t st_region_wait;	/* Region lock granted after wait. */
	u_int32_t st_region_nowait;	/* Region lock granted without wait. */
	u_int32_t st_regsize;		/* Region size. */
};

/*
 * DB_LOCK_ILOCK --
 *	Internal DB access method lock.
 */
struct __db_ilock {
	db_pgno_t pgno;			/* Page being locked. */
	u_int8_t fileid[DB_FILE_ID_LEN];/* File id. */
#define	DB_RECORD_LOCK	1
#define	DB_PAGE_LOCK	2
	u_int8_t type;			/* Record or Page lock */
};

/*
 * DB_LOCK --
 *	The structure is allocated by the caller and filled in during a
 *	lock_get request (or a lock_vec/DB_LOCK_GET).
 */
struct __db_lock_u {
	size_t		off;		/* Offset of the lock in the region */
	u_int32_t	ndx;		/* Index of the object referenced by
					 * this lock; used for locking. */
	u_int32_t	gen;		/* Generation number of this lock. */
	db_lockmode_t	mode;		/* mode of this lock. */
};

/* Lock request structure. */
struct __db_lockreq {
	db_lockop_t	 op;		/* Operation. */
	db_lockmode_t	 mode;		/* Requested mode. */
	db_timeout_t	 timeout;	/* Time to expire lock. */
	DBT		*obj;		/* Object being locked. */
	DB_LOCK		 lock;		/* Lock returned. */
};

/*******************************************************
 * Logging.
 *******************************************************/
#define	DB_LOGVERSION	5		/* Current log version. */
#define	DB_LOGOLDVER	5		/* Oldest log version supported. */
#define	DB_LOGMAGIC	0x040988

/* Flag values for log_archive(). */
#define	DB_ARCH_ABS		0x001	/* Absolute pathnames. */
#define	DB_ARCH_DATA		0x002	/* Data files. */
#define	DB_ARCH_LOG		0x004	/* Log files. */

/*
 * A DB_LSN has two parts, a fileid which identifies a specific file, and an
 * offset within that file.  The fileid is an unsigned 4-byte quantity that
 * uniquely identifies a file within the log directory -- currently a simple
 * counter inside the log.  The offset is also an unsigned 4-byte value.  The
 * log manager guarantees the offset is never more than 4 bytes by switching
 * to a new log file before the maximum length imposed by an unsigned 4-byte
 * offset is reached.
 */
struct __db_lsn {
	u_int32_t	file;		/* File ID. */
	u_int32_t	offset;		/* File offset. */
};

/*
 * DB_LOGC --
 *	Log cursor.
 */
struct __db_log_cursor {
	DB_ENV	 *dbenv;		/* Enclosing dbenv. */

	DB_FH	 *c_fh;			/* File handle. */
	DB_LSN	  c_lsn;		/* Cursor: LSN */
	u_int32_t c_len;		/* Cursor: record length */
	u_int32_t c_prev;		/* Cursor: previous record's offset */

	DBT	  c_dbt;		/* Return DBT. */

#define	DB_LOGC_BUF_SIZE	(32 * 1024)
	u_int8_t *bp;			/* Allocated read buffer. */
	u_int32_t bp_size;		/* Read buffer length in bytes. */
	u_int32_t bp_rlen;		/* Read buffer valid data length. */
	DB_LSN	  bp_lsn;		/* Read buffer first byte LSN. */

					/* Methods. */
	int (*close) __P((DB_LOGC *, u_int32_t));
	int (*get) __P((DB_LOGC *, DB_LSN *, DBT *, u_int32_t));

#define	DB_LOG_DISK		0x01	/* Log record came from disk. */
#define	DB_LOG_LOCKED		0x02	/* Log region already locked */
#define	DB_LOG_SILENT_ERR	0x04	/* Turn-off error messages. */
	u_int32_t flags;
};

/* Log statistics structure. */
struct __db_log_stat {
	u_int32_t st_magic;		/* Log file magic number. */
	u_int32_t st_version;		/* Log file version number. */
	int st_mode;			/* Log file mode. */
	u_int32_t st_lg_bsize;		/* Log buffer size. */
	u_int32_t st_lg_max;		/* Maximum log file size. */
	u_int32_t st_w_bytes;		/* Bytes to log. */
	u_int32_t st_w_mbytes;		/* Megabytes to log. */
	u_int32_t st_wc_bytes;		/* Bytes to log since checkpoint. */
	u_int32_t st_wc_mbytes;		/* Megabytes to log since checkpoint. */
	u_int32_t st_wcount;		/* Total writes to the log. */
	u_int32_t st_wcount_fill;	/* Overflow writes to the log. */
	u_int32_t st_scount;		/* Total syncs to the log. */
	u_int32_t st_region_wait;	/* Region lock granted after wait. */
	u_int32_t st_region_nowait;	/* Region lock granted without wait. */
	u_int32_t st_cur_file;		/* Current log file number. */
	u_int32_t st_cur_offset;	/* Current log file offset. */
	u_int32_t st_disk_file;		/* Known on disk log file number. */
	u_int32_t st_disk_offset;	/* Known on disk log file offset. */
	u_int32_t st_regsize;		/* Region size. */
	u_int32_t st_flushcommit;	/* Flushes containing a commit. */
	u_int32_t st_maxcommitperflush;	/* Max number of commits in a flush. */
	u_int32_t st_mincommitperflush;	/* Min number of commits in a flush. */
};

/*******************************************************
 * Shared buffer cache (mpool).
 *******************************************************/
/* Flag values for DB_MPOOLFILE->get. */
#define	DB_MPOOL_CREATE		0x001	/* Create a page. */
#define	DB_MPOOL_LAST		0x002	/* Return the last page. */
#define	DB_MPOOL_NEW		0x004	/* Create a new page. */

/* Flag values for DB_MPOOLFILE->put, DB_MPOOLFILE->set. */
#define	DB_MPOOL_CLEAN		0x001	/* Page is not modified. */
#define	DB_MPOOL_DIRTY		0x002	/* Page is modified. */
#define	DB_MPOOL_DISCARD	0x004	/* Don't cache the page. */

/* Per-process DB_MPOOLFILE information. */
struct __db_mpoolfile {
	/* These fields need to be protected for multi-threaded support. */
	DB_MUTEX *mutexp;		/* Structure thread lock. */

	DB_FH	  *fhp;			/* Underlying file handle. */

	u_int32_t ref;			/* Reference count. */

	/*
	 * !!!
	 * This field is a special case -- it's protected by the region lock
	 * NOT the thread lock.  The reason for this is that we always have
	 * the region lock immediately before or after we modify the field,
	 * and we don't want to use the structure lock to protect it because
	 * then I/O (which is done with the structure lock held because of
	 * the race between the seek and write of the file descriptor) will
	 * block any other put/get calls using this DB_MPOOLFILE structure.
	 */
	u_int32_t pinref;		/* Pinned block reference count. */

	int	  ftype;		/* File type. */
	DBT	 *pgcookie;		/* Byte-string passed to pgin/pgout. */
	u_int8_t *fileid;		/* Unique file ID. */
	int32_t	  lsn_offset;		/* LSN offset in page. */
	u_int32_t clear_len;		/* Cleared length on created pages. */

	/*
	 * !!!
	 * This field is a special case -- it's protected by the region lock
	 * since it's manipulated only when new files are added to the list.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_mpoolfile) q;
	 */
	struct {
		struct __db_mpoolfile *tqe_next;
		struct __db_mpoolfile **tqe_prev;
	} q;				/* Linked list of DB_MPOOLFILE's. */

	/* These fields are not thread-protected. */
	DB_MPOOL  *dbmp;		/* Overlying DB_MPOOL. */
	MPOOLFILE *mfp;			/* Underlying MPOOLFILE. */

	void	  *addr;		/* Address of mmap'd region. */
	size_t	   len;			/* Length of mmap'd region. */

					/* Methods. */
	int  (*close) __P((DB_MPOOLFILE *, u_int32_t));
	int  (*get) __P((DB_MPOOLFILE *, db_pgno_t *, u_int32_t, void *));
	void (*last_pgno) __P((DB_MPOOLFILE *, db_pgno_t *));
	int  (*open)__P((DB_MPOOLFILE *, const char *, u_int32_t, int, size_t));
	int  (*put) __P((DB_MPOOLFILE *, void *, u_int32_t));
	void (*refcnt) __P((DB_MPOOLFILE *, db_pgno_t *));
	int  (*set) __P((DB_MPOOLFILE *, void *, u_int32_t));
	int  (*set_clear_len) __P((DB_MPOOLFILE *, u_int32_t));
	int  (*set_fileid) __P((DB_MPOOLFILE *, u_int8_t *));
	int  (*set_ftype) __P((DB_MPOOLFILE *, int));
	int  (*set_lsn_offset) __P((DB_MPOOLFILE *, int32_t));
	int  (*set_pgcookie) __P((DB_MPOOLFILE *, DBT *));
	void (*set_unlink) __P((DB_MPOOLFILE *, int));
	int  (*sync) __P((DB_MPOOLFILE *));

	/* These fields need to be protected for multi-threaded support. */
#define	MP_FLUSH	0x001		/* Was opened to flush a buffer. */
#define	MP_OPEN_CALLED	0x002		/* File opened. */
#define	MP_READONLY	0x004		/* File is readonly. */
#define	MP_UPGRADE	0x008		/* File descriptor is readwrite. */
#define	MP_UPGRADE_FAIL	0x010		/* Upgrade wasn't possible. */
	u_int32_t  flags;
};

/* Mpool statistics structure. */
struct __db_mpool_stat {
	u_int32_t st_cache_hit;		/* Pages found in the cache. */
	u_int32_t st_cache_miss;	/* Pages not found in the cache. */
	u_int32_t st_map;		/* Pages from mapped files. */
	u_int32_t st_page_create;	/* Pages created in the cache. */
	u_int32_t st_page_in;		/* Pages read in. */
	u_int32_t st_page_out;		/* Pages written out. */
	u_int32_t st_ro_evict;		/* Clean pages forced from the cache. */
	u_int32_t st_rw_evict;		/* Dirty pages forced from the cache. */
	u_int32_t st_hash_buckets;	/* Number of hash buckets. */
	u_int32_t st_hash_searches;	/* Total hash chain searches. */
	u_int32_t st_hash_longest;	/* Longest hash chain searched. */
	u_int32_t st_hash_examined;	/* Total hash entries searched. */
	u_int32_t st_page_clean;	/* Clean pages. */
	u_int32_t st_page_dirty;	/* Dirty pages. */
	u_int32_t st_page_trickle;	/* Pages written by memp_trickle. */
	u_int32_t st_region_wait;	/* Region lock granted after wait. */
	u_int32_t st_region_nowait;	/* Region lock granted without wait. */
	u_int32_t st_gbytes;		/* Total cache size: GB. */
	u_int32_t st_bytes;		/* Total cache size: B. */
	u_int32_t st_ncache;		/* Number of caches. */
	u_int32_t st_regsize;		/* Cache size. */
};

/* Mpool file statistics structure. */
struct __db_mpool_fstat {
	char *file_name;		/* File name. */
	size_t st_pagesize;		/* Page size. */
	u_int32_t st_cache_hit;		/* Pages found in the cache. */
	u_int32_t st_cache_miss;	/* Pages not found in the cache. */
	u_int32_t st_map;		/* Pages from mapped files. */
	u_int32_t st_page_create;	/* Pages created in the cache. */
	u_int32_t st_page_in;		/* Pages read in. */
	u_int32_t st_page_out;		/* Pages written out. */
};

/*******************************************************
 * Transactions.
 *******************************************************/
#define	DB_TXNVERSION	1

struct __db_txn {
	DB_TXNMGR	*mgrp;		/* Pointer to transaction manager. */
	DB_TXN		*parent;	/* Pointer to transaction's parent. */
	DB_LSN		last_lsn;	/* Lsn of last log write. */
	u_int32_t	txnid;		/* Unique transaction id. */
	roff_t		off;		/* Detail structure within region. */
	db_timeout_t	lock_timeout;	/* Timeout for locks for this txn. */
	db_timeout_t	expire;		/* Time this txn expires. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_txn) links;
	 */
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} links;			/* Links transactions off manager. */

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__kids, __db_txn) kids;
	 */
	struct __kids {
		struct __db_txn *tqh_first;
		struct __db_txn **tqh_last;
	} kids;

	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db_txn) klinks;
	 */
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} klinks;

	u_int32_t	cursors;	/* Number of cursors open for txn */

					/* Methods. */
	int	  (*abort) __P((DB_TXN *));
	int	  (*commit) __P((DB_TXN *, u_int32_t));
	int	  (*discard) __P((DB_TXN *, u_int32_t));
	u_int32_t (*id) __P((DB_TXN *));
	int	  (*prepare) __P((DB_TXN *, u_int8_t *));
	int	  (*set_timeout) __P((DB_TXN *, db_timeout_t, u_int32_t));

#define	TXN_CHILDCOMMIT	0x01		/* Transaction that has committed. */
#define	TXN_COMPENSATE	0x02		/* Compensating transaction. */
#define	TXN_DIRTY_READ	0x04		/* Transaction does dirty reads. */
#define	TXN_LOCKTIMEOUT	0x08		/* Transaction has a lock timeout. */
#define	TXN_MALLOC	0x10		/* Structure allocated by TXN system. */
#define	TXN_NOSYNC	0x20		/* Do not sync on prepare and commit. */
#define	TXN_NOWAIT	0x40		/* Do not wait on locks. */
#define	TXN_SYNC	0x80		/* Sync on prepare and commit. */
	u_int32_t	flags;
};

/* Transaction statistics structure. */
struct __db_txn_active {
	u_int32_t	txnid;		/* Transaction ID */
	u_int32_t	parentid;	/* Transaction ID of parent */
	DB_LSN		lsn;		/* Lsn of the begin record */
};

struct __db_txn_stat {
	DB_LSN	  st_last_ckp;		/* lsn of the last checkpoint */
	DB_LSN	  st_pending_ckp;	/* last checkpoint did not finish */
	time_t	  st_time_ckp;		/* time of last checkpoint */
	u_int32_t st_last_txnid;	/* last transaction id given out */
	u_int32_t st_maxtxns;		/* maximum txns possible */
	u_int32_t st_naborts;		/* number of aborted transactions */
	u_int32_t st_nbegins;		/* number of begun transactions */
	u_int32_t st_ncommits;		/* number of committed transactions */
	u_int32_t st_nactive;		/* number of active transactions */
	u_int32_t st_nrestores;		/* number of restored transactions
					   after recovery. */
	u_int32_t st_maxnactive;	/* maximum active transactions */
	DB_TXN_ACTIVE *st_txnarray;	/* array of active transactions */
	u_int32_t st_region_wait;	/* Region lock granted after wait. */
	u_int32_t st_region_nowait;	/* Region lock granted without wait. */
	u_int32_t st_regsize;		/* Region size. */
};

/*
 * Structure used for two phase commit interface.  Berkeley DB support for two
 * phase commit is compatible with the X/open XA interface.  The xa #define
 * XIDDATASIZE defines the size of a global transaction ID.  We have our own
 * version here which must have the same value.
 */
#define	DB_XIDDATASIZE	128
struct __db_preplist {
	DB_TXN	*txn;
	u_int8_t gid[DB_XIDDATASIZE];
};

/*******************************************************
 * Replication.
 *******************************************************/
/* Special, out-of-band environment IDs. */
#define	DB_EID_BROADCAST	-1
#define	DB_EID_INVALID		-2

/* rep_start flags values */
#define	DB_REP_CLIENT		0x001
#define	DB_REP_LOGSONLY		0x002
#define	DB_REP_MASTER		0x004

/*******************************************************
 * Access methods.
 *******************************************************/
typedef enum {
	DB_BTREE=1,
	DB_HASH=2,
	DB_RECNO=3,
	DB_QUEUE=4,
	DB_UNKNOWN=5			/* Figure it out on open. */
} DBTYPE;

#define	DB_BTREEVERSION	8		/* Current btree version. */
#define	DB_BTREEOLDVER	6		/* Oldest btree version supported. */
#define	DB_BTREEMAGIC	0x053162

#define	DB_HASHVERSION	7		/* Current hash version. */
#define	DB_HASHOLDVER	4		/* Oldest hash version supported. */
#define	DB_HASHMAGIC	0x061561

#define	DB_QAMVERSION	3		/* Current queue version. */
#define	DB_QAMOLDVER	1		/* Oldest queue version supported. */
#define	DB_QAMMAGIC	0x042253

/*
 * DB access method and cursor operation values.  Each value is an operation
 * code to which additional bit flags are added.
 */
#define	DB_AFTER		 1	/* c_put() */
#define	DB_APPEND		 2	/* put() */
#define	DB_BEFORE		 3	/* c_put() */
#define	DB_CACHED_COUNTS	 4	/* stat() */
#define	DB_CHECKPOINT		 5	/* log_put(), DB_LOGC->get() */
#define	DB_COMMIT		 6	/* log_put() (internal) */
#define	DB_CONSUME		 7	/* get() */
#define	DB_CONSUME_WAIT		 8	/* get() */
#define	DB_CURLSN		 9	/* log_put() */
#define	DB_CURRENT		10	/* c_get(), c_put(), DB_LOGC->get() */
#define	DB_FAST_STAT		11	/* stat() */
#define	DB_FIRST		12	/* c_get(), DB_LOGC->get() */
#define	DB_GET_BOTH		13	/* get(), c_get() */
#define	DB_GET_BOTHC		14	/* c_get() (internal) */
#define	DB_GET_BOTH_RANGE	15	/* get(), c_get() */
#define	DB_GET_RECNO		16	/* c_get() */
#define	DB_JOIN_ITEM		17	/* c_get(); do not do primary lookup */
#define	DB_KEYFIRST		18	/* c_put() */
#define	DB_KEYLAST		19	/* c_put() */
#define	DB_LAST			20	/* c_get(), DB_LOGC->get() */
#define	DB_NEXT			21	/* c_get(), DB_LOGC->get() */
#define	DB_NEXT_DUP		22	/* c_get() */
#define	DB_NEXT_NODUP		23	/* c_get() */
#define	DB_NODUPDATA		24	/* put(), c_put() */
#define	DB_NOOVERWRITE		25	/* put() */
#define	DB_NOSYNC		26	/* close() */
#define	DB_POSITION		27	/* c_dup() */
#define	DB_POSITIONI		28	/* c_dup() (internal) */
#define	DB_PREV			29	/* c_get(), DB_LOGC->get() */
#define	DB_PREV_NODUP		30	/* c_get(), DB_LOGC->get() */
#define	DB_RECORDCOUNT		31	/* stat() */
#define	DB_SET			32	/* c_get(), DB_LOGC->get() */
#define	DB_SET_LOCK_TIMEOUT	33	/* set_timout() */
#define	DB_SET_RANGE		34	/* c_get() */
#define	DB_SET_RECNO		35	/* get(), c_get() */
#define	DB_SET_TXN_NOW		36	/* set_timout() (internal) */
#define	DB_SET_TXN_TIMEOUT	37	/* set_timout() */
#define	DB_UPDATE_SECONDARY	38	/* c_get(), c_del() (internal) */
#define	DB_WRITECURSOR		39	/* cursor() */
#define	DB_WRITELOCK		40	/* cursor() (internal) */

/* This has to change when the max opcode hits 255. */
#define	DB_OPFLAGS_MASK	0x000000ff	/* Mask for operations flags. */
/*	DB_DIRTY_READ	0x01000000	   Dirty Read. */
#define	DB_FLUSH	0x02000000	/* Flush data to disk. */
#define	DB_MULTIPLE	0x04000000	/* Return multiple data values. */
#define	DB_MULTIPLE_KEY	0x08000000	/* Return multiple data/key pairs. */
#define	DB_RMW		0x10000000	/* Acquire write flag immediately. */

/*
 * DB (user visible) error return codes.
 *
 * !!!
 * For source compatibility with DB 2.X deadlock return (EAGAIN), use the
 * following:
 *	#include <errno.h>
 *	#define DB_LOCK_DEADLOCK EAGAIN
 *
 * !!!
 * We don't want our error returns to conflict with other packages where
 * possible, so pick a base error value that's hopefully not common.  We
 * document that we own the error name space from -30,800 to -30,999.
 */
/* Public error return codes. */
#define	DB_DONOTINDEX		(-30999)/* "Null" return from 2ndary callbk. */
#define	DB_INCOMPLETE		(-30998)/* Sync didn't finish. */
#define	DB_KEYEMPTY		(-30997)/* Key/data deleted or never created. */
#define	DB_KEYEXIST		(-30996)/* The key/data pair already exists. */
#define	DB_LOCK_DEADLOCK	(-30995)/* Deadlock. */
#define	DB_LOCK_NOTGRANTED	(-30994)/* Lock unavailable. */
#define	DB_NOSERVER		(-30993)/* Server panic return. */
#define	DB_NOSERVER_HOME	(-30992)/* Bad home sent to server. */
#define	DB_NOSERVER_ID		(-30991)/* Bad ID sent to server. */
#define	DB_NOTFOUND		(-30990)/* Key/data pair not found (EOF). */
#define	DB_OLD_VERSION		(-30989)/* Out-of-date version. */
#define	DB_PAGE_NOTFOUND	(-30988)/* Requested page not found. */
#define	DB_REP_DUPMASTER	(-30987)/* There are two masters. */
#define	DB_REP_HOLDELECTION	(-30986)/* Time to hold an election. */
#define	DB_REP_NEWMASTER	(-30985)/* We have learned of a new master. */
#define	DB_REP_NEWSITE		(-30984)/* New site entered system. */
#define	DB_REP_OUTDATED		(-30983)/* Site is too far behind master. */
#define	DB_REP_UNAVAIL		(-30982)/* Site cannot currently be reached. */
#define	DB_RUNRECOVERY		(-30981)/* Panic return. */
#define	DB_SECONDARY_BAD	(-30980)/* Secondary index corrupt. */
#define	DB_VERIFY_BAD		(-30979)/* Verify failed; bad format. */

/* DB (private) error return codes. */
#define	DB_ALREADY_ABORTED	(-30899)
#define	DB_DELETED		(-30898)/* Recovery file marked deleted. */
#define	DB_JAVA_CALLBACK	(-30897)/* Exception during a java callback. */
#define	DB_NEEDSPLIT		(-30896)/* Page needs to be split. */
#define	DB_SURPRISE_KID		(-30895)/* Child commit, but parent doesn't
					   know that it's a parent. */
#define	DB_SWAPBYTES		(-30894)/* Database needs byte swapping. */
#define	DB_TIMEOUT		(-30893)/* Timed out waiting for election. */
#define	DB_TXN_CKP		(-30892)/* Encountered ckp record in log. */
#define	DB_VERIFY_FATAL		(-30891)/* Fatal: DB->verify cannot proceed. */

/* Database handle. */
struct __db {
	/*******************************************************
	 * Public: owned by the application.
	 *******************************************************/
	u_int32_t pgsize;		/* Database logical page size. */

					/* Callbacks. */
	int (*db_append_recno) __P((DB *, DBT *, db_recno_t));
	void (*db_feedback) __P((DB *, int, int));
	int (*dup_compare) __P((DB *, const DBT *, const DBT *));

	void	*app_private;		/* Application-private handle. */

	/*******************************************************
	 * Private: owned by DB.
	 *******************************************************/
	DB_ENV	*dbenv;			/* Backing environment. */

	DBTYPE	 type;			/* DB access method type. */

	DB_MPOOLFILE *mpf;		/* Backing buffer pool. */

	DB_MUTEX *mutexp;		/* Synchronization for free threading */

	u_int8_t fileid[DB_FILE_ID_LEN];/* File's unique ID for locking. */

	u_int32_t adj_fileid;		/* File's unique ID for curs. adj. */

#define	DB_LOGFILEID_INVALID	-1
	int32_t	 log_fileid;		/* File's unique ID for logging. */
	db_pgno_t meta_pgno;		/* Meta page number */
	DB_TXN	*open_txn;		/* Transaction to protect creates. */

	long	 cl_id;			/* RPC: remote client id. */

	/*
	 * Returned data memory for DB->get() and friends.
	 */
	DBT	 my_rskey;		/* Secondary key. */
	DBT	 my_rkey;		/* [Primary] key. */
	DBT	 my_rdata;		/* Data. */

	/*
	 * !!!
	 * Some applications use DB but implement their own locking outside of
	 * DB.  If they're using fcntl(2) locking on the underlying database
	 * file, and we open and close a file descriptor for that file, we will
	 * discard their locks.  The DB_FCNTL_LOCKING flag to DB->open is an
	 * undocumented interface to support this usage which leaves any file
	 * descriptors we open until DB->close.  This will only work with the
	 * DB->open interface and simple caches, e.g., creating a transaction
	 * thread may open/close file descriptors this flag doesn't protect.
	 * Locking with fcntl(2) on a file that you don't own is a very, very
	 * unsafe thing to do.  'Nuff said.
	 */
	DB_FH	*saved_open_fhp;	/* Saved file handle. */

	/*
	 * Linked list of DBP's, used in the log's dbentry table to keep track
	 * of all open db handles for a given log id.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__db) links;
	 */
	struct {
		struct __db *tqe_next;
		struct __db **tqe_prev;
	} links;

	/*
	 * Linked list of DBP's, linked from the DB_ENV, used to keep track
	 * of all open db handles for cursor adjustment.
	 *
	 * XXX
	 * Eventually, this should be merged with "links" above.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_ENTRY(__db) dblistlinks;
	 */
	struct {
		struct __db *le_next;
		struct __db **le_prev;
	} dblistlinks;

	/*
	 * Cursor queues.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_HEAD(__cq_fq, __dbc) free_queue;
	 * TAILQ_HEAD(__cq_aq, __dbc) active_queue;
	 * TAILQ_HEAD(__cq_jq, __dbc) join_queue;
	 */
	struct __cq_fq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} free_queue;
	struct __cq_aq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} active_queue;
	struct __cq_jq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} join_queue;

	/*
	 * Secondary index support.
	 *
	 * Linked list of secondary indices -- set in the primary.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_HEAD(s_secondaries, __db);
	 */
	struct {
		struct __db *lh_first;
	} s_secondaries;

	/*
	 * List entries for secondaries, and reference count of how
	 * many threads are updating this secondary (see __db_c_put).
	 *
	 * !!!
	 * Note that these are synchronized by the primary's mutex, but
	 * filled in in the secondaries.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * LIST_ENTRY(__db) s_links;
	 */
	struct {
		struct __db *le_next;
		struct __db **le_prev;
	} s_links;
	u_int32_t s_refcnt;

	/* Secondary callback and free functions -- set in the secondary. */
	int	(*s_callback) __P((DB *, const DBT *, const DBT *, DBT *));

	/* Reference to primary -- set in the secondary. */
	DB	*s_primary;

	/*
	 * Subsystem-private structures.
	 */
	void	*bt_internal;		/* Btree/Recno access method private. */
	void	*cj_internal;		/* C++/Java private. */
	void	*h_internal;		/* Hash access method private. */
	void	*q_internal;		/* Queue access method private. */
	void	*xa_internal;		/* XA private. */

					/* Methods. */
	int  (*associate) __P((DB *, DB *, int (*)(DB *, const DBT *,
		const DBT *, DBT *), u_int32_t));
	int  (*close) __P((DB *, u_int32_t));
	int  (*cursor) __P((DB *, DB_TXN *, DBC **, u_int32_t));
	int  (*del) __P((DB *, DB_TXN *, DBT *, u_int32_t));
	void (*err) __P((DB *, int, const char *, ...));
	void (*errx) __P((DB *, const char *, ...));
	int  (*fd) __P((DB *, int *));
	int  (*get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*pget) __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
	int  (*get_byteswapped) __P((DB *, int *));
	int  (*get_type) __P((DB *, DBTYPE *));
	int  (*join) __P((DB *, DBC **, DBC **, u_int32_t));
	int  (*key_range) __P((DB *,
		DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t));
	int  (*open) __P((DB *,
		const char *, const char *, DBTYPE, u_int32_t, int));
	int  (*put) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*remove) __P((DB *, const char *, const char *, u_int32_t));
	int  (*rename) __P((DB *,
	    const char *, const char *, const char *, u_int32_t));
	int  (*truncate) __P((DB *, DB_TXN *, u_int32_t *, u_int32_t));
	int  (*set_append_recno) __P((DB *, int (*)(DB *, DBT *, db_recno_t)));
	int  (*set_alloc) __P((DB *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	int  (*set_cachesize) __P((DB *, u_int32_t, u_int32_t, int));
	int  (*set_dup_compare) __P((DB *,
	    int (*)(DB *, const DBT *, const DBT *)));
	void (*set_errcall) __P((DB *, void (*)(const char *, char *)));
	void (*set_errfile) __P((DB *, FILE *));
	void (*set_errpfx) __P((DB *, const char *));
	int  (*set_feedback) __P((DB *, void (*)(DB *, int, int)));
	int  (*set_flags) __P((DB *, u_int32_t));
	int  (*set_lorder) __P((DB *, int));
	int  (*set_pagesize) __P((DB *, u_int32_t));
	int  (*set_paniccall) __P((DB *, void (*)(DB_ENV *, int)));
	int  (*stat) __P((DB *, void *, u_int32_t));
	int  (*sync) __P((DB *, u_int32_t));
	int  (*upgrade) __P((DB *, const char *, u_int32_t));
	int  (*verify) __P((DB *,
	    const char *, const char *, FILE *, u_int32_t));

	int  (*set_bt_compare) __P((DB *,
	    int (*)(DB *, const DBT *, const DBT *)));
	int  (*set_bt_maxkey) __P((DB *, u_int32_t));
	int  (*set_bt_minkey) __P((DB *, u_int32_t));
	int  (*set_bt_prefix) __P((DB *,
	    size_t (*)(DB *, const DBT *, const DBT *)));

	int  (*set_h_ffactor) __P((DB *, u_int32_t));
	int  (*set_h_hash) __P((DB *,
	    u_int32_t (*)(DB *, const void *, u_int32_t)));
	int  (*set_h_nelem) __P((DB *, u_int32_t));

	int  (*set_re_delim) __P((DB *, int));
	int  (*set_re_len) __P((DB *, u_int32_t));
	int  (*set_re_pad) __P((DB *, int));
	int  (*set_re_source) __P((DB *, const char *));
	int  (*set_q_extentsize) __P((DB *, u_int32_t));

	int  (*db_am_remove) __P((DB *, const char *,
	    const char *, DB_LSN *, int (**)(DB *, void *), void **));
	int  (*db_am_rename) __P((DB *,
	    const char *, const char *, const char *));

	/*
	 * Never called; these are a place to save function pointers
	 * so that we can undo an associate.
	 */
	int  (*stored_get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*stored_close) __P((DB *, u_int32_t));

#define	DB_OK_BTREE	0x01
#define	DB_OK_HASH	0x02
#define	DB_OK_QUEUE	0x04
#define	DB_OK_RECNO	0x08
	u_int32_t	am_ok;		/* Legal AM choices. */

#define	DB_AM_DIRTY	0x000001	/* Support Dirty Reads. */
#define	DB_AM_DISCARD	0x000002	/* Discard any cached pages. */
#define	DB_AM_DUP	0x000004	/* DB_DUP. */
#define	DB_AM_DUPSORT	0x000008	/* DB_DUPSORT. */
#define	DB_AM_INMEM	0x000010	/* In-memory; no sync on close. */
#define	DB_AM_PGDEF	0x000020	/* Page size was defaulted. */
#define	DB_AM_RDONLY	0x000040	/* Database is readonly. */
#define	DB_AM_RECOVER	0x000080	/* DBP opened by recovery routine. */
#define	DB_AM_SECONDARY	0x000100	/* Database is a secondary index. */
#define	DB_AM_SUBDB	0x000200	/* Subdatabases supported. */
#define	DB_AM_SWAP	0x000400	/* Pages need to be byte-swapped. */
#define	DB_AM_TXN	0x000800	/* DBP was in a transaction. */
#define	DB_AM_VERIFYING	0x001000	/* DB handle is in the verifier. */
#define	DB_BT_RECNUM	0x002000	/* DB_RECNUM. */
#define	DB_BT_REVSPLIT	0x004000	/* DB_REVSPLITOFF. */
#define	DB_CL_WRITER	0x008000	/* Allow writes in client replica. */
#define	DB_DBM_ERROR	0x010000	/* Error in DBM/NDBM database. */
#define	DB_OPEN_CALLED	0x020000	/* DB->open called. */
#define	DB_RE_DELIMITER	0x040000	/* Variablen length delimiter set. */
#define	DB_RE_FIXEDLEN	0x080000	/* Fixed-length records. */
#define	DB_RE_PAD	0x100000	/* Fixed-length record pad. */
#define	DB_RE_RENUMBER	0x200000	/* DB_RENUMBER. */
#define	DB_RE_SNAPSHOT	0x400000	/* DB_SNAPSHOT. */
	u_int32_t flags;
};

/*
 * Macros for bulk get.  Note that wherever we use a DBT *, we explicitly
 * cast it; this allows the same macros to work with C++ Dbt *'s, as Dbt
 * is a subclass of struct DBT in C++.
 */
#define	DB_MULTIPLE_INIT(pointer, dbt)					\
	(pointer = (u_int8_t *)((DBT *)(dbt))->data +			\
	    ((DBT *)(dbt))->ulen - sizeof(u_int32_t))
#define	DB_MULTIPLE_NEXT(pointer, dbt, retdata, retdlen)		\
	do {								\
		if (*((u_int32_t *)(pointer)) == (u_int32_t)-1) {	\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retdata = (u_int8_t *)					\
		    ((DBT *)(dbt))->data + *(u_int32_t *)(pointer);	\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retdlen = *(u_int32_t *)(pointer);			\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		if (retdlen == 0 &&					\
		    retdata == (u_int8_t *)((DBT *)(dbt))->data)	\
			retdata = NULL;					\
	} while (0)
#define	DB_MULTIPLE_KEY_NEXT(pointer, dbt, retkey, retklen, retdata, retdlen) \
	do {								\
		if (*((u_int32_t *)(pointer)) == (u_int32_t)-1) {	\
			retdata = NULL;					\
			retkey = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retkey = (u_int8_t *)					\
		    ((DBT *)(dbt))->data + *(u_int32_t *)(pointer);	\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retklen = *(u_int32_t *)(pointer);			\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retdata = (u_int8_t *)					\
		    ((DBT *)(dbt))->data + *(u_int32_t *)(pointer);	\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retdlen = *(u_int32_t *)(pointer);			\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
	} while (0)

#define	DB_MULTIPLE_RECNO_NEXT(pointer, dbt, recno, retdata, retdlen)   \
	do {								\
		if (*((u_int32_t *)(pointer)) == (u_int32_t)-1) {	\
			recno = 0;					\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		recno = *(u_int32_t *)(pointer);			\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retdata = (u_int8_t *)					\
		    ((DBT *)(dbt))->data + *(u_int32_t *)(pointer);	\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
		retdlen = *(u_int32_t *)(pointer);			\
		(pointer) = (u_int32_t *)(pointer) - 1;			\
	} while (0)

/*******************************************************
 * Access method cursors.
 *******************************************************/
struct __dbc {
	DB *dbp;			/* Related DB access method. */
	DB_TXN	 *txn;			/* Associated transaction. */

	/*
	 * Active/free cursor queues.
	 *
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(__dbc) links;
	 */
	struct {
		DBC *tqe_next;
		DBC **tqe_prev;
	} links;

	/*
	 * The DBT *'s below are used by the cursor routines to return
	 * data to the user when DBT flags indicate that DB should manage
	 * the returned memory.  They point at a DBT containing the buffer
	 * and length that will be used, and "belonging" to the handle that
	 * should "own" this memory.  This may be a "my_*" field of this
	 * cursor--the default--or it may be the corresponding field of
	 * another cursor, a DB handle, a join cursor, etc.  In general, it
	 * will be whatever handle the user originally used for the current
	 * DB interface call.
	 */
	DBT	 *rskey;		/* Returned secondary key. */
	DBT	 *rkey;			/* Returned [primary] key. */
	DBT	 *rdata;		/* Returned data. */

	DBT	  my_rskey;		/* Space for returned secondary key. */
	DBT	  my_rkey;		/* Space for returned [primary] key. */
	DBT	  my_rdata;		/* Space for returned data. */

	u_int32_t lid;			/* Default process' locker id. */
	u_int32_t locker;		/* Locker for this operation. */
	DBT	  lock_dbt;		/* DBT referencing lock. */
	DB_LOCK_ILOCK lock;		/* Object to be locked. */
	DB_LOCK	  mylock;		/* Lock held on this cursor. */

	long	  cl_id;		/* Remote client id. */

	DBTYPE	  dbtype;		/* Cursor type. */

	DBC_INTERNAL *internal;		/* Access method private. */

	int (*c_close) __P((DBC *));	/* Methods: public. */
	int (*c_count) __P((DBC *, db_recno_t *, u_int32_t));
	int (*c_del) __P((DBC *, u_int32_t));
	int (*c_dup) __P((DBC *, DBC **, u_int32_t));
	int (*c_get) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*c_pget) __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
	int (*c_put) __P((DBC *, DBT *, DBT *, u_int32_t));

					/* Methods: private. */
	int (*c_am_bulk) __P((DBC *, DBT *, u_int32_t));
	int (*c_am_close) __P((DBC *, db_pgno_t, int *));
	int (*c_am_del) __P((DBC *));
	int (*c_am_destroy) __P((DBC *));
	int (*c_am_get) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*c_am_put) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*c_am_writelock) __P((DBC *));

	/* Private: for secondary indices. */
	int (*c_real_get) __P((DBC *, DBT *, DBT *, u_int32_t));

#define	DBC_ACTIVE	 0x0001		/* Cursor in use. */
#define	DBC_COMPENSATE	 0x0002		/* Cursor compensating, don't lock. */
#define	DBC_DIRTY_READ	 0x0004		/* Cursor supports dirty reads. */
#define	DBC_OPD		 0x0008		/* Cursor references off-page dups. */
#define	DBC_RECOVER	 0x0010		/* Recovery cursor; don't log/lock. */
#define	DBC_RMW		 0x0020		/* Acquire write flag in read op. */
#define	DBC_TRANSIENT	 0x0040		/* Cursor is transient. */
#define	DBC_WRITECURSOR	 0x0080		/* Cursor may be used to write (CDB). */
#define	DBC_WRITEDUP	 0x0100		/* idup'ed DBC_WRITECURSOR (CDB). */
#define	DBC_WRITER	 0x0200		/* Cursor immediately writing (CDB). */
#define	DBC_MULTIPLE	 0x0400		/* Return Multiple data. */
#define	DBC_MULTIPLE_KEY 0x0800		/* Return Multiple keys and data. */
	u_int32_t flags;
};

/* Key range statistics structure */
struct __key_range {
	double less;
	double equal;
	double greater;
};

/* Btree/Recno statistics structure. */
struct __db_bt_stat {
	u_int32_t bt_magic;		/* Magic number. */
	u_int32_t bt_version;		/* Version number. */
	u_int32_t bt_metaflags;		/* Metadata flags. */
	u_int32_t bt_nkeys;		/* Number of unique keys. */
	u_int32_t bt_ndata;		/* Number of data items. */
	u_int32_t bt_pagesize;		/* Page size. */
	u_int32_t bt_maxkey;		/* Maxkey value. */
	u_int32_t bt_minkey;		/* Minkey value. */
	u_int32_t bt_re_len;		/* Fixed-length record length. */
	u_int32_t bt_re_pad;		/* Fixed-length record pad. */
	u_int32_t bt_levels;		/* Tree levels. */
	u_int32_t bt_int_pg;		/* Internal pages. */
	u_int32_t bt_leaf_pg;		/* Leaf pages. */
	u_int32_t bt_dup_pg;		/* Duplicate pages. */
	u_int32_t bt_over_pg;		/* Overflow pages. */
	u_int32_t bt_free;		/* Pages on the free list. */
	u_int32_t bt_int_pgfree;	/* Bytes free in internal pages. */
	u_int32_t bt_leaf_pgfree;	/* Bytes free in leaf pages. */
	u_int32_t bt_dup_pgfree;	/* Bytes free in duplicate pages. */
	u_int32_t bt_over_pgfree;	/* Bytes free in overflow pages. */
};

/* Hash statistics structure. */
struct __db_h_stat {
	u_int32_t hash_magic;		/* Magic number. */
	u_int32_t hash_version;		/* Version number. */
	u_int32_t hash_metaflags;	/* Metadata flags. */
	u_int32_t hash_nkeys;		/* Number of unique keys. */
	u_int32_t hash_ndata;		/* Number of data items. */
	u_int32_t hash_pagesize;	/* Page size. */
	u_int32_t hash_nelem;		/* Original nelem specified. */
	u_int32_t hash_ffactor;		/* Fill factor specified at create. */
	u_int32_t hash_buckets;		/* Number of hash buckets. */
	u_int32_t hash_free;		/* Pages on the free list. */
	u_int32_t hash_bfree;		/* Bytes free on bucket pages. */
	u_int32_t hash_bigpages;	/* Number of big key/data pages. */
	u_int32_t hash_big_bfree;	/* Bytes free on big item pages. */
	u_int32_t hash_overflows;	/* Number of overflow pages. */
	u_int32_t hash_ovfl_free;	/* Bytes free on ovfl pages. */
	u_int32_t hash_dup;		/* Number of dup pages. */
	u_int32_t hash_dup_free;	/* Bytes free on duplicate pages. */
};

/* Queue statistics structure. */
struct __db_qam_stat {
	u_int32_t qs_magic;		/* Magic number. */
	u_int32_t qs_version;		/* Version number. */
	u_int32_t qs_metaflags;		/* Metadata flags. */
	u_int32_t qs_nkeys;		/* Number of unique keys. */
	u_int32_t qs_ndata;		/* Number of data items. */
	u_int32_t qs_pagesize;		/* Page size. */
	u_int32_t qs_extentsize;	/* Pages per extent. */
	u_int32_t qs_pages;		/* Data pages. */
	u_int32_t qs_re_len;		/* Fixed-length record length. */
	u_int32_t qs_re_pad;		/* Fixed-length record pad. */
	u_int32_t qs_pgfree;		/* Bytes free in data pages. */
	u_int32_t qs_first_recno;	/* First not deleted record. */
	u_int32_t qs_cur_recno;		/* Last allocated record number. */
};

#ifndef DB_DBM_HSEARCH
#define	DB_DBM_HSEARCH	0		/* No historic interfaces by default. */
#endif
#if DB_DBM_HSEARCH != 0
/*******************************************************
 * Dbm/Ndbm historic interfaces.
 *******************************************************/
typedef struct __db DBM;

#define	DBM_INSERT	0		/* Flags to dbm_store(). */
#define	DBM_REPLACE	1

/*
 * The DB support for ndbm(3) always appends this suffix to the
 * file name to avoid overwriting the user's original database.
 */
#define	DBM_SUFFIX	".db"

#if defined(_XPG4_2)
typedef struct {
	char *dptr;
	size_t dsize;
} datum;
#else
typedef struct {
	char *dptr;
	int dsize;
} datum;
#endif

/*******************************************************
 * Hsearch historic interface.
 *******************************************************/
typedef enum {
	FIND, ENTER
} ACTION;

typedef struct entry {
	char *key;
	char *data;
} ENTRY;

#endif /* DB_DBM_HSEARCH */

#if defined(__cplusplus)
}
#endif

/* DO NOT EDIT: automatically built by dist/s_rpc. */
#define	DB_RPC_SERVERPROG ((unsigned long)(351457))
#define	DB_RPC_SERVERVERS ((unsigned long)(4000))

/* DO NOT EDIT: automatically built by dist/s_include. */
#if defined(__cplusplus)
extern "C" {
#endif
int db_create __P((DB **, DB_ENV *, u_int32_t));
char *db_strerror __P((int));
int db_env_create __P((DB_ENV **, u_int32_t));
char *db_version __P((int *, int *, int *));
int log_compare __P((const DB_LSN *, const DB_LSN *));
int db_env_set_func_close __P((int (*)(int)));
int db_env_set_func_dirfree __P((void (*)(char **, int)));
int db_env_set_func_dirlist __P((int (*)(const char *, char ***, int *)));
int db_env_set_func_exists __P((int (*)(const char *, int *)));
int db_env_set_func_free __P((void (*)(void *)));
int db_env_set_func_fsync __P((int (*)(int)));
int db_env_set_func_ioinfo __P((int (*)(const char *, int, u_int32_t *, u_int32_t *, u_int32_t *)));
int db_env_set_func_malloc __P((void *(*)(size_t)));
int db_env_set_func_map __P((int (*)(char *, size_t, int, int, void **)));
int db_env_set_func_open __P((int (*)(const char *, int, ...)));
int db_env_set_func_read __P((ssize_t (*)(int, void *, size_t)));
int db_env_set_func_realloc __P((void *(*)(void *, size_t)));
int db_env_set_func_rename __P((int (*)(const char *, const char *)));
int db_env_set_func_seek __P((int (*)(int, size_t, db_pgno_t, u_int32_t, int, int)));
int db_env_set_func_sleep __P((int (*)(u_long, u_long)));
int db_env_set_func_unlink __P((int (*)(const char *)));
int db_env_set_func_unmap __P((int (*)(void *, size_t)));
int db_env_set_func_write __P((ssize_t (*)(int, const void *, size_t)));
int db_env_set_func_yield __P((int (*)(void)));
int txn_abort __P((DB_TXN *));
int txn_begin __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
int txn_commit __P((DB_TXN *, u_int32_t));
#if DB_DBM_HSEARCH != 0
int	 __db_ndbm_clearerr __P((DBM *));
void	 __db_ndbm_close __P((DBM *));
int	 __db_ndbm_delete __P((DBM *, datum));
int	 __db_ndbm_dirfno __P((DBM *));
int	 __db_ndbm_error __P((DBM *));
datum __db_ndbm_fetch __P((DBM *, datum));
datum __db_ndbm_firstkey __P((DBM *));
datum __db_ndbm_nextkey __P((DBM *));
DBM	*__db_ndbm_open __P((const char *, int, int));
int	 __db_ndbm_pagfno __P((DBM *));
int	 __db_ndbm_rdonly __P((DBM *));
int	 __db_ndbm_store __P((DBM *, datum, datum, int));
#define	dbm_clearerr(a)		__db_ndbm_clearerr(a)
#define	dbm_close(a)		__db_ndbm_close(a)
#define	dbm_delete(a, b)	__db_ndbm_delete(a, b)
#define	dbm_dirfno(a)		__db_ndbm_dirfno(a)
#define	dbm_error(a)		__db_ndbm_error(a)
#define	dbm_fetch(a, b)		__db_ndbm_fetch(a, b)
#define	dbm_firstkey(a)		__db_ndbm_firstkey(a)
#define	dbm_nextkey(a)		__db_ndbm_nextkey(a)
#define	dbm_open(a, b, c)	__db_ndbm_open(a, b, c)
#define	dbm_pagfno(a)		__db_ndbm_pagfno(a)
#define	dbm_rdonly(a)		__db_ndbm_rdonly(a)
#define	dbm_store(a, b, c, d)	__db_ndbm_store(a, b, c, d)
#define	dbminit(a)	__db_dbm_init(a)
#define	dbmclose	__db_dbm_close
#if !defined(__cplusplus)
#define	delete(a)	__db_dbm_delete(a)
#endif
#define	fetch(a)	__db_dbm_fetch(a)
#define	firstkey	__db_dbm_firstkey
#define	nextkey(a)	__db_dbm_nextkey(a)
#define	store(a, b)	__db_dbm_store(a, b)
int	 __db_dbm_close __P((void));
int	 __db_dbm_dbrdonly __P((void));
int	 __db_dbm_delete __P((datum));
int	 __db_dbm_dirf __P((void));
datum __db_dbm_fetch __P((datum));
datum __db_dbm_firstkey __P((void));
int	 __db_dbm_init __P((char *));
datum __db_dbm_nextkey __P((datum));
int	 __db_dbm_pagf __P((void));
int	 __db_dbm_store __P((datum, datum));
#endif
#if DB_DBM_HSEARCH != 0
#define hcreate(a)	__db_hcreate(a)
#define hdestroy	__db_hdestroy
#define hsearch(a, b)	__db_hsearch(a, b)
int __db_hcreate __P((size_t));
ENTRY *__db_hsearch __P((ENTRY, ACTION));
void __db_hdestroy __P((void));
#endif
#if defined(__cplusplus)
}
#endif

#endif /* !_DB_H_ */
