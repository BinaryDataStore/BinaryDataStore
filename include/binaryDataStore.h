#pragma once

#include <cstring>
#include <string>
#include <cstdio>
#include "iMemory.h"
#include "memoryConfig.h"
#include "dataParameters.h"

/**
 * @def An class that stores data in memory
 *
 * @param PAGE_SIZE_BYTES the size of a page for the targeted devices memory architecture
 */
template<uint32_t PAGE_SIZE_BYTES>
class BinaryDataStore
{
    private:
        /**
         * @def FlagBuffer defines indexes and their names with respect to contents in the flagBuffer
         */
        struct FlagBuffer {
            static const uint8_t INIT_FLAG                      = 0;
            static const uint8_t TAIL_ADDRESS                   = 1;
            static const uint8_t BYTES_IN_TAIL_BLOCK            = 2;
            static const uint8_t TAIL_WRAPPED_AROUND            = 3;
            static const uint8_t SIZE                           = 4;
        };

        const uint8_t FALSE                                     = 0;
        const uint8_t TRUE                                      = 1;
        const uint32_t INIT_ASCII                               = 0x494E4954;
        const uint32_t FLAG_ADDRESS;
        const uint32_t DATA_LOWER_ADDRESS;
        const uint32_t DATA_UPPER_ADDRESS;

        bool memoryFormatted = false;
        uint32_t flagBuffer[FlagBuffer::SIZE] = {0};
        uint8_t pageBuffer[PAGE_SIZE_BYTES] = {0};
        uint8_t scratchBuffer[PAGE_SIZE_BYTES] = {0};
        iMemory &memory;

        /**
         * @def Assigns the values in tempFlags to the flagBuffer
         *
         * @param tempFlag a data array used to assign values to the flagBuffer
         */
        void resetFlagBuffer(uint32_t tempFlags[FlagBuffer::SIZE]);

        /**
         * @def updates the pagebuffer with new data
         *
         * @param data the data to be added to the pageBuffer
         * @param dataSize the byte size of data
         * @param dataOffset a memory offset from the data address
         * @param dataSliceSize the size of data to be written to memory
         * @param pageBufferOffset a memory offset from the pageBuffer address
         *
         * @return true on success, false otherwise
         */
        bool updatePageBuffer(uint8_t data[], uint32_t dataSize, uint32_t dataOffset, uint32_t dataSliceSize, uint32_t pageBufferOffset);

        /**
         * @def erase a block from memory, writes to that block, then reads that block to confirm the correct data was written.
         *
         * @param writeBuffer data to be written to memory
         * @param bufferSize byte size of writeBuffer
         * @param address the address of the block in memory
         *
         * @return true on success, false otherwise
         */
        template <typename T>
        bool eraseWriteReadConfirm(T writeBuffer[], uint32_t bufferSize, uint32_t address);

        /**
         * @def appends a data array to memory
         *
         * @param data the data array to be stored in memory
         * @param dataSize the byte size of data
         *
         * @return true on success, false otherwise
         */
        bool appendGeneric(uint8_t data[], uint32_t dataSize);

        /**
         * @def Gets the head address of the data in memory
         *
         * @return true on success, false otherwise
         */
        bool getHeadAddress(uint32_t &headAddress);

    public:
        /**
         * @def A constructor for an object that stores data in memory
         *
         * @param memory interface for accessing memory
         * @param config memory configuration details for organizing where data will be placed
         *
         */
        BinaryDataStore(iMemory &memory, MemoryConfig config);

        /**
         * @def formats the memory for use. This should be the first function call after the constructor and
         * must be called for data to be stored to memory via this class.
         *
         * @return true on success, false otherwise
         */
        bool formatMemory(void);

        /**
         * @def adds a string to the binary data store
         *
         * @param text the string of data
         *
         * @return true on success, false otherwise
         */
        bool append(std::string &text);

        /**
         * @def adds an array of data to the binary data store
         *
         * @param data an array of data
         * @param dataSize the byte size of the data
         *
         * @return true on success, false otherwise
         */
        bool append(uint8_t data[], uint32_t dataSize);

        /**
         * @def reads a page of data from the binary data store
         *
         * @param pageAddress to read from
         * @param dataSizeInPage how much data is in the page of memory
         * @param data pointer to where the data read will be held
         * @param dataSize size of the data pointer
         *
         * @return true on success, false otherwise
         */
        bool readPage(uint32_t pageAddess, uint32_t &dataSizeInPage, uint8_t *data, uint32_t dataSize);

        /**
         * @def obtains the bytes of data in memory
         *
         * @return bytes of data in memory or 0 if the memory is not formatted
         */
        uint32_t dataSize(void);

        /**
         * @def gets the size of the data stored in a page of memory starting at the giving address
         *
         * @param pageAddress the address in memory where the page starts
         *
         * @return size of data stored in the selected page of memory
         *
         */
        uint32_t dataLengthInPage(const uint32_t pageAddress);

        /**
         * @def gets the memory address of the next page that is used to store data
         *
         * @param pageAddress the address in memory where the page starts
         *
         * @return size of data stored in the selected page of memory
         */
        uint32_t getNextPageAddress(uint32_t pageAddress);

        /**
         * @def Returns addresses used to manage data held in memory
         *
         * @param dataParameters reference to a DataParameters variable
         *
         * @return true on success, false otherwise
         */
        bool getDataParameters(DataParameters &dataParameters);
};