module fcopy;

namespace fcopy {
using namespace std::chrono_literals;

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

void ProgressPrinter::print(CopyProgress const& progress, ByteCount const& transfer_rate) {
	if (!m_line.empty()) {
		clear_line();
	}

	populate_line(progress, transfer_rate);
	std::print("{}", m_line);
	if (progress.to_percent() == 100) {
		std::println("");
	} else {
		std::flush(std::cout);
	}
}

void ProgressPrinter::clear_line() {
	m_line.assign(m_line.size(), ' ');
	std::print("\r{}\r", m_line);
}

void ProgressPrinter::populate_line(CopyProgress const& progress, ByteCount const& transfer_rate) {
	m_line.clear();
	auto const percent = progress.to_percent();
	std::format_to(std::back_inserter(m_line), "[{}%] {}", percent, m_path);
	if (!transfer_rate.unit.empty()) {
		std::format_to(std::back_inserter(m_line), " [{}/s]", transfer_rate);
	}
}

void TransferMeter::update(std::uint64_t const transferred) {
	m_transferred += transferred;
	auto const now = Clock::now();
	if (now - m_previous >= 1s) {
		m_rate = to_byte_count(m_transferred);
		m_transferred = 0;
		m_previous = now;
	}
}

auto TransferMeter::get_rate() const -> ByteCount {
	if (m_rate.unit.empty()) {
		return to_byte_count(m_transferred);
	}
	return m_rate;
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
	if (!quiet) {
		m_print_progress = ProgressPrinter{m_destination};

		if (!transfer_meter) {
			transfer_meter = std::make_shared<TransferMeter>();
		}
	}

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
		set_completed();
	});
}

auto FileCopier::copy_one_shot() -> Result<void> {
	auto err = std::error_code{};
	if (!fs::copy_file(m_source.path, m_destination, err)) {
		return to_error(Error::Type::FileIOFailure, m_destination.generic_string());
	}

	if (transfer_meter) {
		transfer_meter->update(m_source.size);
	}
	set_completed();

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

	if (!quiet) {
		m_print_progress->print(m_progress, transfer_meter->get_rate());
	}

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

		if (!quiet) {
			transfer_meter->update(request_size);
			m_print_progress->print(m_progress, transfer_meter->get_rate());
		}
	}

	overwrite_permissions();
	return {};
}

void FileCopier::overwrite_permissions() const {
	auto err = std::error_code{};
	fs::permissions(m_destination, fs::status(m_source.path).permissions(), err);
}

void FileCopier::set_completed() {
	overwrite_permissions();
	m_progress.synchronized = m_progress.total;
	if (!quiet) {
		m_print_progress->print(m_progress, transfer_meter->get_rate());
	}
}
} // namespace fcopy
