#ifndef WOMBAT_HAIKU_ENDIAN_H
#define WOMBAT_HAIKU_ENDIAN_H

#ifdef __HAIKU__

#include <ByteOrder.h>

#ifndef le16toh
#define le16toh(value) B_LENDIAN_TO_HOST_INT16(value)
#endif

#ifndef be16toh
#define be16toh(value) B_BENDIAN_TO_HOST_INT16(value)
#endif

#endif

#endif
