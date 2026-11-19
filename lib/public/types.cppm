export module fcopy.types;

export import std;

export namespace fcopy {
namespace fs = std::filesystem;

enum class ErrorType : std::int8_t {
	Unknown = 100,
	InvalidArgument,
	TargetExists,
	DirectoryCreationFailure,
	FileOpenFailure,
	FileIOFailure,
};
[[nodiscard]] auto to_string_view(ErrorType error_type) -> std::string_view;

enum class EntryType : std::int8_t {
	Unknown,
	Directory,
	RegularFile,
	Symlink,
};
[[nodiscard]] auto to_string_view(EntryType entry_type) -> std::string_view;

struct Error {
	using Type = ErrorType;

	Type type{};
	std::string message{};
};

template <typename Type>
using Result = std::expected<Type, Error>;

struct Entry {
	using Type = EntryType;

	fs::path path{};
	Type type{};
	std::uint64_t size{};
};

struct CopyProgress {
	[[nodiscard]] constexpr auto to_percent() const -> int {
		if (total == 0) {
			return 0;
		}
		return static_cast<int>(synchronized * 100u / total);
	}

	std::uint64_t total{};
	std::uint64_t synchronized{};
};

struct ByteCount {
	float size{};
	std::string_view unit{};
};

[[nodiscard]] auto to_error(Error::Type type, std::string_view message) -> std::unexpected<Error>;

[[nodiscard]] auto to_byte_count(std::uint64_t byte_count) -> ByteCount;
[[nodiscard]] auto to_entry(fs::path path) -> Result<Entry>;
} // namespace fcopy

template <>
struct std::formatter<fcopy::ByteCount> {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}

	static auto format(fcopy::ByteCount const& byte_count, format_context& fc) -> format_context::iterator {
		return format_to(fc.out(), "{:1.1f}{}", byte_count.size, byte_count.unit);
	}
};
