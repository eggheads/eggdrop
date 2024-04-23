// SPDX-License-Identifier: MIT */
//
// zig.zig -- part of zig.mod
//
// Copyright (c) 2024 Michael Ortmann

const MODULE_NAME = "zig";

const builtin = @import("builtin");
const c = @cImport({
    @cDefine("STATIC", "1"); // TODO
    @cInclude("src/mod/module.h");
});
const std = @import("std");

const global_table = extern struct {
    // 0 - 3
    mod_malloc: *const fn () callconv(.C) void,
    mod_free: *const fn () callconv(.C) void,
    foo0: *const fn () callconv(.C) void,
    module_rename: *const fn () callconv(.C) void,
    // 4 - 7
    module_register: *const fn ([*]const u8, *const modcall, c_int, c_int) callconv(.C) c_int,
    module_find: *const fn () callconv(.C) void,
    module_depend: *const fn ([*]const u8, [*]const u8, c_int, c_int) callconv(.C) ?[*]void, // TODO: return value
    module_undepend: *const fn ([*]const u8) callconv(.C) c_int,
    // 8 - 11
    add_bind_table: *const fn () callconv(.C) void,
    del_bind_table: *const fn () callconv(.C) void,
    find_bind_table: *const fn () callconv(.C) void,
    check_tcl_bind: *const fn () callconv(.C) void,
    // 12 - 15
    add_builtins: *const fn () callconv(.C) void,
    rem_builtins: *const fn () callconv(.C) void,
    add_tcl_commands: *const fn () callconv(.C) void,
    rem_tcl_commands: *const fn () callconv(.C) void,
    // 16 - 19
    add_tcl_ints: *const fn () callconv(.C) void,
    rem_tcl_ints: *const fn () callconv(.C) void,
    add_tcl_strings: *const fn () callconv(.C) void,
    rem_tcl_strings: *const fn () callconv(.C) void,
    // 20 - 23
    base64_to_int: *const fn () callconv(.C) void,
    int_to_base64: *const fn () callconv(.C) void,
    int_to_base10: *const fn () callconv(.C) void,
    simple_sprintf: *const fn () callconv(.C) void,
    // 24 - 27
    botnet_send_zapf: *const fn () callconv(.C) void,
    botnet_send_zapf_broad: *const fn () callconv(.C) void,
    botnet_send_unlinked: *const fn () callconv(.C) void,
    botnet_send_bye: *const fn () callconv(.C) void,
    // 28 - 31
    botnet_send_chat: *const fn () callconv(.C) void,
    botnet_send_filereject: *const fn () callconv(.C) void,
    botnet_send_filesend: *const fn () callconv(.C) void,
    botnet_send_filereq: *const fn () callconv(.C) void,
    // 32 - 35
    botnet_send_join_idx: *const fn () callconv(.C) void,
    botnet_send_part_idx: *const fn () callconv(.C) void,
    updatebot: *const fn () callconv(.C) void,
    nextbot: *const fn () callconv(.C) void,
    // 36 - 39
    zapfbot: *const fn () callconv(.C) void,
    n_free: *const fn () callconv(.C) void,
    u_pass_match: *const fn () callconv(.C) void,
    _user_malloc: *const fn () callconv(.C) void,
    // 40 - 43
    get_user: *const fn () callconv(.C) void,
    set_user: *const fn () callconv(.C) void,
    add_entry_type: *const fn () callconv(.C) void,
    del_entry_type: *const fn () callconv(.C) void,
    // 44 - 47
    get_user_flagrec: *const fn () callconv(.C) void,
    set_user_flagrec: *const fn () callconv(.C) void,
    get_user_by_host: *const fn () callconv(.C) void,
    get_user_by_handle: *const fn () callconv(.C) void,
    // 48 - 51
    find_entry_type: *const fn () callconv(.C) void,
    find_user_entry: *const fn () callconv(.C) void,
    adduser: *const fn () callconv(.C) void,
    deluser: *const fn () callconv(.C) void,
    // 52 - 55
    addhost_by_handle: *const fn () callconv(.C) void,
    delhost_by_handle: *const fn () callconv(.C) void,
    readuserfile: *const fn () callconv(.C) void,
    write_userfile: *const fn () callconv(.C) void,
    // 56 - 59
    geticon: *const fn () callconv(.C) void,
    clear_chanlist: *const fn () callconv(.C) void,
    reaffirm_owners: *const fn () callconv(.C) void,
    change_handle: *const fn () callconv(.C) void,
    // 60 - 63
    write_user: *const fn () callconv(.C) void,
    clear_userlist: *const fn () callconv(.C) void,
    count_users: *const fn () callconv(.C) void,
    sanity_check: *const fn () callconv(.C) void,
    // 64 - 67
    break_down_flags: *const fn () callconv(.C) void,
    build_flags: *const fn () callconv(.C) void,
    flagrec_eq: *const fn () callconv(.C) void,
    flagrec_ok: *const fn () callconv(.C) void,
    // 68 - 71
    shareout: *const fn () callconv(.C) void,
    dprintf: *const fn (c_int, [*]const u8) callconv(.C) void,
    chatout: *const fn () callconv(.C) void,
    chanout_but: *const fn () callconv(.C) void,
    // 72 - 75
    check_validity: *const fn () callconv(.C) void,
    egg_list_delete: *const fn () callconv(.C) void,
    egg_list_append: *const fn () callconv(.C) void,
    egg_list_contains: *const fn () callconv(.C) void,
    // 76 - 79
    answer: *const fn () callconv(.C) void,
    getvhost: *const fn () callconv(.C) void,
    ssl_handshake: *const fn () callconv(.C) void,
    foo1: *const fn () callconv(.C) void,
    tputs: *const fn () callconv(.C) void,
    // 80 - 83
    new_dcc: *const fn () callconv(.C) void,
    lostdcc: *const fn () callconv(.C) void,
    getsock: *const fn () callconv(.C) void,
    killsock: *const fn () callconv(.C) void,
    // 84 - 87
    open_listen: *const fn () callconv(.C) void,
    getdccaddr: *const fn () callconv(.C) void,
    _get_data_ptr: *const fn () callconv(.C) void,
    open_telnet: *const fn () callconv(.C) void,
    // 88 - 91
    check_tcl_event: *const fn () callconv(.C) void,
    foo2: *const fn () callconv(.C) void,
    my_atoul: *const fn () callconv(.C) void,
    my_strcpy: *const fn () callconv(.C) void,
    // 92 - 95
    dcc: *const fn () callconv(.C) void,
    chanset: *const fn () callconv(.C) void,
    userlist: *const fn () callconv(.C) void,
    lastuser: *const fn () callconv(.C) void,
    // 96 - 99
    global_bans: *const fn () callconv(.C) void,
    global_ign: *const fn () callconv(.C) void,
    password_timeout: *const fn () callconv(.C) void,
    share_greet: *const fn () callconv(.C) void,
    // 100 - 103
    max_dcc: *const fn () callconv(.C) void,
    require_p: *const fn () callconv(.C) void,
    ignore_time: *const fn () callconv(.C) void,
    dcc_fingerprint: *const fn () callconv(.C) void,
    foo3: *const fn () callconv(.C) void,
    // 104 - 107
    reserved_port_min: *const fn () callconv(.C) void,
    reserved_port_max: *const fn () callconv(.C) void,
    raw_log: *const fn () callconv(.C) void,
    noshare: *const fn () callconv(.C) void,
    // 108 - 111
    tls_vfybots: *const fn () callconv(.C) void,
    foo4: *const fn () callconv(.C) void,
    make_userfile: *const fn () callconv(.C) void,
    default_flags: *const fn () callconv(.C) void,
    dcc_total: *const fn () callconv(.C) void,
    // 112 - 115
    foo5: *const fn () callconv(.C) void,
    tls_vfyclients: *const fn () callconv(.C) void,
    tls_vfydcc: *const fn () callconv(.C) void,
    foo6: *const fn () callconv(.C) void,
    foo7: *const fn () callconv(.C) void,
    origbotname: *const fn () callconv(.C) void,
    // 116 - 119
    botuser: *const fn () callconv(.C) void,
    admin: *const fn () callconv(.C) void,
    userfile: *const fn () callconv(.C) void,
    ver: *const fn () callconv(.C) void,
    // 120 - 123
    notify_new: *const fn () callconv(.C) void,
    helpdir: *const fn () callconv(.C) void,
    version: *const fn () callconv(.C) void,
    botnetnick: *const fn () callconv(.C) void,
    // 124 - 127
    DCC_CHAT_PASS: *const fn () callconv(.C) void,
    DCC_BOT: *const fn () callconv(.C) void,
    DCC_LOST: *const fn () callconv(.C) void,
    DCC_CHAT: *const fn () callconv(.C) void,
    // 128 - 131
    interp: *const fn () callconv(.C) void,
    now: *const fn () callconv(.C) void,
    findanyidx: *const fn () callconv(.C) void,
    findchan: *const fn () callconv(.C) void,
    // 132 - 135
    cmd_die: *const fn () callconv(.C) void,
    days: *const fn () callconv(.C) void,
    daysago: *const fn () callconv(.C) void,
    daysdur: *const fn () callconv(.C) void,
    // 136 - 139
    ismember: *const fn () callconv(.C) void,
    newsplit: *const fn () callconv(.C) void,
    splitnick: *const fn () callconv(.C) void,
    splitc: *const fn () callconv(.C) void,
    // 140 - 143
    addignore: *const fn () callconv(.C) void,
    match_ignore: *const fn () callconv(.C) void,
    delignore: *const fn () callconv(.C) void,
    fatal: *const fn () callconv(.C) void,
    // 144 - 147
    xtra_kill: *const fn () callconv(.C) void,
    xtra_unpack: *const fn () callconv(.C) void,
    movefile: *const fn () callconv(.C) void,
    copyfile: *const fn () callconv(.C) void,
    // 148 - 151
    do_tcl: *const fn () callconv(.C) void,
    readtclprog: *const fn () callconv(.C) void,
    get_language: *const fn () callconv(.C) void,
    def_get: *const fn () callconv(.C) void,
    // 152 - 155
    makepass: *const fn () callconv(.C) void,
    _wild_match: *const fn () callconv(.C) void,
    maskaddr: *const fn () callconv(.C) void,
    show_motd: *const fn () callconv(.C) void,
    // 156 - 159
    tellhelp: *const fn () callconv(.C) void,
    showhelp: *const fn () callconv(.C) void,
    add_help_reference: *const fn () callconv(.C) void,
    rem_help_reference: *const fn () callconv(.C) void,
    // 160 - 163
    touch_laston: *const fn () callconv(.C) void,
    add_mode: *const fn () callconv(.C) void,
    rmspace: *const fn () callconv(.C) void,
    in_chain: *const fn () callconv(.C) void,
    // 164 - 167
    add_note: *const fn () callconv(.C) void,
    del_lang_section: *const fn () callconv(.C) void,
    detect_dcc_flood: *const fn () callconv(.C) void,
    flush_lines: *const fn () callconv(.C) void,
    // 168 - 171
    expected_memory: *const fn () callconv(.C) void,
    tell_mem_status: *const fn () callconv(.C) void,
    do_restart: *const fn () callconv(.C) void,
    check_tcl_filt: *const fn () callconv(.C) void,
    // 172 - 175
    add_hook: *const fn () callconv(.C) void,
    del_hook: *const fn () callconv(.C) void,
    H_dcc: *const fn () callconv(.C) void,
    H_filt: *const fn () callconv(.C) void,
    // 176 - 179
    H_chon: *const fn () callconv(.C) void,
    H_chof: *const fn () callconv(.C) void,
    H_load: *const fn () callconv(.C) void,
    H_unld: *const fn () callconv(.C) void,
    // 180 - 183
    H_chat: *const fn () callconv(.C) void,
    H_act: *const fn () callconv(.C) void,
    H_bcst: *const fn () callconv(.C) void,
    H_bot: *const fn () callconv(.C) void,
    // 184 - 187
    H_link: *const fn () callconv(.C) void,
    H_disc: *const fn () callconv(.C) void,
    H_away: *const fn () callconv(.C) void,
    H_nkch: *const fn () callconv(.C) void,
    // 188 - 191
    USERENTRY_BOTADDR: *const fn () callconv(.C) void,
    USERENTRY_BOTFL: *const fn () callconv(.C) void,
    USERENTRY_HOSTS: *const fn () callconv(.C) void,
    USERENTRY_PASS: *const fn () callconv(.C) void,
    // 192 - 195
    USERENTRY_XTRA: *const fn () callconv(.C) void,
    user_del_chan: *const fn () callconv(.C) void,
    USERENTRY_INFO: *const fn () callconv(.C) void,
    USERENTRY_COMMENT: *const fn () callconv(.C) void,
    // 196 - 199
    USERENTRY_LASTON: *const fn () callconv(.C) void,
    putlog: *const fn () callconv(.C) void,
    botnet_send_chan: *const fn () callconv(.C) void,
    list_type_kill: *const fn () callconv(.C) void,
    // 200 - 203
    logmodes: *const fn () callconv(.C) void,
    masktype: *const fn () callconv(.C) void,
    stripmodes: *const fn () callconv(.C) void,
    stripmasktype: *const fn () callconv(.C) void,
    // 204 - 207
    sub_lang: *const fn () callconv(.C) void,
    online_since: *const fn () callconv(.C) void,
    cmd_loadlanguage: *const fn () callconv(.C) void,
    check_dcc_attrs: *const fn () callconv(.C) void,
    // 208 - 211
    check_dcc_chanattrs: *const fn () callconv(.C) void,
    add_tcl_coups: *const fn () callconv(.C) void,
    rem_tcl_coups: *const fn () callconv(.C) void,
    botname: *const fn () callconv(.C) void,
    // 212 - 215
    foo8: *const fn () callconv(.C) void,
    check_tcl_chjn: *const fn () callconv(.C) void,
    sanitycheck_dcc: *const fn () callconv(.C) void,
    isowner: *const fn () callconv(.C) void,
    // 216 - 219
    fcopyfile: *const fn () callconv(.C) void,
    copyfilef: *const fn () callconv(.C) void,
    rfc_casecmp: *const fn () callconv(.C) void,
    rfc_ncasecmp: *const fn () callconv(.C) void,
    // 220 - 223
    global_exempts: *const fn () callconv(.C) void,
    global_invites: *const fn () callconv(.C) void,
    foo9: *const fn () callconv(.C) void,
    foo10: *const fn () callconv(.C) void,
    // 224 - 227
    H_event: *const fn () callconv(.C) void,
    use_exempts: *const fn () callconv(.C) void,
    use_invites: *const fn () callconv(.C) void,
    force_expire: *const fn () callconv(.C) void,
    // 228 - 231
    add_lang_section: *const fn () callconv(.C) void,
    _user_realloc: *const fn () callconv(.C) void,
    mod_realloc: *const fn () callconv(.C) void,
    xtra_set: *const fn () callconv(.C) void,
    // 232 - 235
    foo11: *const fn () callconv(.C) void,
    eggAssert: *const fn () callconv(.C) void,
    foo12: *const fn () callconv(.C) void,
    allocsock: *const fn () callconv(.C) void,
    call_hostbyip: *const fn () callconv(.C) void,
    // 236 - 239
    call_ipbyhost: *const fn () callconv(.C) void,
    iptostr: *const fn () callconv(.C) void,
    DCC_DNSWAIT: *const fn () callconv(.C) void,
    hostsanitycheck_dcc: *const fn () callconv(.C) void,
    // 240 - 243
    dcc_dnsipbyhost: *const fn () callconv(.C) void,
    dcc_dnshostbyip: *const fn () callconv(.C) void,
    changeover_dcc: *const fn () callconv(.C) void,
    make_rand_str: *const fn () callconv(.C) void,
    // 244 - 247
    protect_readonly: *const fn () callconv(.C) void,
    findchan_by_dname: *const fn () callconv(.C) void,
    removedcc: *const fn () callconv(.C) void,
    userfile_perm: *const fn () callconv(.C) void,
    // 248 - 251
    sock_has_data: *const fn () callconv(.C) void,
    bots_in_subtree: *const fn () callconv(.C) void,
    users_in_subtree: *const fn () callconv(.C) void,
    egg_inet_aton: *const fn () callconv(.C) void,
    // 252 - 255
    egg_snprintf: *const fn () callconv(.C) void,
    egg_vsnprintf: *const fn () callconv(.C) void,
    foo13: *const fn () callconv(.C) void,
    foo14: *const fn () callconv(.C) void,
    // 256 - 259
    foo15: *const fn () callconv(.C) void,
    is_file: *const fn () callconv(.C) void,
    must_be_owner: *const fn () callconv(.C) void,
    tandbot: *const fn () callconv(.C) void,
    // 260 - 263
    party: *const fn () callconv(.C) void,
    open_address_listen: *const fn () callconv(.C) void,
    str_escape: *const fn () callconv(.C) void,
    strchr_unescape: *const fn () callconv(.C) void,
    // 264 - 267
    str_unescape: *const fn () callconv(.C) void,
    egg_strcatn: *const fn () callconv(.C) void,
    clear_chanlist_member: *const fn () callconv(.C) void,
    foo16: *const fn () callconv(.C) void,
    // 268 - 271
    socklist: *const fn () callconv(.C) void,
    sockoptions: *const fn () callconv(.C) void,
    flush_inbuf: *const fn () callconv(.C) void,
    kill_bot: *const fn () callconv(.C) void,
    // 272 - 275
    quit_msg: *const fn () callconv(.C) void,
    module_load: *const fn () callconv(.C) void,
    module_unload: *const fn () callconv(.C) void,
    parties: *const fn () callconv(.C) void,
    // 276 - 279
    tell_bottree: *const fn () callconv(.C) void,
    MD5_Init: *const fn () callconv(.C) void,
    MD5_Update: *const fn () callconv(.C) void,
    MD5_Final: *const fn () callconv(.C) void,
    // 280 - 283
    _wild_match_per: *const fn () callconv(.C) void,
    killtransfer: *const fn () callconv(.C) void,
    write_ignores: *const fn () callconv(.C) void,
    copy_to_tmp: *const fn () callconv(.C) void,
    // 284 - 287
    quiet_reject: *const fn () callconv(.C) void,
    file_readable: *const fn () callconv(.C) void,
    setsockname: *const fn () callconv(.C) void,
    open_telnet_raw: *const fn () callconv(.C) void,
    // 288 - 291
    pref_af: *const fn () callconv(.C) void,
    foo17: *const fn () callconv(.C) void,
    strip_mirc_codes: *const fn () callconv(.C) void,
    check_ansi: *const fn () callconv(.C) void,
    oatoi: *const fn () callconv(.C) void,
    // 292 - 295
    str_isdigit: *const fn () callconv(.C) void,
    remove_crlf: *const fn () callconv(.C) void,
    addr_match: *const fn () callconv(.C) void,
    mask_match: *const fn () callconv(.C) void,
    // 296 - 299
    check_conflags: *const fn () callconv(.C) void,
    increase_socks_max: *const fn () callconv(.C) void,
    log_ts: *const fn () callconv(.C) void,
    tcl_resultempty: *const fn () callconv(.C) void,
    // 300 - 303
    tcl_resultint: *const fn () callconv(.C) void,
    tcl_resultstring: *const fn () callconv(.C) void,
    getdccfamilyaddr: *const fn () callconv(.C) void,
    strlcpy: *const fn () callconv(.C) void,
    foo18: *const fn () callconv(.C) void,
    // 304 - 307
    strncpyz: *const fn () callconv(.C) void,
    b64_ntop: *const fn () callconv(.C) void,
    b64_pton: *const fn () callconv(.C) void,
    foo19: *const fn () callconv(.C) void,
    foo20: *const fn () callconv(.C) void,
    check_validpass: *const fn () callconv(.C) void,
    // 308 - 311
    make_rand_str_from_chars: *const fn () callconv(.C) void,
    add_tcl_objcommands: *const fn () callconv(.C) void,
    pid_file: *const fn () callconv(.C) void,
    explicit_bzero: *const fn () callconv(.C) void,
    foo21: *const fn () callconv(.C) void,
    // 312 - 315
    USERENTRY_PASS2: *const fn () callconv(.C) void,
    crypto_verify: *const fn () callconv(.C) void,
    egg_uname: *const fn () callconv(.C) void,
    get_expire_time: *const fn () callconv(.C) void,
    // 316 - 319
    USERENTRY_ACCOUNT: *const fn () callconv(.C) void,
    get_user_by_account: *const fn () callconv(.C) void,
    foo22: *const fn () callconv(.C) void, // TODO: bug in eggdrop? reported to IRC #eggheads 20240422
    check_tcl_event_arg: *const fn () callconv(.C) void,
    // 320 - 323
    bind_bind_entry: *const fn () callconv(.C) void,
    unbind_bind_entry: *const fn () callconv(.C) void,
    argv0: *const fn () callconv(.C) void,
};

