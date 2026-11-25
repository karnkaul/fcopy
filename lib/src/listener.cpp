module fcopy.listener;

namespace fcopy {
void ProgressPrinterListener::on_file_copy_progress(Progress const& progress) {
	if (!m_line.empty()) {
		clear_line();
	}

	populate_line(progress);
	std::print("{}", m_line);
	std::flush(std::cout);
}

void ProgressPrinterListener::post_file_copy(Progress const& progress, Result<void> const& result) {
	on_file_copy_progress(progress);
	std::println("");

	if (result) {
		return;
	}
	std::println(std::cerr, "{}", result.error().message);
}

void ProgressPrinterListener::clear_line() {
	m_line.assign(m_line.size(), ' ');
	std::print("\r{}\r", m_line);
}

void ProgressPrinterListener::populate_line(Progress const& progress) {
	m_line.clear();
	auto const percent = progress.to_percent();
	std::format_to(std::back_inserter(m_line), "[{}%] {}", percent, m_path);
	if (!progress.transfer_rate.unit.empty()) {
		std::format_to(std::back_inserter(m_line), " [{}/s]", progress.transfer_rate);
	}
}
} // namespace fcopy
