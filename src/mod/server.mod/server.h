#define check_tcl_ctcp(a,b,c,d,e,f) check_tcl_ctcpr(a,b,c,d,e,f,H_ctcp)
#define check_tcl_ctcr(a,b,c,d,e,f) check_tcl_ctcpr(a,b,c,d,e,f,H_ctcr)
#ifndef MAKING_SERVER
/* 4 - 7 */

#define botuserhost ((char *)(server_funcs[5]))
#define quiet_reject (*(int *)(server_funcs[6]))
#define serv (*(int *)(server_funcs[7]))
/* 8 - 11 */
#define flud_thr (*(int*)(server_funcs[8]))
#define flud_time (*(int*)(server_funcs[9]))
#define flud_ctcp_thr (*(int*)(server_funcs[10]))
#define flud_ctcp_time (*(int*)(server_funcs[11]))
/* 12 - 15 */
#define match_my_nick ((int(*)(char *))server_funcs[12])
#define check_tcl_flud ((int (*)(char *,char *,struct userrec *,char *,char *))server_funcs[13])
#define fixfrom ((void (*)(char *))server_funcs[14])
#define answer_ctcp (*(int *)(server_funcs[15]))
/* 16 - 19 */
#define trigger_on_ignore (*(int *)(server_funcs[16]))
#define check_tcl_ctcpr ((int(*)(char*,char*,struct userrec*,char*,char*,char*,p_tcl_bind_list))server_funcs[17])
#define detect_avalanche ((int(*)(char *))server_funcs[18])
#define nuke_server ((void(*)(char *))server_funcs[19])
/* 20 - 22 */
#define newserver ((char *)(server_funcs[20]))
#define newserverport (*(int *)(server_funcs[21]))
#define newserverpass ((char *)(server_funcs[22]))
/* 23 - 26 */
#define cycle_time (*(int *)(server_funcs[23]))
#define default_port (*(int *)(server_funcs[24]))
#define server_online (*(int *)(server_funcs[25]))
#define min_servs (*(int *)(server_funcs[26]))
/* 27 - 30 */
#define H_raw (*(p_tcl_bind_list *)(server_funcs[27]))
#define H_wall (*(p_tcl_bind_list *)(server_funcs[28]))
#define H_msg (*(p_tcl_bind_list *)(server_funcs[29]))
#define H_msgm (*(p_tcl_bind_list *)(server_funcs[30]))
/* 31 - 34 */
#define H_notc (*(p_tcl_bind_list *)(server_funcs[31]))
#define H_flud (*(p_tcl_bind_list *)(server_funcs[32]))
#define H_ctcp (*(p_tcl_bind_list *)(server_funcs[33]))
#define H_ctcr (*(p_tcl_bind_list *)(server_funcs[34]))
/* 35 - 36 */
#define ctcp_reply ((char *)(server_funcs[35]))
#define get_altbotnick ((char *(*)(void))(server_funcs[36]))
#endif
