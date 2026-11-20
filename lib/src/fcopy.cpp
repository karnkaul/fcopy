module fcopy;

import fcopy.file_copier;

namespace fcopy {
namespace {
[[nodiscard]] auto check_is_directory(Entry const& entry) -> Result<void> {
	if (entry.type == Entry::Type::Directory) {
		return {};
	}
	return to_error(Error::Type::InvalidArgument, std::format("Not a directory: {}", entry.path.generic_string()));
}
} // namespace
} // namespace fcopy

auto fcopy::copy_file(Entry source, fs::path destination, Options const options) -> ExitCode {
	auto copier = FileCopier{};
	copier.overwrite_existing = options.overwrite;
	if (options.quiet) {
		copier.progress_listener.reset();
	}

	auto const result = copier.copy_file(std::move(source), std::move(destination));
	if (result) {
		return ExitCode::Success;
	}

	std::println(std::cerr, "{}", result.error().message);
	return to_exit_code(result.error().type);
}

auto fcopy::copy_directory(Entry const& source, fs::path const& destination, Options const options) -> ExitCode {
	if (auto const result = check_is_directory(source); !result) {
		std::println(std::cerr, "{}", result.error().message);
		return to_exit_code(result.error().type);
	}

	auto copier = FileCopier{};
	copier.overwrite_existing = options.overwrite;
	if (options.quiet) {
		copier.progress_listener.reset();
	}

	auto ret = ExitCode::Success;
	auto const destination_root = destination / source.path.filename();
	for (auto const& it : fs::recursive_directory_iterator{source.path}) {
		auto entry = to_entry(it.path());
		if (!entry || !FileCopier::can_copy(entry->type)) {
			continue;
		}

		auto const filename = fs::relative(it.path(), source.path);
		auto file_destination = destination_root / filename;
		auto result = copier.copy_file(std::move(*entry), std::move(file_destination));
		if (!result) {
			std::println(std::cerr, "{}", result.error().message);
			ret = to_exit_code(result.error().type);
		}
	}

	return ret;
}

auto fcopy::copy_path(fs::path source, fs::path destination, Options const options) -> ExitCode {
	auto entry = to_entry(std::move(source));
	if (!entry) {
		std::println(std::cerr, "{}", entry.error().message);
		return to_exit_code(entry.error().type);
	}

	if (entry->type == EntryType::Directory) {
		return copy_directory(*entry, destination, options);
	}
	return copy_file(std::move(*entry), std::move(destination), options);
}
