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
        void resetFlagBuffer(uint32_t tempFlags[FlagBuffer::SIZE])
        {
            flagBuffer[FlagBuffer::INIT_FLAG] = tempFlags[FlagBuffer::INIT_FLAG];
            flagBuffer[FlagBuffer::TAIL_ADDRESS] = tempFlags[FlagBuffer::TAIL_ADDRESS];
            flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK] = tempFlags[FlagBuffer::BYTES_IN_TAIL_BLOCK];
            flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND] = tempFlags[FlagBuffer::TAIL_WRAPPED_AROUND];
        }

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
        bool updatePageBuffer(uint8_t data[], uint32_t dataSize, uint32_t dataOffset, uint32_t dataSliceSize, uint32_t pageBufferOffset)
        {
            bool status = false;

            if ((pageBufferOffset + dataSliceSize > PAGE_SIZE_BYTES) || (dataOffset + dataSliceSize > dataSize) || dataSize > PAGE_SIZE_BYTES)
            {
                return status;
            }

            memcpy(pageBuffer + pageBufferOffset, data + dataOffset, dataSliceSize);

            flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK] += dataSliceSize;

            status = true;

            return status;
        }

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
        bool eraseWriteReadConfirm(T writeBuffer[], uint32_t bufferSize, uint32_t address)
        {
            bool status = false;

            T *readBuffer = reinterpret_cast<T*>(scratchBuffer);

            if (!memory.erase(address))
            {
                writeBuffer[bufferSize - 1] = 0;
                return status;
            }

            if (!memory.write(writeBuffer, bufferSize, address))
            {
                writeBuffer[bufferSize - 1] = 0;
                return status;
            }

            if (!memory.read(readBuffer, bufferSize, address))
            {
                writeBuffer[bufferSize - 1] = 0;
                return status;
            }

            if (memcmp(readBuffer, writeBuffer, bufferSize) != 0)
            {
                writeBuffer[bufferSize - 1] = 0;
                return status;
            }

            status = true;
            return status;
        }

        /**
         * @def appends a data array to memory
         *
         * @param data the data array to be stored in memory
         * @param dataSize the byte size of data
         *
         * @return true on success, false otherwise
         */
        bool appendGeneric(uint8_t data[], uint32_t dataSize)
        {
            const uint32_t BYTES_REMAINING_IN_PAGE = PAGE_SIZE_BYTES - flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK];
            bool status = false;
            uint32_t firstWriteSize_bytes = dataSize;
            uint32_t secondWriteSize_bytes = 0;
            uint32_t tempFlags[FlagBuffer::SIZE] = {
                flagBuffer[FlagBuffer::INIT_FLAG],
                flagBuffer[FlagBuffer::TAIL_ADDRESS],
                flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK],
                flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND]
            };

            // Must be less than the scratch buffer size
            if (dataSize > PAGE_SIZE_BYTES)
            {
                return false;
            }

            // Get the second write size if needed.
            if ( firstWriteSize_bytes > BYTES_REMAINING_IN_PAGE)
            {
                firstWriteSize_bytes = BYTES_REMAINING_IN_PAGE;
                secondWriteSize_bytes = dataSize - firstWriteSize_bytes;
            }

            if (!updatePageBuffer(data, dataSize, 0, firstWriteSize_bytes, flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK]))
            {
                resetFlagBuffer(tempFlags);
                return status;
            }

            if (!eraseWriteReadConfirm(pageBuffer, flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK], flagBuffer[FlagBuffer::TAIL_ADDRESS]))
            {
                resetFlagBuffer(tempFlags);
                return status;
            }

            // Check if we need to finish writing data
            if (secondWriteSize_bytes > 0)
            {
                // Get next page address
                flagBuffer[FlagBuffer::TAIL_ADDRESS] = flagBuffer[FlagBuffer::TAIL_ADDRESS] + PAGE_SIZE_BYTES;
                if (flagBuffer[FlagBuffer::TAIL_ADDRESS] >= DATA_UPPER_ADDRESS)
                {
                    flagBuffer[FlagBuffer::TAIL_ADDRESS] = DATA_LOWER_ADDRESS;
                    flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND] = TRUE;
                }

                // clear buffer
                flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK] = 0;

                if (!updatePageBuffer(data, dataSize, firstWriteSize_bytes, secondWriteSize_bytes, flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK]))
                {
                    resetFlagBuffer(tempFlags);
                    return status;
                }

                // Write to memory
                if (!eraseWriteReadConfirm(pageBuffer, flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK], flagBuffer[FlagBuffer::TAIL_ADDRESS]))
                {
                    resetFlagBuffer(tempFlags);
                    return status;
                }
            }

            // Update Flags
            if (!eraseWriteReadConfirm(flagBuffer, sizeof(flagBuffer), FLAG_ADDRESS))
            {
                resetFlagBuffer(tempFlags);
                return status;
            }

            status = true;

            return status;
        }

        /**
         * @def Gets the head address of the data in memory
         *
         * @return true on success, false otherwise
         */
        bool getHeadAddress(uint32_t &headAddress)
        {
            if (!memoryFormatted)
            {
                return false;
            }

            if (flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND] && (flagBuffer[FlagBuffer::TAIL_ADDRESS] + PAGE_SIZE_BYTES < DATA_UPPER_ADDRESS))
            {
                headAddress = flagBuffer[FlagBuffer::TAIL_ADDRESS] + PAGE_SIZE_BYTES;
            }
            else
            {
                headAddress = DATA_LOWER_ADDRESS;
            }

            return true;
        }

    public:
        /**
         * @def A constructor for an object that stores data in memory
         *
         * @param memory interface for accessing memory
         * @param config memory configuration details for organizing where data will be placed
         *
         */
        BinaryDataStore(iMemory &memory, MemoryConfig config) :
            memory(memory),
            FLAG_ADDRESS(config.startAddress),
            DATA_LOWER_ADDRESS(config.startAddress + PAGE_SIZE_BYTES),
            DATA_UPPER_ADDRESS((config.startAddress + config.bytesAllocated) -
                (config.bytesAllocated % PAGE_SIZE_BYTES))
        {}

        /**
         * @def formats the memory for use. This should be the first function call after the constructor and
         * must be called for data to be stored to memory via this class.
         *
         * @return true on success, false otherwise
         */
        bool formatMemory(void)
        {
            bool status = false;

            if (DATA_LOWER_ADDRESS >= DATA_UPPER_ADDRESS)
            {
                return status;
            }

            // Read the flags from memory
            if (!memory.read( flagBuffer, sizeof(flagBuffer), FLAG_ADDRESS))
            {
                return status;
            }

            // Verify if the device has already been programmed and pull data if so
            if (INIT_ASCII == flagBuffer[FlagBuffer::INIT_FLAG])
            {
                // Fill the page buffer with current data
                if (memory.read( pageBuffer, PAGE_SIZE_BYTES, flagBuffer[FlagBuffer::TAIL_ADDRESS]))
                {
                    status = true;
                }
            }
            else
            {
                flagBuffer[FlagBuffer::INIT_FLAG]                       = INIT_ASCII;
                flagBuffer[FlagBuffer::TAIL_ADDRESS]                    = DATA_LOWER_ADDRESS;
                flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK]              = 0;
                flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND]             = FALSE;

                if (eraseWriteReadConfirm(flagBuffer, sizeof(flagBuffer), FLAG_ADDRESS))
                {
                    status = true;
                }
            }

            memoryFormatted = true;

            return status;
        }

        /**
         * @def adds a string to the binary data store
         *
         * @param text the string of data
         *
         * @return true on success, false otherwise
         */
        bool append(std::string &text)
        {
            if (!memoryFormatted)
            {
                return false;
            }

            uint8_t data[text.size()];
            memcpy(data, text.data(), text.size());

            return appendGeneric(data, sizeof(data));
        }

        /**
         * @def adds an array of data to the binary data store
         *
         * @param data an array of data
         * @param dataSize the byte size of the data
         *
         * @return true on success, false otherwise
         */
        bool append(uint8_t data[], uint32_t dataSize)
        {
            if (!memoryFormatted || data == nullptr)
            {
                return false;
            }

            return appendGeneric(data, dataSize);
        }

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
        bool readPage(uint32_t pageAddess, uint32_t &dataSizeInPage, uint8_t *data, uint32_t dataSize)
        {
            if (dataSize < PAGE_SIZE_BYTES || data == nullptr)
            {
                return false;
            }

            dataSizeInPage = dataLengthInPage(pageAddess);

            return memory.read( data, dataSizeInPage, pageAddess );
        }

        /**
         * @def obtains the bytes of data in memory
         *
         * @return bytes of data in memory or 0 if the memory is not formatted
         */
        uint32_t dataSize(void)
        {
            uint32_t dataSize = 0;

            if (!memoryFormatted)
            {
                return dataSize;
            }

            if (flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND])
            {
                uint32_t memorySize = DATA_UPPER_ADDRESS - DATA_LOWER_ADDRESS;
                dataSize = (memorySize - PAGE_SIZE_BYTES) + flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK];
            }
            else
            {
                dataSize = (flagBuffer[FlagBuffer::TAIL_ADDRESS] + flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK]) - DATA_LOWER_ADDRESS;
            }

            return dataSize;
        }

        /**
         * @def gets the size of the data stored in a page of memory starting at the giving address
         *
         * @param pageAddress the address in memory where the page starts
         *
         * @return size of data stored in the selected page of memory
         *
         */
        uint32_t dataLengthInPage(const uint32_t pageAddress)
        {
            if (pageAddress == flagBuffer[FlagBuffer::TAIL_ADDRESS])
            {
                return flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK];
            }

            return PAGE_SIZE_BYTES;
        }

        /**
         * @def gets the memory address of the next page that is used to store data
         *
         * @param pageAddress the address in memory where the page starts
         *
         * @return size of data stored in the selected page of memory
         */
        uint32_t getNextPageAddress(uint32_t pageAddress)
        {
            uint32_t nextPageAddress = pageAddress + PAGE_SIZE_BYTES;
            if (nextPageAddress >= DATA_UPPER_ADDRESS)
            {
                return DATA_LOWER_ADDRESS;
            }

            return nextPageAddress;
        }

        /**
         * @def Returns addresses used to manage data held in memory
         *
         * @param dataParameters reference to a DataParameters variable
         *
         * @return true on success, false otherwise
         */
        bool getDataParameters(DataParameters &dataParameters)
        {
            if (!memoryFormatted)
            {
                return false;
            }

            uint32_t headAddress = 0;
            getHeadAddress(headAddress);

            dataParameters.lowerAddress = DATA_LOWER_ADDRESS;
            dataParameters.upperAddress = DATA_UPPER_ADDRESS;
            dataParameters.headAddress = headAddress;
            dataParameters.tailAddress = flagBuffer[FlagBuffer::TAIL_ADDRESS];
            dataParameters.bytesInTailBlock = flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK];

            return true;
        }
};