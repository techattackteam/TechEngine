#include "StreamWriter.hpp"

namespace TechEngine {
    void StreamWriter::writeBuffer(Buffer buffer, bool writeSize) {
        if (writeSize)
            writeData((char*)&buffer.size, sizeof(uint32_t));

        writeData((char*)buffer.data, buffer.size);
    }

    void StreamWriter::writeZero(uint64_t size) {
        char zero = 0;
        for (uint64_t i = 0; i < size; i++)
            writeData(&zero, 1);
    }

    void StreamWriter::writeString(const std::string& string) {
        size_t size = string.size();
        writeData((char*)&size, sizeof(size_t));
        writeData((char*)string.data(), sizeof(char) * string.size());
    }

    void StreamWriter::writeString(std::string_view string) {
        size_t size = string.size();
        writeData((char*)&size, sizeof(size_t));
        writeData((char*)string.data(), sizeof(char) * string.size());
    }
}