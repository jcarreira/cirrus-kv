/*
 * et-h-krb524_err.h:
 * This file is automatically generated; please do not edit it.
 */

#include <com_err.h>

#define KRB524_BADKEY                            (-1750206208L)
#define KRB524_BADADDR                           (-1750206207L)
#define KRB524_BADPRINC                          (-1750206206L)
#define KRB524_BADREALM                          (-1750206205L)
#define KRB524_V4ERR                             (-1750206204L)
#define KRB524_ENCFULL                           (-1750206203L)
#define KRB524_DECEMPTY                          (-1750206202L)
#define KRB524_NOTRESP                           (-1750206201L)
#define KRB524_KRB4_DISABLED                     (-1750206200L)
#define ERROR_TABLE_BASE_k524 (-1750206208L)

extern const struct error_table et_k524_error_table;

#if !defined(_WIN32)
/* for compatibility with older versions... */
extern void initialize_k524_error_table (void) /*@modifies internalState@*/;
#else
#define initialize_k524_error_table()
#endif

#if !defined(_WIN32)
#define init_k524_err_tbl initialize_k524_error_table
#define k524_err_base ERROR_TABLE_BASE_k524
#endif
