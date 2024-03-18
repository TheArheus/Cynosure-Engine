
#include <windows.h>
#include <string>
#include <vector>

[[nodiscard]] int engine_main([[maybe_unused]] const std::vector<std::string>& args);

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{
    int argc = 0;
    LPWSTR *argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) 
	{
        int required_size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        std::string arg(required_size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, arg.data(), required_size, NULL, NULL);
        args.push_back(arg);
    }
    LocalFree(argvW);

    return engine_main(args);
}

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

	return engine_main(args);
}
