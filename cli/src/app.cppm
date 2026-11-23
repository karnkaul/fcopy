export module fcopy.cli;

import fcopy;
import clap;

export namespace fcopy::cli {
class App {
  public:
	[[nodiscard]] auto run(int argc, char const* const* argv) -> int {
		auto const parse_result = parse_args(argc, argv);
		if (parse_result.should_early_exit()) {
			return parse_result.return_code();
		}

		if (m_params.sources.empty()) {
			std::println(std::cerr, "fcopy: missing SRC");
			return 1;
		}

		return execute();
	}

  private:
	struct Params {
		std::string_view destination{"."};
		std::vector<std::string_view> sources{};
		AgentOptions options{};
	};

	[[nodiscard]] auto parse_args(int argc, char const* const* argv) -> clap::Result {
		auto spec = clap::spec::Parameters{
			.parameters =
				{
					clap::named_flag(m_params.options.force, "f,force", "overwrite existing"),
					clap::named_flag(m_params.options.quiet, "q,quiet", "only output errors"),
					clap::named_flag(m_params.options.recursive, "r,recursive", "recurse all files in source"),
					clap::positional_list(m_params.sources, "SRC", "sources"),
					clap::positional_required(m_params.destination, "DST", "destination"),
				},
			.program =
				{
					.name = "fcopy",
					.version = build_version_v,
				},
		};

		return clap::parse_main(std::move(spec), argc, argv);
	}

	[[nodiscard]] auto execute() const -> int {
		auto sources = std::vector<fs::path>{};
		sources.reserve(m_params.sources.size());
		for (auto const source : m_params.sources) {
			sources.emplace_back(source);
		}

		auto agent = Agent{m_params.options};
		if (sources.size() == 1) {
			return static_cast<int>(agent.copy_path(sources.front(), m_params.destination));
		}

		return static_cast<int>(agent.copy_paths(sources, m_params.destination));
	}

	Params m_params{};
};
} // namespace fcopy::cli
