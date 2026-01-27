#pragma once

#include <cstdint>

/**
 * @def holds parameters used to manage data held in memory
 */
typedef struct
{
    uint32_t lowerAddress;          // The first data address in the allocated memory space
    uint32_t upperAddress;          // The last data address in the allocated memory space
    uint32_t headAddress;           // The starting location of the data in memory
    uint32_t tailAddress;           // The ending location of the data in memory
    uint32_t bytesInTailBlock;      // Number of bytes in the tail block of memory
} DataParameters;