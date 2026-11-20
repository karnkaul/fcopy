export module fcopy;

export import fcopy.types;
export import fcopy.build_version;

export namespace fcopy {
[[nodiscard]] auto copy_file(Entry source, fs::path destination, Options options = {}) -> ExitCode;
[[nodiscard]] auto copy_directory(Entry const& source, fs::path const& destination, Options options = {}) -> ExitCode;
[[nodiscard]] auto copy_path(fs::path source, fs::path destination, Options options = {}) -> ExitCode;
} // namespace fcopy
