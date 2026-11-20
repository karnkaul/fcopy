import fcopy.cli;
import std;

int main(int argc, char** argv) {
	try {
		return fcopy::cli::App{}.run(argc, argv);
	} catch (std::exception const& e) {
		std::println(std::cerr, "PANIC: {}", e.what());
		return 1;
	} catch (...) {
		std::println(std::cerr, "PANIC!");
		return 1;
	}
}
