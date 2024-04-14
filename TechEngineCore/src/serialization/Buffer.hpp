#pragma once

#include <memory>
#include <string.h>

namespace TechEngine {
    class Buffer {
    public:
        void* data;
        uint64_t size;

        Buffer()
            : data(nullptr), size(0) {
        }

        Buffer(const void* data, uint64_t size)
            : data((void*)data), size(size) {
        }

        Buffer(const Buffer& other, uint64_t size)
            : data(other.data), size(size) {
        }

        static Buffer copy(const Buffer& other) {
            Buffer buffer;
            buffer.allocate(other.size);
            memcpy(buffer.data, other.data, other.size);
            return buffer;
        }

        static Buffer copy(const void* data, uint64_t size) {
            Buffer buffer;
            buffer.allocate(size);
            memcpy(buffer.data, data, size);
            return buffer;
        }

        void allocate(uint64_t size) {
            delete[](uint8_t*)data;
            data = nullptr;

            if (size == 0)
                return;

            data = new uint8_t[size];
            this->size = size;
        }

        void release() {
            delete[](uint8_t*)data;
            data = nullptr;
            size = 0;
        }

        void resize(uint64_t newSize) {
            void* newData = new uint8_t[newSize];
            if (data) {
                memcpy(newData, data, size < newSize ? size : newSize);
                delete[] static_cast<uint8_t*>(data);
            }
            data = newData;
            size = newSize;
        }

        void zeroInitialize() {
            if (data)
                memset(data, 0, size);
        }

        template<typename T>
        T& read(uint64_t offset = 0) {
            return *(T*)((uint32_t*)data + offset);
        }

        template<typename T>
        const T& read(uint64_t offset = 0) const {
            return *(T*)((uint32_t*)data + offset);
        }

        uint8_t* readBytes(uint64_t size, uint64_t offset) const {
            //WL_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
            uint8_t* buffer = new uint8_t[size];
            memcpy(buffer, (uint8_t*)data + offset, size);
            return buffer;
        }

        void write(const void* data, uint64_t size, uint64_t offset = 0) {
            memcpy((uint8_t*)data + offset, data, size);
        }

        operator bool() const {
            return data;
        }

        uint8_t& operator[](int index) {
            return ((uint8_t*)data)[index];
        }

        uint8_t operator[](int index) const {
            return ((uint8_t*)data)[index];
        }

        template<typename T>
        T* As() const {
            return (T*)data;
        }

        inline uint64_t GetSize() const {
            return size;
        }
    };
}
