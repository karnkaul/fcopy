module fcopy.types;

namespace fcopy {
namespace {
[[nodiscard]] auto to_entry_type(fs::path const& path) {
	if (fs::is_directory(path)) {
		return EntryType::Directory;
	}
	if (fs::is_regular_file(path)) {
		return EntryType::RegularFile;
	}
	if (fs::is_symlink(path)) {
		return EntryType::Symlink;
	}
	return EntryType::Unknown;
}
} // namespace
} // namespace fcopy

auto fcopy::to_string_view(ErrorType const error_type) -> std::string_view {
	switch (error_type) {
	case ErrorType::InvalidArgument: return "InvalidArgument";
	case ErrorType::TargetExists: return "TargetExists";
	case ErrorType::DirectoryCreationFailure: return "DirectoryCreationFailure";
	case ErrorType::FileOpenFailure: return "FileOpenFailure";
	case ErrorType::FileIOFailure: return "FileIOFailure";

	case ErrorType::Unknown: break;
	}
	return "Unknown";
}

auto fcopy::to_string_view(EntryType const entry_type) -> std::string_view {
	switch (entry_type) {
	case EntryType::Directory: return "Directory";
	case EntryType::RegularFile: return "RegularFile";
	case EntryType::Symlink: return "Symlink";
	case EntryType::Unknown: break;
	}
	return "Unknown";
}

auto fcopy::to_error(Error::Type const type, std::string_view const message) -> std::unexpected<Error> {
	auto const type_name = to_string_view(type);
	auto const type_number = std::to_underlying(type);
	auto error = Error{.type = type, .message = std::format("{} ({}): {}", type_name, type_number, message)};
	return std::unexpected{std::move(error)};
}

auto fcopy::to_byte_count(std::uint64_t const byte_count) -> ByteCount {
	static constexpr auto divisor_v{1024.0f};
	static constexpr auto units_v = std::array{"KiB", "MiB", "GiB", "TiB"};

	auto ret = ByteCount{.size = static_cast<float>(byte_count), .unit = "B"};
	if (ret.size < divisor_v) {
		return ret;
	}

	for (auto const* unit : units_v) {
		ret.size /= divisor_v;
		ret.unit = unit;
		if (ret.size < divisor_v) {
			return ret;
		}
	}
	return ret;
}

auto fcopy::to_entry(fs::path path) -> Result<Entry> {
	if (!fs::exists(path)) {
		return to_error(Error::Type::InvalidArgument, std::format("Nonexistent path: {}", path.generic_string()));
	}

	auto const entry_type = to_entry_type(path);
	if (entry_type == EntryType::Directory || entry_type == EntryType::Unknown) {
		return to_error(Error::Type::InvalidArgument, std::format("Not a file: {}", path.generic_string()));
	}

	auto ret = Entry{.path = std::move(path), .type = entry_type};
	if (ret.type == EntryType::RegularFile) {
		ret.size = fs::file_size(ret.path);
	}
	return ret;
}
