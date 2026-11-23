export module fcopy.agent;

export import fcopy.file_copier;

export namespace fcopy {
struct AgentOptions {
	static constexpr std::string_view app_name_v{"fcopy"};

	std::string_view app_name{app_name_v};
	bool force{false};
	bool recursive{false};
	bool quiet{false};
};

class Agent {
  public:
	using Options = AgentOptions;

	explicit Agent(Options options = {});

	[[nodiscard]] auto copy_paths(std::span<fs::path const> sources, fs::path const& target_directory) -> ExitCode;
	[[nodiscard]] auto copy_path(fs::path source, fs::path const& destination) -> ExitCode;

  private:
	[[nodiscard]] auto copy_file(Entry source, fs::path destination) -> ExitCode;
	[[nodiscard]] auto copy_directory(Entry const& source, fs::path const& destination_directory) -> ExitCode;

	[[nodiscard]] auto copy_directory_recursive(fs::path const& relative) -> ExitCode;

	[[nodiscard]] auto prompt_overwrite(fs::path const& destination) const -> ExitCode;

	FileCopier m_file_copier{};
	std::string_view m_app_name{};
	bool m_force{};
	bool m_recursive{};

	fs::path m_source_root{};
	fs::path m_destination_root{};
};
} // namespace fcopy
