/* just include *all* the include files...it's slower but EASIER */

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../main.h"
#include "modvals.h"
#include "../tandem.h"

/*
 * this file contains all the orrible stuff required to do the lookup
 * table for symbols, rather than getting the OS to do it, since most OS's
 * require all symbols resolved, this can cause a problem with some modules
 * this is intimately related to the table in modules.c
 * don't change the files unless you have flamable underwear
 * do not read this file whilst unless heavily sedated, I will not be held
 * responsible for mental break-downs caused by this file <G>
 */

#undef nmalloc
#undef nfree
#undef nrealloc
#undef context
#undef contextnote
#undef feof

/* redefine for module-relevance */
/* 0 - 3 */
#define nmalloc(x) ((void *)(global[0]((x),MODULE_NAME,__FILE__,__LINE__)))
#define nfree(x) (global[1]((x),MODULE_NAME,__FILE__,__LINE__))
#define context (global[2](MODULE_NAME,__FILE__,__LINE__))
#define module_rename ((int (*)(char *, char *))global[3])
/* 4 - 7 */
#define module_register ((int (*)(char *, Function *, int, int))global[4])
#define module_find ((module_entry * (*)(char *,int,int))global[5])
#define module_depend ((Function *(*)(char *,char *,int,int))global[6])
#define module_undepend ((int(*)(char *))global[7])
/* 8 - 11 */
#define add_bind_table ((p_tcl_bind_list(*)(char *,int,Function))global[8])
#define del_bind_table ((void (*) (p_tcl_bind_list))global[9])
#define find_bind_table ((p_tcl_bind_list(*)(char *))global[10])
#define check_tcl_bind ((int (*) (p_tcl_bind_list,char *,struct flag_record *,char *, int))global[11])
/* 12 - 15 */
#define add_builtins ((int (*) (p_tcl_bind_list, cmd_t *,int))global[12])
#define rem_builtins ((int (*) (p_tcl_bind_list, cmd_t *,int))global[13])
#define add_tcl_commands ((void (*) (tcl_cmds *))global[14])
#define rem_tcl_commands ((void (*) (tcl_cmds *))global[15])
/* 16 - 19 */
#define add_tcl_ints ((void (*) (tcl_ints *))global[16])
#define rem_tcl_ints ((void (*) (tcl_ints *))global[17])
#define add_tcl_strings ((void (*) (tcl_strings *))global[18])
#define rem_tcl_strings ((void (*) (tcl_strings *))global[19])
/* 20 - 23 */
#define base64_to_int ((int (*) (char *))global[20])
#define int_to_base64 ((char * (*) (int))global[21])
#define int_to_base10 ((char * (*) (int))global[22])
#define simple_sprintf ((int (*)())global[23])
/* 24 - 27 */
#define botnet_send_zapf ((void (*)(int, char *, char *, char *))global[24])
#define botnet_send_zapf_broad ((void (*)(int, char *, char *, char *))global[25])
#define botnet_send_unlinked ((void (*)(int, char *, char *))global[26])
#define botnet_send_bye ((void(*)(void))global[27])
/* 28 - 31 */
#define botnet_send_chat ((void(*)(int,char*,char*))global[28])
#define botnet_send_filereject ((void(*)(int,char*,char*,char*))global[29])
#define botnet_send_filesend ((void(*)(int,char*,char*,char*))global[30])
#define botnet_send_filereq ((void(*)(int,char*,char*,char*))global[31])
/* 32 - 35 */
#define botnet_send_join_idx ((void(*)(int,int))global[32])
#define botnet_send_part_idx ((void(*)(int,char *))global[33])
#define updatebot ((void(*)(int,char*,char,int))global[34])
#define nextbot ((int (*)(char *))global[35])
/* 36 - 39 */
#define zapfbot ((void (*)(int))global[36])
#define n_free ((void (*)(void *,char *, int))global[37])
#define u_pass_match ((int (*)(struct userrec *,char *))global[38])
#define user_malloc(x) ((void *(*)(int,char *,int))global[39])(x,__FILE__,__LINE__)
/* 40 - 43 */
#define get_user ((void *(*)(struct user_entry_type *,struct userrec *))global[40])
#define set_user ((int(*)(struct user_entry_type *,struct userrec *,void *))global[41])
#define add_entry_type ((int (*) ( struct user_entry_type * ))global[42])
#define del_entry_type ((int (*) ( struct user_entry_type * ))global[43])
/* 44 - 47 */
#define get_user_flagrec ((void (*)(struct userrec *, struct flag_record *, char *))global[44])
#define set_user_flagrec ((void (*)(struct userrec *, struct flag_record *, char *))global[45])
#define get_user_by_host ((struct userrec * (*)(char *))global[46])
#define get_user_by_handle ((struct userrec *(*)(struct userrec *,char *))global[47])
/* 48 - 51 */
#define find_entry_type ((struct user_entry_type * (*) ( char * ))global[48])
#define find_user_entry ((struct user_entry * (*)( struct user_entry_type *, struct userrec *))global[49])
#define adduser ((struct userrec *(*)(struct userrec *,char*,char*,char*,int))global[50])
#define deluser ((int (*)(char *))global[51])
/* 52 - 55 */
#define addhost_by_handle ((void (*) (char *, char *))global[52])
#define delhost_by_handle ((int(*)(char *,char *))global[53])
#define readuserfile ((int (*)(char *,struct userrec **))global[54])
#define write_userfile ((void(*)(int))global[55])
/* 56 - 59 */
#define geticon ((char (*) (int))global[56])
#define clear_chanlist ((void (*)(void))global[57])
#define reaffirm_owners ((void (*)(void))global[58])
#define change_handle ((int(*)(struct userrec *,char*))global[59])
/* 60 - 63 */
#define write_user ((int (*)(struct userrec *, FILE *,int))global[60])
#define clear_userlist ((void (*)(struct userrec *))global[61])
#define count_users ((int(*)(struct userrec *))global[62])
#define sanity_check ((int(*)(int))global[63])
/* 64 - 67 */
#define break_down_flags ((void (*)(char *,struct flag_record *,struct flag_record *))global[64])
#define build_flags ((void (*)(char *, struct flag_record *, struct flag_record *))global[65])
#define flagrec_eq ((int(*)(struct flag_record*,struct flag_record *))global[66])
#define flagrec_ok ((int(*)(struct flag_record*,struct flag_record *))global[67])
/* 68 - 71 */
#define shareout (*(Function *)(global[68]))
#define dprintf (global[69])
#define chatout (global[70])
#define chanout_but ((void(*)())global[71])
/* 72 - 75 */
#define check_validity ((int (*) (char *,Function))global[72])
#define list_delete ((int (*)( struct list_type **, struct list_type *))global[73])
#define list_append ((int (*) ( struct list_type **, struct list_type *))global[74])
#define list_contains ((int (*) (struct list_type *, struct list_type *))global[75])
/* 76 - 79 */
#define answer ((int (*) (int,char *,unsigned long *,unsigned short *,int))global[76])
#define getmyip ((IP (*) (void))global[77])
#define neterror ((void (*) (char *))global[78])
#define tputs ((void (*) (int, char *,unsigned int))global[79])
/* 80 - 83 */
#define new_dcc ((int (*) (struct dcc_table *, int))global[80])
#define lostdcc ((void (*) (int))global[81])
#define getsock ((int (*) (int))global[82])
#define killsock ((void (*) (int))global[83])
/* 84 - 87 */
#define open_listen ((int (*) (int *))global[84])
#define open_telnet_dcc ((int (*) (int,char *,char *))global[85])
#define get_data_ptr(x) ((void *(*)(int,char*,int))global[86])(x,__FILE__,__LINE__)
#define open_telnet ((int (*) (char *, int))global[87])
/* 88 - 91 */
#ifndef HAVE_BZERO
#define bzero ((void (*) (void *, int))global[88])
#endif
#define my_memcpy ((void * (*) (void *, const void *, size_t))global[89])
#define my_atoul ((IP(*)(char *))global[90])
#define my_strcpy ((int (*)(char *, const char *))global[91])
/* 92 - 95 */
#define dcc (*(struct dcc_t **)global[92])
#define chanset (*(struct chanset_t **)(global[93]))
#define userlist (*(struct userrec **)global[94])
#define lastuser (*(struct userrec **)(global[95]))
/* 96 - 99 */
#define global_bans (*(maskrec **)(global[96]))
#define global_ign (*(struct igrec **)(global[97]))
#define password_timeout (*(int *)(global[98]))
#define share_greet (*(int *)global[99])
/* 100 - 103 */
#define max_dcc (*(int *)global[100])
#define require_p (*(int *)global[101])
#define use_silence (*(int *)(global[102]))
#define use_console_r (*(int *)(global[103]))
/* 104 - 107 */
#define ignore_time (*(int *)(global[104]))
#define reserved_port (*(int *)(global[105]))
#define debug_output (*(int *)(global[106]))
#define noshare (*(int *)(global[107]))
/* 108 - 111 */
#define gban_total (*(int*)global[108])
#define make_userfile (*(int*)global[109])
#define default_flags (*(int*)global[110])
#define dcc_total (*(int*)global[111])
/* 112 - 115 */
#define tempdir ((char *)(global[112]))
#define natip ((char *)(global[113]))
#define hostname ((char *)(global[114]))
#define origbotname ((char *)(global[115]))
/* 116 - 119 */
#define botuser ((char *)(global[116]))
#define admin ((char *)(global[117]))
#define userfile ((char *)global[118])
#define ver ((char *)global[119])
/* 120 - 123 */
#define notify_new ((char *)global[120])
#define helpdir ((char *)global[121])
#define Version ((char *)global[122])
#define botnetnick ((char *)global[123])
/* 124 - 127 */
#define DCC_CHAT_PASS (*(struct dcc_table *)(global[124]))
#define DCC_BOT (*(struct dcc_table *)(global[125]))
#define DCC_LOST (*(struct dcc_table *)(global[126]))
#define DCC_CHAT (*(struct dcc_table *)(global[127]))
/* 128 - 131 */
#define interp (*(Tcl_Interp **)(global[128]))
#define now (*(time_t*)global[129])
#define findanyidx ((int (*)(int))global[130])
#define findchan ((struct chanset_t *(*)(char *))global[131])
/* 132 - 135 */
#define cmd_die (global[132])
#define days ((void (*)(time_t,time_t,char *))global[133])
#define daysago ((void (*)(time_t,time_t,char *))global[134])
#define daysdur ((void (*)(time_t,time_t,char *))global[135])
/* 136 - 139 */
#define ismember ((memberlist * (*) (struct chanset_t *, char *))global[136])
#define newsplit ((char *(*)(char **))global[137])
#define splitnick ((char *(*)(char **))global[138])
#define splitc ((void (*)(char *,char *,char))global[139])
/* 140 - 143 */
#define addignore ((void (*) (char *, char *, char *,time_t))global[140])
#define match_ignore ((int (*)(char *))global[141])
#define delignore ((int (*)(char *))global[142])
#define fatal (global[143])
/* 144 - 147 */
#define xtra_kill ((void (*)(struct user_entry *))global[144])
#define xtra_unpack ((void (*)(struct userrec *, struct user_entry *))global[145])
#define movefile ((int (*) (char *, char *))global[146])
#define copyfile ((int (*) (char *, char *))global[147])
/* 148 - 151 */
#define do_tcl ((void (*)(char *,char*))global[148])
#define readtclprog ((int (*)(char *))global[149])
#define get_language ((char *(*)(int))global[150])
#define def_get ((void *(*)(struct userrec *, struct user_entry *))global[151])
/* 152 - 155 */
#define makepass ((void (*) (char *))global[152])
#define wild_match ((int (*)(const char *, const char *))global[153])
#define maskhost ((void(*)(char *,char*))global[154])
#define show_motd ((void(*)(int))global[155])
/* 156 - 159 */
#define tellhelp ((void(*)(int,char *,struct flag_record *,int))global[156])
#define showhelp ((void(*)(char *,char *,struct flag_record *,int))global[157])
#define add_help_reference ((void(*)(char *))global[158])
#define rem_help_reference ((void(*)(char *))global[159])
/* 160 - 163 */
#define touch_laston ((void (*)(struct userrec *,char *,time_t))global[160])
#define add_mode ((void (*)(struct chanset_t *,char,char,char *))(*(Function**)(global[161])))
#define rmspace ((void (*)(char *))global[162])
#define in_chain ((int (*)(char *))global[163])
/* 164 - 167 */
#define add_note ((int (*)(char *,char*,char*,int,int))global[164])
#define cmd_note (global[165])
#define detect_dcc_flood ((int (*) (time_t *,struct chat_info *,int))global[166])
#define flush_lines ((void(*)(int,struct chat_info*))global[167])
/* 168 - 171 */
#define expected_memory ((int(*)(void))global[168])
#define tell_mem_status ((void(*)(char *))global[169])
#define do_restart (*(int *)(global[170]))
#define check_tcl_filt ((char *(*)(int, char *))global[171])
/* 172 - 175 */
#define add_hook(a,b) (((void (*) (int, void *))global[172])(a,b))
#define del_hook(a,b) (((void (*) (int, void *))global[173])(a,b))
#define H_dcc (*(p_tcl_bind_list *)(global[174]))
#define H_filt (*(p_tcl_bind_list *)(global[175]))
/* 176 - 179 */
#define H_chon (*(p_tcl_bind_list *)(global[176]))
#define H_chof (*(p_tcl_bind_list *)(global[177]))
#define H_load (*(p_tcl_bind_list *)(global[178]))
#define H_unld (*(p_tcl_bind_list *)(global[179]))
/* 180 - 183 */
#define H_chat (*(p_tcl_bind_list *)(global[180]))
#define H_act (*(p_tcl_bind_list *)(global[181]))
#define H_bcst (*(p_tcl_bind_list *)(global[182]))
#define H_bot (*(p_tcl_bind_list *)(global[183]))
/* 184 - 187 */
#define H_link (*(p_tcl_bind_list *)(global[184]))
#define H_disc (*(p_tcl_bind_list *)(global[185]))
#define H_away (*(p_tcl_bind_list *)(global[186]))
#define H_nkch (*(p_tcl_bind_list *)(global[187]))
/* 188 - 191 */
#define USERENTRY_BOTADDR (*(struct user_entry_type *)(global[188]))
#define USERENTRY_BOTFL (*(struct user_entry_type *)(global[189]))
#define USERENTRY_HOSTS (*(struct user_entry_type *)(global[190]))
#define USERENTRY_PASS (*(struct user_entry_type *)(global[191]))
/* 192 - 195 */
#define USERENTRY_XTRA (*(struct user_entry_type *)(global[192]))

