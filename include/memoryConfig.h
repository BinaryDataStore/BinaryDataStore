#pragma once

#include <cstddef>

/**
 * @def data structure for configuring memory
 */
struct MemoryConfig
{
    /**
     * @def starting location in memory that will be allocated for log use
     */
    uint32_t startAddress             = 0;

    /**
     * @def bytes of memory to be allocated for use
     */
    uint32_t bytesAllocated           = 0;

};