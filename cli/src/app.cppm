export module fcopy.cli;

import fcopy;
import clap;

export namespace fcopy::cli {
class App {
  public:
	struct Params {
		std::string_view source;

		std::string_view destination{"."};
		bool overwrite{false};
	};

	[[nodiscard]] auto run(int argc, char const* const* argv) -> int {
		auto const parse_result = parse_args(argc, argv);
		if (parse_result.should_early_exit()) {
			return parse_result.return_code();
		}

		return execute();
	}

  private:
	[[nodiscard]] auto parse_args(int argc, char const* const* argv) -> clap::Result {
		auto spec = clap::spec::Parameters{
			.parameters =
				{
					clap::named_flag(m_params.overwrite, "o,overwrite", "overwrite existing"),
					clap::positional_required(m_params.source, "SRC", "source"),
					clap::positional_optional(m_params.destination, "DST", "destination (default=.)"),
				},
			.program =
				{
					.name = "fcopy",
					.version = build_version_v,
				},
		};

		auto parser = clap::Parser{std::move(spec)};
		return parser.parse_main(argc, argv);
	}

	[[nodiscard]] auto execute() -> int {
		auto source = to_entry(m_params.source);
		if (!source) {
			return handle_error(source.error());
		}

		auto destination = fs::path(m_params.destination);
		if (fs::is_directory(destination)) {
			destination /= fs::path{m_params.source}.filename();
		}

		auto const result = m_file_copier.copy_file(std::move(*source), destination, m_params.overwrite);
		if (!result) {
			return handle_error(result.error());
		}

		return 0;
	}

	[[nodiscard]] static auto handle_error(Error const& error) -> int {
		std::println(std::cerr, "{}", error.message);
		return static_cast<int>(error.type);
	}

	Params m_params{};

	FileCopier m_file_copier{};
};
} // namespace fcopy::cli