var global: *global_table = undefined;

export fn zig_close() ?[*]const u8 {
    return null;
}

export fn zig_report(idx: c_int, details: c_int) void {
    if (details > 0) {
        const allocator = std.heap.page_allocator;
        const s = std.fmt.allocPrint(allocator, "    zig version: {s}\n", .{builtin.zig_version_string}) catch return;
        global.dprintf(idx, s.ptr);
    }
}

const modcall = extern struct {
    start: *const fn (*global_table) callconv(.C) ?[*]const u8, // MODCALL_START, TODO: will become Optional Pointer via #1564
    close: ?*const fn () callconv(.C) ?[*]const u8, // MODCALL_CLOSE
    expmem: ?*const fn () callconv(.C) void, // MODCALL_EXPMEM
    report: ?*const fn (c_int, c_int) callconv(.C) void, // MODCALL_REPORT
};

export fn zig_start(global_funcs: *global_table) ?[*]const u8 {
    global = global_funcs;
    const zig_table = modcall{
        .start = zig_start,
        .close = zig_close,
        .expmem = null,
        .report = zig_report,
    };
    _ = global.module_register(MODULE_NAME.ptr, &zig_table, 0, 1);

    if (global.module_depend(MODULE_NAME, "eggdrop", 109, 5) == null) {
        _ = global.module_undepend(MODULE_NAME);
        return "This module requires Eggdrop 1.9.5 or later.";
    }

    return null;
}
