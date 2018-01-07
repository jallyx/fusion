#if defined(_MSC_VER)
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

#include "AITest.h"
#include "EchoTest.h"

int main(int argc, char **argv)
{
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    AIMain(argc, argv);
    EchoMain(argc, argv);
    return 0;
}
