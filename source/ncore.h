#pragma once

#ifndef _NCORE_H
#define _NCORE_H

#include "base64.hpp"
#include "config.hpp"
#include "enumeration.hpp"
#include "collection.hpp"
#include "defines.hpp"
#include "dimension_vector.hpp"
#include "disassembly.hpp"
#include "environment.hpp"
#include "file.hpp"
#include "handle.hpp"
#include "input.hpp"
#include "process.hpp"
#include "readable_byte.hpp"
#include "shared_memory.hpp"
#include "signature.hpp"
#include "silent_library.hpp"
#include "static_array.hpp"
#include "strings.hpp"
#include "task.hpp"
#include "thread.hpp"
#include "time.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "web.hpp"
#include "zip.hpp"

#ifdef NCORE_INCLUDE_KERNELMAP
#include "kernel_map.hpp"
#endif

#ifdef NCORE_INCLUDE_GWINDOW
#include "gwindow.hpp"
#endif

#endif
