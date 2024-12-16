#include <string>
#include <vector>

[[nodiscard]] int engine_main([[maybe_unused]] const std::vector<std::string>& args);

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
	{
        args.push_back(argv[i]);
	}

	return engine_main(args);
}
