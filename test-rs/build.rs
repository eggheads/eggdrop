use std::path::PathBuf;

fn main() {
    let eggdrop_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().to_path_buf();
    let src_dir = eggdrop_dir.join("src");

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

    println!("cargo:rerun-if-changed={}", eggdrop_dir.join("libeggdrop.a").display());
    println!("cargo:rerun-if-changed=wrapper.h");

    // Generate bindings with bindgen
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        // Turn `static` into `extern` so bindgen sees all file-scope declarations.
        // This only affects the parse, not compilation — function-local statics
        // appear as extra globals (harmless noise, filtered by allowlist).
        .clang_arg("-Dstatic=extern")
        // Match eggdrop's build flags
        .clang_arg("-DHAVE_CONFIG_H")
        .clang_arg("-DSTATIC")
        .clang_arg("-DMAKING_MODS")
        // Include paths
        .clang_arg(format!("-I{}", eggdrop_dir.display()))
        .clang_arg(format!("-I{}", src_dir.display()))
        .clang_arg("-I/usr/include/tcl8.6")
        // Only generate bindings for eggdrop symbols, not system headers
        .allowlist_file(".*/src/.*\\.h")
        .derive_debug(true)
        .derive_default(true)
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings");
}
