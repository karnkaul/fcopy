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

[[nodiscard]] auto ensure_target_directory(fs::path const& path, bool const create) -> Result<void> {
	if (fs::is_directory(path)) {
		return {};
	}

	if (!fs::exists(path) && create) {
		auto err = std::error_code{};
		if (fs::create_directories(path, err)) {
			return {};
		}

		return to_error(Error::Type::DirectoryCreationFailure, path.generic_string());
	}

	return to_error(Error::Type::InvalidArgument, std::format("Invalid target directory: {}", path.generic_string()));
}

void to_lower(std::string& out) {
	static constexpr auto delta_v = static_cast<char>('a' - 'A');
	for (char& c : out) {
		if (c >= 'A' && c <= 'Z') {
			c += delta_v;
		}
	}
}

[[nodiscard]] constexpr auto is_affirmative(std::string_view const lowered_input) {
	return lowered_input == "y" || lowered_input == "yes" || lowered_input == "true" || lowered_input == "1";
}
} // namespace

Agent::Agent(Options const options)
	: m_app_name(options.app_name),
	  m_force(options.force),
	  m_recursive(options.recursive) {
	m_file_copier.overwrite_existing = true;
	if (!options.quiet) {
		m_file_copier.progress_listener = std::make_shared<ProgressPrinterListener>();
	}
}

auto Agent::copy_paths(std::span<fs::path const> sources, fs::path const& target_directory) -> ExitCode {
	if (sources.empty()) {
		return ExitCode::Success;
	}

	auto const create_target = m_force || m_recursive;
	auto const result = ensure_target_directory(target_directory, create_target);
	if (!result) {
		return handle_error(result.error());
	}

	auto ret = ExitCode::Success;
	for (auto const& source : sources) {
		auto const result = copy_path(source, target_directory);
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
		return copy_directory(*entry, destination);
	}

	return copy_file(std::move(*entry), destination);
}

auto Agent::copy_file(Entry source, fs::path destination) -> ExitCode {
	if (m_force && destination.has_parent_path()) {
		auto const result = ensure_target_directory(destination.parent_path(), true);
		if (!result) {
			return handle_error(result.error());
		}
	}

	if (fs::is_directory(destination)) {
		destination /= source.path.filename();
	}

	if (!m_force && fs::exists(destination)) {
		auto const exit_code = prompt_overwrite(destination);
		m_file_copier.reset_transfer_rate();

		if (exit_code != ExitCode::Success) {
			return exit_code;
		}
	}

	auto const result = m_file_copier.copy_file(std::move(source), std::move(destination));
	if (!result) {
		return handle_error(result.error());
	}

	return ExitCode::Success;
}

auto Agent::copy_directory(Entry const& source, fs::path const& destination_directory) -> ExitCode {
	if (!m_recursive) {
		return handle_error(Error{
			.type = ErrorType::InvalidArgument,
			.message = std::format("fcopy: -r not specified; omitting directory '{}'", source.path.generic_string()),
		});
	}

	m_source_root = source.path;
	m_destination_root = destination_directory / source.path.filename();

	return copy_directory_recursive({});
}

auto Agent::copy_directory_recursive(fs::path const& relative) -> ExitCode {
	auto const destination_directory = m_destination_root / relative;

	if (auto const result = ensure_target_directory(destination_directory, true); !result) {
		return handle_error(result.error());
	}

	auto err = std::error_code{};
	auto const path = m_source_root / relative;
	auto iter = fs::directory_iterator{path, err};
	if (iter == fs::directory_iterator{}) {
		return handle_error(to_error(Error::Type::DirectoryOpenFailure, path.generic_string()).error());
	}

	auto ret = ExitCode::Success;
	for (auto const& it : iter) {
		auto entry = to_entry(it.path());
		if (!entry) {
			continue;
		}

		if (entry->type == EntryType::Directory) {
			auto const result = copy_directory_recursive(fs::relative(entry->path, m_source_root));
			ret = prioritize(ret, result);
			continue;
		}

		if (!FileCopier::can_copy(entry->type)) {
			continue;
		}

		auto const result = copy_file(std::move(*entry), destination_directory);
		ret = prioritize(ret, result);
	}

	return ret;
}

auto Agent::prompt_overwrite(fs::path const& destination) const -> ExitCode {
	std::print("{}: overwrite '{}'? ", m_app_name, destination.generic_string());
	std::flush(std::cout);

	auto response = std::string{};
	std::getline(std::cin, response);
	to_lower(response);

	if (is_affirmative(response)) {
		return ExitCode::Success;
	}

	return static_cast<ExitCode>(ErrorType::NegativeInteraction);
}
} // namespace fcopy
