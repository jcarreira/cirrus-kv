/* ktutil_ct.c - automatically generated from ktutil_ct.ct */
#include <ss/ss.h>

#ifndef __STDC__
#define const
#endif

static char const * const ssu00001[] = {
"clear_list",
    "clear",
    (char const *)0
};
extern void ktutil_clear_list __SS_PROTO;
static char const * const ssu00002[] = {
"read_kt",
    "rkt",
    (char const *)0
};
extern void ktutil_read_v5 __SS_PROTO;
static char const * const ssu00003[] = {
"read_st",
    "rst",
    (char const *)0
};
extern void ktutil_read_v4 __SS_PROTO;
static char const * const ssu00004[] = {
"write_kt",
    "wkt",
    (char const *)0
};
extern void ktutil_write_v5 __SS_PROTO;
static char const * const ssu00005[] = {
"write_st",
    "wst",
    (char const *)0
};
extern void ktutil_write_v4 __SS_PROTO;
static char const * const ssu00006[] = {
"add_entry",
    "addent",
    (char const *)0
};
extern void ktutil_add_entry __SS_PROTO;
static char const * const ssu00007[] = {
"delete_entry",
    "delent",
    (char const *)0
};
extern void ktutil_delete_entry __SS_PROTO;
static char const * const ssu00008[] = {
"list",
    "l",
    (char const *)0
};
extern void ktutil_list __SS_PROTO;
static char const * const ssu00009[] = {
"list_requests",
    "lr",
    "?",
    (char const *)0
};
extern void ss_list_requests __SS_PROTO;
static char const * const ssu00010[] = {
"quit",
    "exit",
    "q",
    (char const *)0
};
extern void ss_quit __SS_PROTO;
static ss_request_entry ssu00011[] = {
    { ssu00001,
      ktutil_clear_list,
      "Clear the current keylist.",
      0 },
    { ssu00002,
      ktutil_read_v5,
      "Read a krb5 keytab into the current keylist.",
      0 },
    { ssu00003,
      ktutil_read_v4,
      "Read a krb4 srvtab into the current keylist.",
      0 },
    { ssu00004,
      ktutil_write_v5,
      "Write the current keylist to a krb5 keytab.",
      0 },
    { ssu00005,
      ktutil_write_v4,
      "Write the current keylist to a krb4 srvtab.",
      0 },
    { ssu00006,
      ktutil_add_entry,
      "Add an entry to the current keylist.",
      0 },
    { ssu00007,
      ktutil_delete_entry,
      "Delete an entry from the current keylist.",
      0 },
    { ssu00008,
      ktutil_list,
      "List the current keylist.",
      0 },
    { ssu00009,
      ss_list_requests,
      "List available requests.",
      0 },
    { ssu00010,
      ss_quit,
      "Exit program.",
      0 },
    { 0, 0, 0, 0 }
};

ss_request_table ktutil_cmds = { 2, ssu00011 };
