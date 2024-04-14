#include "StreamReader.hpp"

namespace TechEngine {
    bool StreamReader::readBuffer(Buffer& buffer, uint32_t size) {
        buffer.size = size;
        if (size == 0) {
            if (!readData((char*)&buffer.size, sizeof(uint32_t)))
                return false;
        }

        buffer.allocate(buffer.size);
        return readData((char*)buffer.data, buffer.size);
    }

    bool StreamReader::readString(std::string& string) {
        size_t size;
        if (!readData((char*)&size, sizeof(size_t)))
            return false;

        string.resize(size);
        return readData((char*)string.data(), sizeof(char) * size);
    }
} // namespace Hazel
