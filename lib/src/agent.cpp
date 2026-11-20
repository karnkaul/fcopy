module fcopy.agent;

namespace fcopy {
namespace {
[[nodiscard]] constexpr auto prioritize(ExitCode const current, ExitCode const next) {
	if (current != ExitCode::Success) {
		return current;
	}
	return next;
}

[[nodiscard]] auto handle_error(Error const& error) {
	std::println(std::cerr, "{}", error.message);
	return to_exit_code(error.type);
}

[[nodiscard]] auto check_is_directory(Entry const& entry) -> Result<void> {
	if (entry.type == Entry::Type::Directory) {
		return {};
	}
	return to_error(Error::Type::InvalidArgument, std::format("Not a directory: {}", entry.path.generic_string()));
}

[[nodiscard]] auto ensure_target_directory(fs::path const& path) -> Result<void> {
	if (fs::is_directory(path)) {
		return {};
	}

	if (!fs::exists(path)) {
		auto err = std::error_code{};
		if (fs::create_directories(path, err)) {
			return {};
		}

		return to_error(Error::Type::DirectoryCreationFailure, path.generic_string());
	}

	return to_error(Error::Type::InvalidArgument, std::format("Invalid target directory: {}", path.generic_string()));
}
} // namespace

Agent::Agent(Options const options) : m_recursive(options.recursive) {
	m_file_copier.overwrite_existing = options.overwrite;
	if (!options.quiet) {
		m_file_copier.progress_listener = std::make_shared<ProgressPrinterListener>();
	}
}

auto Agent::copy_paths(std::span<fs::path const> sources, fs::path const& destination) -> ExitCode {
	if (sources.empty()) {
		return ExitCode::Success;
	}

	if (sources.size() > 1) {
		auto const result = ensure_target_directory(destination);
		if (!result) {
			return handle_error(result.error());
		}
	}

	auto ret = ExitCode::Success;
	for (auto const& source : sources) {
		auto const result = copy_path(source, destination);
		ret = prioritize(ret, result);
	}

	return ret;
}

auto Agent::copy_path(fs::path source, fs::path const& destination) -> ExitCode {
	auto entry = to_entry(std::move(source));
	if (!entry) {
		return handle_error(entry.error());
	}

	if (entry->type == EntryType::Directory) {
		if (!m_recursive) {
			return handle_error(Error{
				.type = ErrorType::InvalidArgument,
				.message =
					std::format("fcopy: -r not specified; omitting directory '{}'", entry->path.generic_string()),
			});
		}

		return copy_directory(*entry, destination);
	}

	return copy_file(std::move(*entry), destination);
}

auto Agent::copy_file(Entry source, fs::path destination) -> ExitCode {
	auto const result = m_file_copier.copy_file(std::move(source), std::move(destination));
	if (!result) {
		return handle_error(result.error());
	}

	return ExitCode::Success;
}

auto Agent::copy_directory(Entry const& source, fs::path const& destination) -> ExitCode {
	if (auto const result = check_is_directory(source); !result) {
		return handle_error(result.error());
	}

	auto ret = ExitCode::Success;
	auto const destination_root = destination / source.path.filename();

	if (auto const result = ensure_target_directory(destination_root); !result) {
		return handle_error(result.error());
	}

	for (auto const& it : fs::recursive_directory_iterator{source.path}) {
		auto entry = to_entry(it.path());
		if (!entry || !FileCopier::can_copy(entry->type)) {
			continue;
		}

		auto const filename = fs::relative(it.path(), source.path);
		auto file_destination = destination_root / filename;
		auto const result = copy_file(std::move(*entry), std::move(file_destination));
		ret = prioritize(ret, result);
	}

	return ret;
}
} // namespace fcopy
