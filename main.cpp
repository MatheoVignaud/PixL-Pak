#include <PixL_Pak.hpp>

int main(int argc, char *argv[])
{
    bool print_help = false;

    for (int i = 0; i < argc; ++i)
    {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
    }

    if (argc >= 4 && argc <= 5)
    {
        bool mapping = false;
        bool pak = false;
        std::string arg1 = argv[1];
        if (arg1 == "-p")
        {
            pak = true;
        }
        else if (arg1 == "-u")
        {
            pak = false;
        }
        else
        {
            std::cout << "Incorrect argument" << std::endl;
            print_help = true;
        }

        // check if argv[1] is valid path
        std::string path = argv[2];
        if (path.empty() || path[0] != '/' && path[0] != '.')
        {
            std::cout << "The path must start with '/' or './'" << std::endl;
            print_help = true;
        }

        // check if argv[2] ends with .pak
        std::string pak_file = argv[3];
        if (pak_file.size() < 4 || pak_file.substr(pak_file.size() - 4) != ".pak")
        {
            std::cout << "The pak file must end with .pak" << std::endl;
            print_help = true;
        }

        if (argc == 5)
        {
            std::string argv4 = argv[4];
            if (argv4 == "-m")
            {
                mapping = true;
            }
        }

        if (!print_help)
        {
            if (pak)
            {
                PixL_Pak::Pak(path, pak_file, mapping);
            }
            else
            {
                PixL_Pak::Context *context = PixL_Pak::Open(pak_file);
                if (context)
                {
                    if (!context->extract(path))
                    {
                        std::cerr << "Failed to extract files from pak." << std::endl;
                    }
                    delete context;
                }
            }
        }
    }
    else
    {
        print_help = true;
    }

    if (print_help)
    {
        std::cout << "Usage: " << argv[0] << " [-p | -u] <dir> <pak_file> [-m]" << std::endl;
    }
    return 0;
}