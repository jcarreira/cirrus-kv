/* kadmin_ct.c - automatically generated from kadmin_ct.ct */
#include <ss/ss.h>

#ifndef __STDC__
#define const
#endif

static char const * const ssu00001[] = {
"add_principal",
    "addprinc",
    "ank",
    (char const *)0
};
extern void kadmin_addprinc __SS_PROTO;
static char const * const ssu00002[] = {
"delete_principal",
    "delprinc",
    (char const *)0
};
extern void kadmin_delprinc __SS_PROTO;
static char const * const ssu00003[] = {
"modify_principal",
    "modprinc",
    (char const *)0
};
extern void kadmin_modprinc __SS_PROTO;
static char const * const ssu00004[] = {
"rename_principal",
    "renprinc",
    (char const *)0
};
extern void kadmin_renameprinc __SS_PROTO;
static char const * const ssu00005[] = {
"change_password",
    "cpw",
    (char const *)0
};
extern void kadmin_cpw __SS_PROTO;
static char const * const ssu00006[] = {
"get_principal",
    "getprinc",
    (char const *)0
};
extern void kadmin_getprinc __SS_PROTO;
static char const * const ssu00007[] = {
"list_principals",
    "listprincs",
    "get_principals",
    "getprincs",
    (char const *)0
};
extern void kadmin_getprincs __SS_PROTO;
static char const * const ssu00008[] = {
"add_policy",
    "addpol",
    (char const *)0
};
extern void kadmin_addpol __SS_PROTO;
static char const * const ssu00009[] = {
"modify_policy",
    "modpol",
    (char const *)0
};
extern void kadmin_modpol __SS_PROTO;
static char const * const ssu00010[] = {
"delete_policy",
    "delpol",
    (char const *)0
};
extern void kadmin_delpol __SS_PROTO;
static char const * const ssu00011[] = {
"get_policy",
    "getpol",
    (char const *)0
};
extern void kadmin_getpol __SS_PROTO;
static char const * const ssu00012[] = {
"list_policies",
    "listpols",
    "get_policies",
    "getpols",
    (char const *)0
};
extern void kadmin_getpols __SS_PROTO;
static char const * const ssu00013[] = {
"get_privs",
    "getprivs",
    (char const *)0
};
extern void kadmin_getprivs __SS_PROTO;
static char const * const ssu00014[] = {
"ktadd",
    "xst",
    (char const *)0
};
extern void kadmin_keytab_add __SS_PROTO;
static char const * const ssu00015[] = {
"ktremove",
    "ktrem",
    (char const *)0
};
extern void kadmin_keytab_remove __SS_PROTO;
static char const * const ssu00016[] = {
"lock",
    (char const *)0
};
extern void kadmin_lock __SS_PROTO;
static char const * const ssu00017[] = {
"unlock",
    (char const *)0
};
extern void kadmin_unlock __SS_PROTO;
static char const * const ssu00018[] = {
"purgekeys",
    (char const *)0
};
extern void kadmin_purgekeys __SS_PROTO;
static char const * const ssu00019[] = {
"get_strings",
    "getstrs",
    (char const *)0
};
extern void kadmin_getstrings __SS_PROTO;
static char const * const ssu00020[] = {
"set_string",
    "setstr",
    (char const *)0
};
extern void kadmin_setstring __SS_PROTO;
static char const * const ssu00021[] = {
"del_string",
    "delstr",
    (char const *)0
};
extern void kadmin_delstring __SS_PROTO;
static char const * const ssu00022[] = {
"list_requests",
    "lr",
    "?",
    (char const *)0
};
extern void ss_list_requests __SS_PROTO;
static char const * const ssu00023[] = {
"quit",
    "exit",
    "q",
    (char const *)0
};
extern void ss_quit __SS_PROTO;
static ss_request_entry ssu00024[] = {
    { ssu00001,
      kadmin_addprinc,
      "Add principal",
      0 },
    { ssu00002,
      kadmin_delprinc,
      "Delete principal",
      0 },
    { ssu00003,
      kadmin_modprinc,
      "Modify principal",
      0 },
    { ssu00004,
      kadmin_renameprinc,
      "Rename principal",
      0 },
    { ssu00005,
      kadmin_cpw,
      "Change password",
      0 },
    { ssu00006,
      kadmin_getprinc,
      "Get principal",
      0 },
    { ssu00007,
      kadmin_getprincs,
      "List principals",
      0 },
    { ssu00008,
      kadmin_addpol,
      "Add policy",
      0 },
    { ssu00009,
      kadmin_modpol,
      "Modify policy",
      0 },
    { ssu00010,
      kadmin_delpol,
      "Delete policy",
      0 },
    { ssu00011,
      kadmin_getpol,
      "Get policy",
      0 },
    { ssu00012,
      kadmin_getpols,
      "List policies",
      0 },
    { ssu00013,
      kadmin_getprivs,
      "Get privileges",
      0 },
    { ssu00014,
      kadmin_keytab_add,
      "Add entry(s) to a keytab",
      0 },
    { ssu00015,
      kadmin_keytab_remove,
      "Remove entry(s) from a keytab",
      0 },
    { ssu00016,
      kadmin_lock,
      "Lock database exclusively (use with extreme caution!)",
      0 },
    { ssu00017,
      kadmin_unlock,
      "Release exclusive database lock",
      0 },
    { ssu00018,
      kadmin_purgekeys,
      "Purge previously retained old keys from a principal",
      0 },
    { ssu00019,
      kadmin_getstrings,
      "Show string attributes on a principal",
      0 },
    { ssu00020,
      kadmin_setstring,
      "Set a string attribute on a principal",
      0 },
    { ssu00021,
      kadmin_delstring,
      "Delete a string attribute on a principal",
      0 },
    { ssu00022,
      ss_list_requests,
      "List available requests.",
      0 },
    { ssu00023,
      ss_quit,
      "Exit program.",
      0 },
    { 0, 0, 0, 0 }
};

ss_request_table kadmin_cmds = { 2, ssu00024 };
