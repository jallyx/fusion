#if defined(_MSC_VER)
    #include <vld.h>
#endif

//#include "AITest.h"
//#include "EchoTest.h"
#include "ParallelTest.h"

const char *I18N_StrID(uint32 strid) {
    return "";
}

int main(int argc, char **argv)
{
    //AIMain(argc, argv);
    //EchoMain(argc, argv);
    ParallelMain(argc, argv);
    return 0;
}
