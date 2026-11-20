export module fcopy.agent;

export import fcopy.file_copier;

export namespace fcopy {
struct AgentOptions {
	bool overwrite{false};
	bool recursive{false};
	bool quiet{false};
};

class Agent {
  public:
	using Options = AgentOptions;

	explicit Agent(Options options = {});

	[[nodiscard]] auto copy_paths(std::span<fs::path const> sources, fs::path const& destination) -> ExitCode;
	[[nodiscard]] auto copy_path(fs::path source, fs::path const& destination) -> ExitCode;

  private:
	auto copy_file(Entry source, fs::path destination) -> ExitCode;
	auto copy_directory(Entry const& source, fs::path const& destination) -> ExitCode;

	FileCopier m_file_copier{};
	bool m_recursive{};
};
} // namespace fcopy
