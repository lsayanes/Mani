#include "platform/TouchIdAuth.h"

bool touchIdAvailable()
{
#if defined(Q_OS_MACOS)
    return false;
#else
    return false;
#endif
}

bool touchIdAuthenticate(const char * /*reason*/)
{
    return false;
}
