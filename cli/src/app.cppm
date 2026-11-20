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
					clap::named_flag(m_params.options.overwrite, "o,overwrite", "overwrite existing"),
					clap::named_flag(m_params.options.quiet, "q,quiet", "only output errors"),
					clap::named_flag(m_params.options.recursive, "r,recursive", "recurse all files in source"),
					clap::named_option(m_params.destination, "t,target", "destination (default=.)"),
					clap::positional_list(m_params.sources, "SRC", "sources"),
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

	[[nodiscard]] auto execute() const -> int {
		auto sources = std::vector<fs::path>{};
		sources.reserve(m_params.sources.size());
		for (auto const source : m_params.sources) {
			sources.emplace_back(source);
		}

		auto agent = Agent{m_params.options};
		return static_cast<int>(agent.copy_paths(sources, m_params.destination));
	}

	Params m_params{};
};
} // namespace fcopy::cli
