#ifndef __INCLUDE_BITBUFMODULE_H__
#define __INCLUDE_BITBUFMODULE_H__

#include "Macros.h"

extern "C" __declspec(dllexport) void initbitbuf();
DEFINE_CUSTOM_EXCEPTION_DECL(BitBufVectorException, bitbuf)

#endif