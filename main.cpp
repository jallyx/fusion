#if defined(_MSC_VER)
    #include <vld.h>
#endif
#include "AITest.h"
#include "EchoTest.h"

int main(int argc, char **argv)
{
    AIMain(argc, argv);
    EchoMain(argc, argv);
    return 0;
}
