
#include "os_files.cpp"
#include "os_network.cpp"
#include "os_semaphore.cpp"

#if OS_LINUX
#include "linux/linux_inc.cpp"
#elif OS_WINDOWS
#include "win32/win32_inc.cpp"
#else
# error Provide Platform Implementations
#endif

