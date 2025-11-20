
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <va_inf.h>

int
main(int argc, char** argv)
{
    struct va_funcs fns;
    int error;

    error = va_inf_get_funcs(&fns, VI_VERSION_INT(VI_MAJOR, VI_MINOR));
    printf("main: va_inf_get_funcs rv %d\n", error);
    if (error == 0)
    {
        int version;
        error = fns.get_version(&version);
        printf("main: get_version rv %d version 0x%8.8X\n", error, version);
     
        void* enc;
        error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, 0);
        printf("main: encoder_create rv %d\n", error);
    }

    return 0;
}