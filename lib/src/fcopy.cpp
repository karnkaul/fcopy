module fcopy;

namespace fcopy {
namespace {
[[nodiscard]] auto ensure_directory(fs::path const& path) -> Result<void> {
	auto err = std::error_code{};
	if (!fs::exists(path) && (!fs::create_directories(path) || err != std::error_code{})) {
		return to_error(Error::Type::DirectoryCreationFailure, path.generic_string());
	}
	return {};
}

[[nodiscard]] auto ensure_parent_directory(fs::path const& destination) -> Result<void> {
	if (!destination.has_parent_path()) {
		return {};
	}
	return ensure_directory(destination.parent_path());
}

template <typename Type>
[[nodiscard]] auto open_file(Type& out, fs::path const& path) -> Result<void> {
	out = Type{path, std::ios::binary};
	if (!out) {
		return to_error(Error::Type::FileOpenFailure, path.generic_string());
	}
	return {};
}

[[nodiscard]] auto open_for_read(fs::path const& path) -> Result<std::ifstream> {
	auto ret = std::ifstream{};
	return open_file(ret, path).transform([&] {
		return std::move(ret);
	});
}

[[nodiscard]] auto open_for_write(fs::path const& path) -> Result<std::ofstream> {
	auto ret = std::ofstream{};
	return open_file(ret, path).transform([&] {
		return std::move(ret);
	});
}
} // namespace

void ProgressPrinter::print(CopyProgress const& progress) {
	if (m_printed) {
		std::print("\r");
	}

	auto const percent = progress.to_percent();
	std::print("[{}%] {}", percent, m_path);
	if (percent == 100) {
		std::println("");
	} else {
		std::flush(std::cout);
	}
	m_printed = true;
}

auto FileCopier::copy_file(Entry source, fs::path destination, bool const overwrite) -> Result<void> {
	if (source.type != Entry::Type::RegularFile) {
		return to_error(Error::Type::InvalidArgument,
						std::format("Unsupported EntryType: {}", to_string_view(source.type)));
	}

	if (!overwrite && fs::exists(destination)) {
		return to_error(Error::Type::TargetExists, destination.generic_string());
	}

	m_source = std::move(source);
	m_destination = std::move(destination);
	m_progress = CopyProgress{.total = m_source.size};
	m_print_progress = ProgressPrinter{m_destination};

	return copy_file();
}

auto FileCopier::copy_file() -> Result<void> {
	if (m_source.size == 0) {
		return create_empty_file();
	}

	if (m_source.size <= buffer_size) {
		return ensure_parent_directory(m_destination).and_then([this] {
			return copy_one_shot();
		});
	}

	return open_source().and_then([&] {
		return ensure_parent_directory(m_destination)
			.and_then([&] {
				return open_destination();
			})
			.and_then([&] {
				return copy_bytes();
			});
	});
}

auto FileCopier::create_empty_file() -> Result<void> {
	return open_for_write(m_destination).transform([&](std::ofstream file) {
		file.close();
		overwrite_permissions();
		m_progress.synchronized = m_progress.total;
		m_print_progress->print(m_progress);
	});
}

auto FileCopier::copy_one_shot() -> Result<void> {
	auto err = std::error_code{};
	if (!fs::copy_file(m_source.path, m_destination, err)) {
		return to_error(Error::Type::FileIOFailure, m_destination.generic_string());
	}

	overwrite_permissions();
	m_progress.synchronized = m_progress.total;
	m_print_progress->print(m_progress);

	return {};
}

auto FileCopier::open_source() -> Result<void> {
	return open_for_read(m_source.path).transform([&](std::ifstream file) {
		m_in = std::move(file);
	});
}

auto FileCopier::open_destination() -> Result<void> {
	return open_for_write(m_destination).transform([&](std::ofstream file) {
		m_out = std::move(file);
	});
}

auto FileCopier::copy_bytes() -> Result<void> {
	m_buffer.clear();
	m_buffer.resize(buffer_size);

	m_print_progress->print(m_progress);
	for (auto remain = m_source.size; remain > 0;) {
		auto const request_size = std::min(m_buffer.size(), remain);
		if (!m_in.read(m_buffer.data(), static_cast<std::streamsize>(request_size))) {
			return to_error(Error::Type::FileIOFailure, m_source.path.generic_string());
		}
		if (!m_out.write(m_buffer.data(), static_cast<std::streamsize>(request_size))) {
			return to_error(Error::Type::FileIOFailure, m_destination.generic_string());
		}

		remain -= request_size;
		m_progress.synchronized += request_size;
		m_print_progress->print(m_progress);
	}

	overwrite_permissions();
	return {};
}

void FileCopier::overwrite_permissions() const {
	fs::permissions(m_destination, fs::status(m_source.path).permissions());
}
} // namespace fcopy
