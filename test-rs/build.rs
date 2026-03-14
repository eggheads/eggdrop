use std::collections::HashSet;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let eggdrop_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().to_path_buf();
    let src_dir = eggdrop_dir.join("src");
    let lib_path = eggdrop_dir.join("libeggdrop.a");

    // Link flags
    println!("cargo:rustc-link-search=native={}", eggdrop_dir.display());
    println!("cargo:rustc-link-lib=static=eggdrop");

    println!("cargo:rustc-link-search=native=/usr/lib/x86_64-linux-gnu");
    println!("cargo:rustc-link-lib=dylib=tcl8.6");
    println!("cargo:rustc-link-lib=dylib=z");
    println!("cargo:rustc-link-lib=dylib=ssl");
    println!("cargo:rustc-link-lib=dylib=crypto");
    println!("cargo:rustc-link-lib=dylib=resolv");
    println!("cargo:rustc-link-lib=dylib=m");
    println!("cargo:rustc-link-lib=dylib=pthread");
    println!("cargo:rustc-link-lib=dylib=dl");

    println!("cargo:rerun-if-changed={}", lib_path.display());
    println!("cargo:rerun-if-changed=wrapper.h");

    let out_dir = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    let globals_h = out_dir.join("globals.h");

    let clang_args = &[
        "-DHAVE_CONFIG_H",
        "-DSTATIC",
        "-DMAKING_MODS",
        &format!("-I{}", eggdrop_dir.display()),
        &format!("-I{}", src_dir.display()),
        "-I/usr/include/tcl8.6",
    ];

    // Collect all macro names from the preprocessed headers so we can skip
    // nm symbols that collide with #define'd names (e.g. server.h macros).
    let cpp_output = Command::new("clang")
        .arg("-dM")
        .arg("-E")
        .args(clang_args)
        .arg("wrapper.h")
        .output()
        .expect("failed to run cc -dM -E");
    let cpp_stdout = String::from_utf8_lossy(&cpp_output.stdout);

    let mut macro_names = HashSet::new();
    // For macros like #define nick_len (*(int *)(server_funcs[37])),
    // extract the cast type so we can emit a properly typed extern declaration.
    let mut macro_types = std::collections::HashMap::new();
    for line in cpp_stdout.lines() {
        // Lines look like: #define NAME ...
        let parts: Vec<&str> = line.splitn(3, ' ').collect();
        if parts.len() >= 2 && parts[0] == "#define" {
            // Strip function-like macro parens: NAME(x) -> NAME
            let name = parts[1].split('(').next().unwrap_or(parts[1]);
            if is_valid_c_ident(name) {
                macro_names.insert(name.to_string());
                // Try to extract type from (*(TYPE *)(funcs[N])) or ((TYPE)(funcs[N]))
                if parts.len() == 3 {
                    if let Some(ctype) = extract_macro_deref_type(parts[2]) {
                        macro_types.insert(name.to_string(), ctype);
                    }
                }
            }
        }
    }

    // Also collect symbols already declared in headers via a first-pass bindgen.
    // This avoids redeclaration conflicts (e.g. global_bans already in chan.h).
    let header_bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_args(clang_args)
        .allowlist_file(".*/src/.*\\.h")
        .derive_debug(true)
        .derive_default(true)
        .generate()
        .expect("Unable to generate header-only bindings");

    let header_str = header_bindings.to_string();
    let mut header_symbols = HashSet::new();
    for line in header_str.lines() {
        let trimmed = line.trim();
        // Match "pub static mut NAME:" or "pub static NAME:"
        let rest = if let Some(r) = trimmed.strip_prefix("pub static mut ") {
            Some(r)
        } else {
            trimmed.strip_prefix("pub static ")
        };
        if let Some(rest) = rest {
            if let Some(name) = rest.split(':').next() {
                header_symbols.insert(name.trim().to_string());
            }
        }
    }

    // Run nm on the archive to find global data symbols (B=BSS, D=initialized data).
    let nm_output = Command::new("nm")
        .arg(&lib_path)
        .output()
        .expect("failed to run nm");
    let nm_stdout = String::from_utf8_lossy(&nm_output.stdout);

    let mut seen = HashSet::new();
    let mut undefs = Vec::new();
    let mut decls = Vec::new();

    for line in nm_stdout.lines() {
        let parts: Vec<&str> = line.split_whitespace().collect();
        if parts.len() < 3 { continue; }
        let sym_type = parts[1];
        let sym_name = parts[2];

        if sym_type != "B" && sym_type != "D" { continue; }
        if !is_valid_c_ident(sym_name) { continue; }
        if !seen.insert(sym_name.to_string()) { continue; }
        // Skip symbols already declared in headers (would conflict with typed decl)
        if header_symbols.contains(sym_name) { continue; }

        // If the name collides with a macro, #undef it and use the type from
        // the macro cast (e.g. #define nick_len (*(int *)(server_funcs[37])) -> extern int nick_len;)
        if macro_names.contains(sym_name) {
            undefs.push(format!("#undef {}", sym_name));
        }

        if let Some(ctype) = macro_types.get(sym_name) {
            decls.push(format!("extern {} {};", ctype, sym_name));
        } else {
            decls.push(format!("extern char {}[];", sym_name));
        }
    }

    let mut globals_content = undefs;
    globals_content.extend(decls);
    std::fs::write(&globals_h, globals_content.join("\n")).expect("write globals.h");

    // Generate final bindings with both headers and globalized symbol declarations
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .header(globals_h.to_str().unwrap())
        .clang_args(clang_args)
        .allowlist_file(".*/src/.*\\.h")
        .allowlist_file(".*/globals\\.h")
        .derive_debug(true)
        .derive_default(true)
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("Couldn't write bindings");
}

/// Extract the dereferenced type from a module function table macro.
/// Patterns:
///   (*(int *)(server_funcs[37]))       -> "int"
///   (*(struct chanset_t **)(global[93])) -> "struct chanset_t *"
///   ((char *)(server_funcs[5]))        -> "char"  (cast, not deref)
fn extract_macro_deref_type(expansion: &str) -> Option<String> {
    let s = expansion.trim();
    // Match (*(TYPE *)(table[N])) — dereference through pointer cast
    // The outer parens, then *, then (TYPE *), then (table[N])
    if s.starts_with("(*") {
        // Find the inner cast: (*(TYPE *)(...)
        // Strip outer "(*(" and find the matching "*)("
        let inner = s.strip_prefix("(*(")?;
        // Find "*)" that closes the type cast — this is TYPE *)
        let cast_end = inner.find("*)")?;
        let ctype = inner[..cast_end].trim();
        // The type in the cast is "TYPE *", so the dereferenced type is "TYPE"
        // But if it was "TYPE **", the dereferenced type is "TYPE *"
        // We have the raw type before the "*)" delimiter
        return Some(ctype.to_string());
    }
    // ((TYPE *)(table[N])) — plain cast without deref. The table entry
    // IS a pointer to the data (e.g. char array). We can't reliably
    // determine the underlying variable's type from this pattern, so
    // return None to fall back to the generic extern char[] declaration.
    None
}

fn is_valid_c_ident(s: &str) -> bool {
    let mut chars = s.chars();
    match chars.next() {
        Some(c) if c.is_ascii_alphabetic() || c == '_' => {}
        _ => return false,
    }
    chars.all(|c| c.is_ascii_alphanumeric() || c == '_')
}
