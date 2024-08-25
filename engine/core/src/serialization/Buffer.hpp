#pragma once

#include <memory>
#include <string.h>

#include "core/Logger.hpp"

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
                //Add error handling
                if (memcpy(newData, data, size) == nullptr) {
                    TE_LOGGER_CRITICAL("Failed to copy data to new buffer")
                }
                delete[](uint8_t*)data;
            }
            data = newData;
            size = newSize;
        }

        void zeroInitialize() {
            if (data)
                memset(data, 0, size);
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