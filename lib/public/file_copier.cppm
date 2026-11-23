export module fcopy.file_copier;

export import fcopy.listener;

export import std;

export namespace fcopy {
class ProgressPrinter {
  public:
	ProgressPrinter() = default;

	explicit ProgressPrinter(fs::path const& path) : m_path(path.generic_string()) {}

	void print(Progress const& progress);

	void operator()(Progress const& progress) { print(progress); }

  private:
	void clear_line();
	void populate_line(Progress const& progress);

	std::string m_path{};
	std::string m_line{};
};

class TransferMeter {
  public:
	void update(std::uint64_t transferred);
	[[nodiscard]] auto get_rate() const -> ByteCount const& { return m_rate; }

  private:
	using Clock = std::chrono::steady_clock;

	Clock::time_point m_start{Clock::now()};
	std::uint64_t m_transferred{};
	ByteCount m_rate{};
};

class FileCopier {
  public:
	[[nodiscard]] static constexpr auto can_copy(EntryType const entry_type) {
		return entry_type == EntryType::RegularFile;
	}

	[[nodiscard]] auto copy_file(fs::path source, fs::path destination) -> Result<void>;
	[[nodiscard]] auto copy_file(Entry source, fs::path destination) -> Result<void>;

	void reset_transfer_rate();

	std::size_t buffer_size{16 * 1024 * 1024uz};
	std::shared_ptr<IListener> progress_listener{};
	bool overwrite_existing{};

  private:
	[[nodiscard]] auto copy_file() -> Result<void>;
	[[nodiscard]] auto create_empty_file() -> Result<void>;
	[[nodiscard]] auto copy_one_shot() -> Result<void>;
	[[nodiscard]] auto open_source() -> Result<void>;
	[[nodiscard]] auto open_destination() -> Result<void>;
	[[nodiscard]] auto copy_bytes() -> Result<void>;
	void set_completed();

	void dispatch_pre_copy() const;
	void dispatch_progress() const;

	Entry m_source{};
	fs::path m_destination{};

	std::ifstream m_in{};
	std::ofstream m_out{};

	Progress m_progress{};
	TransferMeter m_transfer{};
	std::vector<char> m_buffer{};
};
} // namespace fcopy
