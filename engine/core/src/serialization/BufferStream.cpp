#include "BufferStream.hpp"

namespace TechEngine {
    BufferStreamWriter::BufferStreamWriter(Buffer& targetBuffer, uint64_t position)
        : targetBuffer(targetBuffer), bufferPosition(position) {
    }

    bool BufferStreamWriter::writeData(const char* data, size_t size) {
        if (bufferPosition + size > targetBuffer.size) {
            uint64_t newSize = targetBuffer.size;
            do {
                newSize *= 2;
            } while (bufferPosition + size > newSize);

            targetBuffer.resize(newSize);
        }

        memcpy(targetBuffer.As<uint8_t>() + bufferPosition, data, size);
        bufferPosition += size;
        return true;
    }

    BufferStreamReader::BufferStreamReader(Buffer& targetBuffer, uint64_t position)
        : targetBuffer(targetBuffer), bufferPosition(position) {
    }

    bool BufferStreamReader::readData(char* destination, size_t size) {
        bool valid = bufferPosition + size <= targetBuffer.size;
        if (!valid)
            return false;

        memcpy(destination, targetBuffer.As<uint8_t>() + bufferPosition, size);
        bufferPosition += size;
        return true;
    }
}