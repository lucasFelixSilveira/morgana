#include "commands.hpp"
#include "compiler_outputs.hpp"
#include "params.hpp"

int
main(int argc, char **argv)
{
    int min_arguments = 2;
    if( argc < min_arguments ) CompilerOutputs::Fatal("You need enter with a action. If you don't know the acceptable actions, use: help.");

    CompilerParams params = CompilerParams::format(argc, argv);
    Commands cmd(params);

    return cmd.status;
}
