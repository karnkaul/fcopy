module fcopy.file_copier;

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

void sync_permissions(fs::path const& source, fs::path const& destination) {
	auto err = std::error_code{};
	fs::permissions(destination, fs::status(source).permissions(), err);
}
} // namespace

void TransferMeter::update(std::uint64_t const transferred) {
	m_transferred += transferred;
	auto const dt = std::chrono::duration<float>(Clock::now() - m_start);
	if (dt < 10ms) {
		return;
	}

	auto const bytes_per_second = static_cast<float>(m_transferred) / dt.count();
	m_rate = to_byte_count(static_cast<std::uint64_t>(bytes_per_second));
}

auto FileCopier::copy_file(fs::path source, fs::path destination) -> Result<void> {
	auto entry = to_entry(std::move(source));
	if (!entry) {
		return std::unexpected{std::move(entry.error())};
	}

	return copy_file(std::move(*entry), std::move(destination));
}

auto FileCopier::copy_file(Entry source, fs::path destination) -> Result<void> {
	if (!can_copy(source.type)) {
		return to_error(
			Error::Type::InvalidArgument,
			std::format("Unsupported EntryType ({}): {}", to_string_view(source.type), source.path.generic_string()));
	}

	if (!overwrite_existing && fs::exists(destination)) {
		return to_error(Error::Type::TargetExists, destination.generic_string());
	}

	if (destination.has_parent_path() && !fs::is_directory(destination.parent_path())) {
		return to_error(Error::Type::InvalidArgument,
						std::format("Invalid destination directory: {}", destination.parent_path().generic_string()));
	}

	m_source = std::move(source);
	m_destination = std::move(destination);
	m_progress = Progress{.total = m_source.size};

	auto ret = copy_file();
	dispatch_post_copy(ret);

	return ret;
}

void FileCopier::reset_transfer_rate() { m_transfer = {}; }

auto FileCopier::copy_file() -> Result<void> {
	dispatch_pre_copy();

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
	static constexpr auto options_v = fs::copy_options::overwrite_existing;
	if (!fs::copy_file(m_source.path, m_destination, options_v, err)) {
		return to_error(Error::Type::FileIOFailure, m_destination.generic_string());
	}

	m_transfer.update(m_source.size);
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

	for (auto remain = m_source.size; remain > 0;) {
		dispatch_progress();

		auto const request_size = std::min(m_buffer.size(), remain);
		if (!m_in.read(m_buffer.data(), static_cast<std::streamsize>(request_size))) {
			return to_error(Error::Type::FileIOFailure, m_source.path.generic_string());
		}
		if (!m_out.write(m_buffer.data(), static_cast<std::streamsize>(request_size))) {
			return to_error(Error::Type::FileIOFailure, m_destination.generic_string());
		}

		remain -= request_size;

		m_progress.synchronized += request_size;
		m_transfer.update(request_size);
		m_progress.transfer_rate = m_transfer.get_rate();
	}

	m_in.close();
	m_out.close();

	set_completed();
	return {};
}

void FileCopier::set_completed() {
	m_progress.synchronized = m_progress.total;
	m_progress.transfer_rate = m_transfer.get_rate();

	dispatch_progress();

	sync_permissions(m_source.path, m_destination);
}

void FileCopier::dispatch_pre_copy() const {
	if (!progress_listener) {
		return;
	}
	progress_listener->pre_file_copy(m_source, m_destination);
}

void FileCopier::dispatch_progress() const {
	if (!progress_listener) {
		return;
	}
	progress_listener->on_file_copy_progress(m_progress);
}

void FileCopier::dispatch_post_copy(Result<void> const& result) const {
	if (!progress_listener) {
		return;
	}
	progress_listener->post_file_copy(m_progress, result);
}
} // namespace fcopy
