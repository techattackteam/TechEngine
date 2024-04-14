#pragma once

#include "buffer.hpp"

#include <string>
#include <map>
#include <unordered_map>

namespace TechEngine {
    class StreamReader {
    public:
        virtual ~StreamReader() = default;

        virtual bool isStreamGood() const = 0;

        virtual uint64_t getStreamPosition() = 0;

        virtual void setStreamPosition(uint64_t position) = 0;

        virtual bool readData(char* destination, size_t size) = 0;

        operator bool() const {
            return isStreamGood();
        }

        bool readBuffer(Buffer& buffer, uint32_t size = 0);

        bool readString(std::string& string);

        template<typename T>
        bool readRaw(T& type) {
            bool success = readData((char*)&type, sizeof(T));
            return success;
        }

        template<typename T>
        void readObject(T& obj) {
            if constexpr (std::is_pointer<T>::value) {
                using PointedType = typename std::remove_pointer<T>::type;
                PointedType::Deserialize(this, *obj);
            } else {
                T::Deserialize(this, obj);
            }
        }

        template<typename Key, typename Value>
        void readMap(std::map<Key, Value>& map, uint32_t size = 0) {
            if (size == 0)
                readRaw<uint32_t>(size);

            for (uint32_t i = 0; i < size; i++) {
                Key key;
                if (std::is_pointer<Key>::value) {
                    using PointedType = typename std::remove_pointer<Key>::type;
                    if constexpr (std::is_trivial<PointedType>())
                        readRaw<Key>(key);
                    else
                        readObject<Key>(key);
                } else {
                    if constexpr (std::is_trivial<Key>())
                        readRaw<Key>(key);
                    else
                        readObject<Key>(key);
                }

                if constexpr (std::is_pointer<Value>::value) {
                    using PointerType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointerType>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                } else {
                    if constexpr (std::is_trivial<Value>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                }
            }
        }

        template<typename Key, typename Value>
        void readMap(std::unordered_map<Key, Value>& map, uint32_t size = 0) {
            if (size == 0)
                readRaw<uint32_t>(size);

            for (uint32_t i = 0; i < size; i++) {
                Key key;
                if (std::is_pointer<Key>::value) {
                    using PointedType = typename std::remove_pointer<Key>::type;
                    if constexpr (std::is_trivial<PointedType>())
                        readRaw<Key>(key);
                    else
                        readObject<Key>(key);
                } else {
                    if constexpr (std::is_trivial<Key>())
                        readRaw<Key>(key);
                    else
                        readObject<Key>(key);
                }

                if constexpr (std::is_pointer<Value>::value) {
                    using PointerType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointerType>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                } else {
                    if constexpr (std::is_trivial<Value>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                }
            }
        }

        template<typename Value>
        void readMap(std::unordered_map<std::string, Value>& map, uint32_t size = 0) {
            if (size == 0)
                readRaw<uint32_t>(size);

            for (uint32_t i = 0; i < size; i++) {
                std::string key;
                readString(key);

                if constexpr (std::is_pointer<Value>::value) {
                    using PointerType = typename std::remove_pointer<Value>::type;
                    if constexpr (std::is_trivial<PointerType>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                } else {
                    if constexpr (std::is_trivial<Value>())
                        readRaw<Value>(map[key]);
                    else
                        readObject<Value>(map[key]);
                }
            }
        }

        template<typename T>
        void readArray(std::vector<T>& array, uint32_t size = 0) {
            if (size == 0)
                readRaw<uint32_t>(size);

            array.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                if constexpr (std::is_trivial<T>())
                    readRaw<T>(array[i]);
                else
                    readObject<T>(array[i]);
            }
        }

        template<>
        void readArray(std::vector<std::string>& array, uint32_t size) {
            if (size == 0)
                readRaw<uint32_t>(size);

            array.resize(size);

            for (uint32_t i = 0; i < size; i++)
                readString(array[i]);
        }
    };
}
