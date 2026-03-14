fn main() {
    let eggdrop_dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();

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
}
