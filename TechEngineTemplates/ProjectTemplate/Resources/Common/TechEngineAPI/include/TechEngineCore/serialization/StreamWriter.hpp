#pragma once

#include <cassert>

#include "buffer.hpp"

#include <string>
#include <map>
#include <unordered_map>
#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class CORE_DLL StreamWriter {
    public:
        virtual ~StreamWriter() = default;

        virtual bool isStreamGood() const = 0;

        virtual uint64_t getStreamPosition() = 0;

        virtual void setStreamPosition(uint64_t position) = 0;

        virtual bool writeData(const char* data, size_t size) = 0;

        operator bool() const {
            return isStreamGood();
        }

        void writeBuffer(Buffer buffer, bool writeSize = true);

        void writeZero(uint64_t size);

        void writeString(const std::string& string);

        void writeString(std::string_view string);

        template<typename T>
        void writeRaw(const T& type) {
            bool success = writeData((char*)&type, sizeof(T));
            assert(success);
        }

        template<typename T>
        void writeObject(const T& obj) {
            if constexpr (std::is_pointer<T>::value) {
                using PointedType = typename std::remove_pointer<T>::type;
                PointedType::Serialize(this, *obj);
            } else {
                T::Serialize(this, obj);
            }
        }

        template<typename Key, typename Value>
        void writeMap(const std::map<Key, Value>& map, bool writeSize = true) {
            if (writeSize)
                writeRaw<uint32_t>((uint32_t)map.size());

            for (const auto& [key, value]: map) {
                if constexpr (std::is_pointer<Key>::value) {
                    using PointedType = typename std::remove_pointer<Key>::type;
                    if constexpr (std::is_trivial<PointedType>::value)
                        writeRaw<Key>(value);
                    else
                        writeObject<Key>(value);
                } else {
                    if constexpr (std::is_trivial<Key>::value)
                        writeRaw<Key>(value);
                    else
                        writeObject<Key>(value);
                }

                if constexpr (std::is_pointer<Value>::value) {
                    using PointedType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointedType>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                } else {
                    if constexpr (std::is_trivial<Value>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                }
            }
        }

        template<typename Key, typename Value>
        void writeMap(const std::unordered_map<Key, Value>& map, bool writeSize = true) {
            if (writeSize)
                writeRaw<uint32_t>((uint32_t)map.size());

            for (const auto& [key, value]: map) {
                if constexpr (std::is_pointer<Key>::value) {
                    using PointedType = typename std::remove_pointer<Key>::type;
                    if constexpr (std::is_trivial<PointedType>::value)
                        writeRaw<Key>(value);
                    else
                        writeObject<Key>(value);
                } else {
                    if constexpr (std::is_trivial<Key>::value)
                        writeRaw<Key>(value);
                    else
                        writeObject<Key>(value);
                }

                if constexpr (std::is_pointer<Value>::value) {
                    using PointedType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointedType>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                } else {
                    if constexpr (std::is_trivial<Value>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                }
            }
        }

        template<typename Value>
        void writeMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true) {
            if (writeSize)
                writeRaw<uint32_t>((uint32_t)map.size());

            for (const auto& [key, value]: map) {
                writeString(key);

                if constexpr (std::is_pointer<Value>::value) {
                    using PointedType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointedType>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                } else {
                    if constexpr (std::is_trivial<Value>::value)
                        writeRaw<Value>(value);
                    else
                        writeObject<Value>(value);
                }
            }
        }

        template<typename T>
        void writeArray(const std::vector<T>& array, bool writeSize = true) {
            if (writeSize)
                writeRaw<uint32_t>((uint32_t)array.size());

            for (const auto& element: array) {
                if constexpr (std::is_trivial<T>())
                    WriteRaw<T>(element);
                else
                    WriteObject<T>(element);
            }
        }

        template<>
        void writeArray(const std::vector<std::string>& array, bool writeSize) {
            if (writeSize)
                writeRaw<uint32_t>((uint32_t)array.size());

            for (const auto& element: array)
                writeString(element);
        }
    };
}