#define USERENTRY_INFO (*(struct user_entry_type *)(global[194]))
#define USERENTRY_COMMENT (*(struct user_entry_type *)(global[195]))
/* 196 - 199 */
#define USERENTRY_LASTON (*(struct user_entry_type *)(global[196]))
#define putlog (global[197])
#define botnet_send_chan ((void(*)(int,char*,char*,int,char*))global[198])
#define list_type_kill ((void(*)(struct list_type *))global[199])
/* 200 - 203 */
#define logmodes ((int(*)(char *))global[200])
#define masktype ((const char *(*)(int))global[201])
#define stripmodes ((int(*)(char *))global[202])
#define stripmasktype ((const char *(*)(int))global[203])
/* 204 - 207 */
#define sub_lang ((void(*)(int,char *))global[204])
#define online_since (*(int *)(global[205]))
#define cmd_loadlanguage ((int (*)(struct userrec *,int,char *))global[206])
#define check_dcc_attrs ((int (*)(struct userrec *,int))global[207])
/* 208 - 211 */
#define check_dcc_chanattrs ((int (*)(struct userrec *,char *,int,int))global[208])
#define add_tcl_coups ((void (*) (tcl_coups *))global[209])
#define rem_tcl_coups ((void (*) (tcl_coups *))global[210])
#define botname ((char *)(global[211]))
/* 212 - 215 */
#define remove_gunk ((void(*)(char *))global[212])
#define check_tcl_chjn ((void (*) (char *,char *,int,char,int,char *))global[213])
#define sanitycheck_dcc ((int (*)(char *, char *, char *, char *))global[214])
#define isowner ((int (*)(char *))global[215])
/* 216 - 219 */
#define min_dcc_port (*(int *)(global[216]))	/* dcc-portrange dw/guppy */
#define max_dcc_port (*(int *)(global[217]))
#define rfc_casecmp ((int(*)(char *, char *))global[218])
#define rfc_ncasecmp ((int(*)(char *, char *, int *))global[219])
/* 220 - 223 */
#define global_exempts (*(maskrec **)(global[220]))
#define global_invites (*(maskrec **)(global[221]))
#define ginvite_total (*(int*)global[222])
#define gexempt_total (*(int*)global[223])
/* 224 - 227 */
#define H_event (*(p_tcl_bind_list *)(global[224]))
#define use_exempts (*(int *)(global[225]))	/* drummer/Jason */
#define use_invites (*(int *)(global[226]))	/* drummer/Jason */
#define force_expire (*(int *)(global[227]))	/* Rufus */
/* 228 - 231 */
#define add_lang_section ((void(*)(char *))global[228])
#define user_realloc(x,y) ((void *(*)(void *,int,char *,int))global[229])((x),(y),__FILE__,__LINE__)
#define nrealloc(x,y) ((void *)(global[230]((x),(y),MODULE_NAME,__FILE__,__LINE__)))
#define xtra_set ((int(*)(struct userrec *,struct user_entry *, void *))global[231])

/* time to let modules use contextnote <cybah> */
#define contextnote(note) (global[230](MODULE_NAME, __FILE__, __LINE__, note))

/* this is for blowfish module, couldnt be bothereed making a whole new .h
 * file for it ;) */
#ifndef MAKING_BLOWFISH
#define encrypt_string(a,b) (((char *(*)(char *,char*))blowfish_funcs[4])(a,b))
#define decrypt_string(a,b) (((char *(*)(char *,char*))blowfish_funcs[5])(a,b))
#endif
