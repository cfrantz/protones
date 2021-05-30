pub mod app_context;
pub mod pyexec;

#[cfg_attr(target_os = "linux", path = "terminal_unix.rs")]
#[cfg_attr(target_os = "windows", path = "terminal_windows.rs")]
pub mod terminal;
pub use terminal::TerminalGuard;

//pub mod build {
//    include!(concat!(env!("OUT_DIR"), "/built.rs"));
//
//    pub fn version() -> &'static str {
//        let v = GIT_VERSION.unwrap_or(PKG_VERSION);
//        // Git tags start with 'v'
//        return v.trim_start_matches(|c: char| !c.is_digit(10));
//    }
//}
