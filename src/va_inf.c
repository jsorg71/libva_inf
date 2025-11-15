
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>

#include "va_inf.h"

/*****************************************************************************/
int
yami_get_version(int *version)
{
    *version = YI_VERSION_INT(YI_MAJOR, YI_MINOR);
    return YI_SUCCESS;
}
