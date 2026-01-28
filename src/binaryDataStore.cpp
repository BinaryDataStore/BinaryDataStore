#include "binaryDataStore.h"

template <uint32_t PAGE_SIZE_BYTES>
void BinaryDataStore<PAGE_SIZE_BYTES>::resetFlagBuffer(uint32_t tempFlags[FlagBuffer::SIZE])
{
    flagBuffer[FlagBuffer::INIT_FLAG] = tempFlags[FlagBuffer::INIT_FLAG];
    flagBuffer[FlagBuffer::TAIL_ADDRESS] = tempFlags[FlagBuffer::TAIL_ADDRESS];
    flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK] = tempFlags[FlagBuffer::BYTES_IN_TAIL_BLOCK];
    flagBuffer[FlagBuffer::TAIL_WRAPPED_AROUND] = tempFlags[FlagBuffer::TAIL_WRAPPED_AROUND];
}

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::updatePageBuffer(uint8_t data[], uint32_t dataSize, uint32_t dataOffset, uint32_t dataSliceSize, uint32_t pageBufferOffset)
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

template <uint32_t PAGE_SIZE_BYTES>
template <typename T>
bool BinaryDataStore<PAGE_SIZE_BYTES>::eraseWriteReadConfirm(T writeBuffer[], uint32_t bufferSize, uint32_t address)
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

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::appendGeneric(uint8_t data[], uint32_t dataSize)
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

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::getHeadAddress(uint32_t &headAddress)
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

template <uint32_t PAGE_SIZE_BYTES>
BinaryDataStore<PAGE_SIZE_BYTES>::BinaryDataStore(iMemory &memory, MemoryConfig config) :
    memory(memory),
    FLAG_ADDRESS(config.startAddress),
    DATA_LOWER_ADDRESS(config.startAddress + PAGE_SIZE_BYTES),
    DATA_UPPER_ADDRESS((config.startAddress + config.bytesAllocated) - (config.bytesAllocated % PAGE_SIZE_BYTES))
{}

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::formatMemory(void)
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

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::append(std::string &text)
{
    if (!memoryFormatted)
    {
        return false;
    }

    uint8_t data[text.size()];
    memcpy(data, text.data(), text.size());

    return appendGeneric(data, sizeof(data));
}

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::append(uint8_t data[], uint32_t dataSize)
{
    if (!memoryFormatted || data == nullptr)
    {
        return false;
    }

    return appendGeneric(data, dataSize);
}

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::readPage(uint32_t pageAddess, uint32_t &dataSizeInPage, uint8_t *data, uint32_t dataSize)
{
    if (dataSize < PAGE_SIZE_BYTES || data == nullptr)
    {
        return false;
    }

    dataSizeInPage = dataLengthInPage(pageAddess);

    return memory.read( data, dataSizeInPage, pageAddess );
}

template <uint32_t PAGE_SIZE_BYTES>
uint32_t BinaryDataStore<PAGE_SIZE_BYTES>::dataSize(void)
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


template <uint32_t PAGE_SIZE_BYTES>
uint32_t BinaryDataStore<PAGE_SIZE_BYTES>::dataLengthInPage(const uint32_t pageAddress)
{
    if (pageAddress == flagBuffer[FlagBuffer::TAIL_ADDRESS])
    {
        return flagBuffer[FlagBuffer::BYTES_IN_TAIL_BLOCK];
    }

    return PAGE_SIZE_BYTES;
}

template <uint32_t PAGE_SIZE_BYTES>
uint32_t BinaryDataStore<PAGE_SIZE_BYTES>::getNextPageAddress(uint32_t pageAddress)
{
    uint32_t nextPageAddress = pageAddress + PAGE_SIZE_BYTES;
    if (nextPageAddress >= DATA_UPPER_ADDRESS)
    {
        return DATA_LOWER_ADDRESS;
    }

    return nextPageAddress;
}

template <uint32_t PAGE_SIZE_BYTES>
bool BinaryDataStore<PAGE_SIZE_BYTES>::getDataParameters(DataParameters &dataParameters)
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