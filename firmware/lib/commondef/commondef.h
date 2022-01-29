#pragma once


#define INVALID_UINT8  (0xFF)
#define INVALID_UINT16 (0xFFFF)
#define INVALID_UINT32 (0xFFFFFFFF)

#ifndef __FILENAME__
#define __DIR_SEPARATOR__ '/'
#define __FILENAME__ (strrchr(__FILE__, __DIR_SEPARATOR__) ? strrchr(__FILE__, __DIR_SEPARATOR__) + 1 : __FILE__)
#endif
