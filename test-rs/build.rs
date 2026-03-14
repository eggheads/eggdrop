use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let eggdrop_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .to_path_buf();
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
        &format!("-I{}", src_dir.join("mod").display()),
        "-I/usr/include/tcl8.6",
    ];

    // Collect all macro names from the preprocessed headers so we can
    // #undef names that collide with nm symbols before declaring them extern.
    let cpp_output = Command::new("clang")
        .arg("-dM")
        .arg("-E")
        .args(clang_args)
        .arg("wrapper.h")
        .output()
        .expect("failed to run clang -dM -E");
    let cpp_stdout = String::from_utf8_lossy(&cpp_output.stdout);

    let mut macro_names = HashSet::new();
    for line in cpp_stdout.lines() {
        let parts: Vec<&str> = line.splitn(3, ' ').collect();
        if parts.len() >= 2 && parts[0] == "#define" {
            let name = parts[1].split('(').next().unwrap_or(parts[1]);
            if is_valid_c_ident(name) {
                macro_names.insert(name.to_string());
            }
        }
    }

    // Collect symbols already declared in headers via a first-pass bindgen.
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
    let mut known_types = HashSet::new();
    for line in header_str.lines() {
        let trimmed = line.trim();
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
        // Collect type names from bindgen output (type aliases and struct/enum/union names)
        if let Some(rest) = trimmed.strip_prefix("pub type ") {
            if let Some(name) = rest.split_whitespace().next() {
                known_types.insert(name.to_string());
            }
        } else if let Some(rest) = trimmed.strip_prefix("pub struct ") {
            if let Some(name) = rest.split_whitespace().next().map(|n| n.trim_end_matches('{')) {
                known_types.insert(name.to_string());
            }
        } else if let Some(rest) = trimmed.strip_prefix("pub enum ") {
            if let Some(name) = rest.split_whitespace().next().map(|n| n.trim_end_matches('{')) {
                known_types.insert(name.to_string());
            }
        }
    }

    // Run nm -A on the archive to find global data symbols and their .o files.
    let nm_output = Command::new("nm")
        .arg("-A")
        .arg(&lib_path)
        .output()
        .expect("failed to run nm");
    let nm_stdout = String::from_utf8_lossy(&nm_output.stdout);

    let mut nm_symbols = HashSet::new();
    let mut obj_files = HashSet::new();
    for line in nm_stdout.lines() {
        // Format: /path/libeggdrop.a:foo.o:0000 B symbol_name
        let after_archive = match line.split_once(':') {
            Some((_, rest)) => rest,
            None => continue,
        };
        let (obj_name, rest) = match after_archive.split_once(':') {
            Some(pair) => pair,
            None => continue,
        };
        // rest is "addr TYPE name" (e.g. "0000000000000000 B bg")
        let parts: Vec<&str> = rest.split_whitespace().collect();
        if parts.len() < 3 {
            continue;
        }
        let sym_type = parts[1];
        let sym_name = parts[2];

        if sym_type == "B" || sym_type == "D" {
            if is_valid_c_ident(sym_name) {
                nm_symbols.insert(sym_name.to_string());
                obj_files.insert(obj_name.to_string());
            }
        }
    }

    // Map .o file basenames to .c source files by searching src/.
    let c_files = find_c_files_for_objects(&src_dir, &obj_files);

    // Run clang AST dump on each .c file (in parallel) to extract types
    // for file-scope static variables.
    let static_types = extract_static_types(&c_files, clang_args);

    // Generate globals.h with properly typed extern declarations.
    let mut seen = HashSet::new();
    let mut undefs = Vec::new();
    let mut decls = Vec::new();

    for sym_name in &nm_symbols {
        if !seen.insert(sym_name.clone()) {
            continue;
        }
        if header_symbols.contains(sym_name) {
            continue;
        }

        if macro_names.contains(sym_name) {
            undefs.push(format!("#undef {}", sym_name));
        }

        if let Some(qualtype) = static_types.get(sym_name) {
            if type_is_visible(qualtype, &known_types) {
                decls.push(format_extern_decl(sym_name, qualtype));
            } else {
                // Type uses module-internal typedefs not in headers
                decls.push(format!("extern char {}[];", sym_name));
            }
        } else {
            // Fallback for symbols not found in AST (e.g. from compat/ or md5/)
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

/// Find .c source files corresponding to .o basenames by searching src/.
fn find_c_files_for_objects(src_dir: &PathBuf, obj_files: &HashSet<String>) -> Vec<PathBuf> {
    let mut c_files = Vec::new();
    let mut needed: HashSet<String> = obj_files
        .iter()
        .map(|o| o.strip_suffix(".o").unwrap_or(o).to_string())
        .collect();

    fn walk_dir(dir: &PathBuf, needed: &mut HashSet<String>, c_files: &mut Vec<PathBuf>) {
        let entries = match std::fs::read_dir(dir) {
            Ok(e) => e,
            Err(_) => return,
        };
        for entry in entries.flatten() {
            let path = entry.path();
            if path.is_dir() {
                walk_dir(&path, needed, c_files);
            } else if let Some(ext) = path.extension() {
                if ext == "c" {
                    if let Some(stem) = path.file_stem().and_then(|s| s.to_str()) {
                        if needed.remove(stem) {
                            c_files.push(path);
                        }
                    }
                }
            }
        }
    }

    walk_dir(src_dir, &mut needed, &mut c_files);
    c_files
}

/// Run clang -Xclang -ast-dump=json on each .c file in parallel,
/// extract file-scope static variable declarations with their types.
fn extract_static_types(c_files: &[PathBuf], clang_args: &[&str]) -> HashMap<String, String> {
    use std::thread;

    let handles: Vec<_> = c_files
        .iter()
        .map(|c_file| {
            let c_file = c_file.clone();
            let args: Vec<String> = clang_args.iter().map(|s| s.to_string()).collect();
            thread::spawn(move || {
                let output = Command::new("clang")
                    .arg("-Xclang")
                    .arg("-ast-dump=json")
                    .arg("-fsyntax-only")
                    .args(&args)
                    .arg(&c_file)
                    .output();

                let output = match output {
                    Ok(o) if o.status.success() => o,
                    _ => return Vec::new(),
                };

                let ast: serde_json::Value = match serde_json::from_slice(&output.stdout) {
                    Ok(v) => v,
                    Err(_) => return Vec::new(),
                };

                let mut results = Vec::new();
                if let Some(inner) = ast.get("inner").and_then(|v| v.as_array()) {
                    for node in inner {
                        if node.get("kind").and_then(|v| v.as_str()) != Some("VarDecl") {
                            continue;
                        }
                        if node.get("storageClass").and_then(|v| v.as_str()) != Some("static") {
                            continue;
                        }
                        let name = match node.get("name").and_then(|v| v.as_str()) {
                            Some(n) => n.to_string(),
                            None => continue,
                        };
                        let qualtype = match node
                            .get("type")
                            .and_then(|v| v.get("qualType"))
                            .and_then(|v| v.as_str())
                        {
                            Some(t) => t.to_string(),
                            None => continue,
                        };
                        results.push((name, qualtype));
                    }
                }
                results
            })
        })
        .collect();

    let mut types = HashMap::new();
    for handle in handles {
        if let Ok(results) = handle.join() {
            for (name, qualtype) in results {
                // Use first occurrence (some names may appear in multiple files
                // but colliders are excluded from globalization anyway)
                types.entry(name).or_insert(qualtype);
            }
        }
    }
    types
}

/// C type keywords and qualifiers that don't need to be in the known_types set.
const C_TYPE_KEYWORDS: &[&str] = &[
    "void",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "const",
    "volatile",
    "restrict",
    "struct",
    "union",
    "enum",
    "_Bool",
    "_Complex",
    "_Imaginary",
];

/// Check if all type identifiers in a qualType string are either C keywords
/// or present in the known_types set from the header-only bindgen pass.
fn type_is_visible(qualtype: &str, known_types: &HashSet<String>) -> bool {
    // Extract identifier-like tokens from the type string
    for token in qualtype.split(|c: char| !c.is_ascii_alphanumeric() && c != '_') {
        if token.is_empty() {
            continue;
        }
        // Skip numeric tokens (array sizes like [512])
        if token.chars().next().map_or(true, |c| c.is_ascii_digit()) {
            continue;
        }
        // Skip C keywords
        if C_TYPE_KEYWORDS.contains(&token) {
            continue;
        }
        // Must be in known types
        if !known_types.contains(token) {
            return false;
        }
    }
    true
}

/// Format an extern declaration from a clang qualType string.
/// Handles array types where brackets must follow the identifier:
///   "int"           -> "extern int name;"
///   "char [512]"    -> "extern char name[512];"
///   "int *"         -> "extern int * name;"
///   "void (*)(int)" -> "extern void (*name)(int);"  (function pointers)
fn format_extern_decl(name: &str, qualtype: &str) -> String {
    // Check for array types: "TYPE [N]" or "TYPE [N][M]"
    if let Some(bracket_pos) = qualtype.find('[') {
        let base_type = qualtype[..bracket_pos].trim();
        let array_part = &qualtype[bracket_pos..];
        format!("extern {} {}{};", base_type, name, array_part)
    } else if qualtype.contains("(*)") {
        // Function pointer: "TYPE (*)(ARGS)" -> "extern TYPE (*name)(ARGS);"
        let replaced = qualtype.replacen("(*)", &format!("(*{})", name), 1);
        format!("extern {};", replaced)
    } else {
        format!("extern {} {};", qualtype, name)
    }
}

fn is_valid_c_ident(s: &str) -> bool {
    let mut chars = s.chars();
    match chars.next() {
        Some(c) if c.is_ascii_alphabetic() || c == '_' => {}
        _ => return false,
    }
    chars.all(|c| c.is_ascii_alphanumeric() || c == '_')
}
