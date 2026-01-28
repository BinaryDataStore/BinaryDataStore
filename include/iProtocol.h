#pragma once

#include <cstdbool>
#include <cstddef>

/**
 * @def Interface to a protocol
 */
class iProtocol
{
    public:
        /**
         * @def indicates when the protocol is ready to receive data
         *
         * @return true on success, false otherwise
         */
        virtual bool receiverReady(void) = 0;

        /**
         * @def indicates when the protocol is ready to transmit data
         *
         * @return true on success, false otherwise
         */
        virtual bool transmitterReady(void) = 0;

        /**
         * @def gets errors associated with the protocol
         *
         * @param error pointer to where the error codes will be held
         */
        virtual void getError(void* error) = 0;

        /**
         * @def recieve data over the protocol
         *
         * @param message pointer to were the data will be held
         * @param messageSize size of the message pointer
         *
         * @return true on success, false otherwise
         */
        virtual bool recieve(void* message, size_t messageSize) = 0;

        /**
         * @def transmit data over the protocol
         *
         * @param message pointer to the data to be transmitted
         * @param messageSize size of the message pointer
         *
         * @return true on success, false otherwise
         */
        virtual bool transmit(void* message, size_t messageSize) = 0;
};