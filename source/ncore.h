#pragma once

#ifndef _NCORE_H
#define _NCORE_H

#include "action.hpp"
#include "base64.hpp"
#include "collection.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "dimension_vector.hpp"
#include "disassembly.hpp"
#include "enumeration.hpp"
#include "environment.hpp"
#include "file.hpp"
#include "handle.hpp"
#include "hidden.hpp"
#include "input.hpp"
#include "json.hpp"
#include "pair.hpp"
#include "process.hpp"
#include "readable_byte.hpp"
#include "shared_memory.hpp"
#include "signature.hpp"
#include "silent_library.hpp"
#include "static_array.hpp"
#include "strings.hpp"
#include "tagged_pool.hpp"
#include "task.hpp"
#include "thread.hpp"
#include "time.hpp"
#include "unhashed_map.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "web.hpp"
#include "winreg.hpp"
#include "zip.hpp"

#ifdef NCORE_INCLUDE_KERNELMAP
#include "kernel_map.hpp"
#endif

#endif
