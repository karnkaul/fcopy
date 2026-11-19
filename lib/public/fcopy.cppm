export module fcopy;

export import fcopy.build_version;
export import fcopy.types;

export import std;

export namespace fcopy {
class ProgressPrinter {
  public:
	explicit ProgressPrinter(fs::path const& path) : m_path(path.generic_string()) {}

	void print(CopyProgress const& progress);

	void operator()(CopyProgress const& progress) { print(progress); }

  private:
	std::string m_path{};
	bool m_printed{};
};

class FileCopier {
  public:
	[[nodiscard]] auto copy_file(Entry source, fs::path destination, bool overwrite) -> Result<void>;

	std::size_t buffer_size{16 * 1024 * 1024uz};

  private:
	[[nodiscard]] auto copy_file() -> Result<void>;
	[[nodiscard]] auto create_empty_file() -> Result<void>;
	[[nodiscard]] auto copy_one_shot() -> Result<void>;
	[[nodiscard]] auto open_source() -> Result<void>;
	[[nodiscard]] auto open_destination() -> Result<void>;
	[[nodiscard]] auto copy_bytes() -> Result<void>;
	void overwrite_permissions() const;

	void print_progress() const;

	std::optional<ProgressPrinter> m_print_progress{};

	Entry m_source{};
	fs::path m_destination{};

	std::ifstream m_in{};
	std::ofstream m_out{};

	CopyProgress m_progress{};
	std::vector<char> m_buffer{};
};
} // namespace fcopy
