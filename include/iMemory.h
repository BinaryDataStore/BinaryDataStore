#pragma once

#include <cstdint>
#include <cstdbool>

/**
 * @def Interface for managing memory
 */
class iMemory
{
    public:
        /**
         * @def reads data from memory
         *
         * @param data pointer to were data read will be held
         * @param length size of the data pointer
         * @param address memory address to read from
         *
         * @return true on success, false otherwise
         */
        virtual bool read(void* data, uint32_t length, const uint32_t address) = 0;

        /**
         * @def writes data to memory
         *
         * @param data pointer to data that will be written to memory
         * @param length length of the data held in the data pointer
         * @param address memory address to write to
         *
         * @return true on success, false otherwise
         */
        virtual bool write(void *data, uint32_t length, const uint32_t address) = 0;

        /**
         * @def erases a page from memory at the selected memory address
         *
         * @param address the address to erase a page from
         */
        virtual bool erase(uint32_t address) = 0;
};