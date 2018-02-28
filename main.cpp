#if defined(_MSC_VER)
    #include <vld.h>
#endif
//#include "AITest.h"
//#include "EchoTest.h"
//#include "ParallelTest.h"
#include "SessionlessTest.h"

int main(int argc, char **argv)
{
    //AIMain(argc, argv);
    //EchoMain(argc, argv);
    //ParallelMain(argc, argv);
    SessionlessMain(argc, argv);
    return 0;
}
