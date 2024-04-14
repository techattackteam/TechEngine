#pragma once

#include "StreamReader.hpp"
#include "StreamWriter.hpp"

#include <filesystem>

namespace TechEngine {
    //==============================================================================
    /// BufferStreamWriter
    class BufferStreamWriter : public StreamWriter {
    public:
        BufferStreamWriter(Buffer targetBuffer, uint64_t position = 0);

        BufferStreamWriter(const BufferStreamWriter&) = delete;

        virtual ~BufferStreamWriter() override = default;

        bool isStreamGood() const final {
            return (bool)targetBuffer;
        }

        uint64_t getStreamPosition() override {
            return bufferPosition;
        }

        void setStreamPosition(uint64_t position) override {
            bufferPosition = position;
        }

        bool writeData(const char* data, size_t size) final;

        // Returns Buffer with currently written size
        Buffer getBuffer() const {
            return Buffer(targetBuffer, bufferPosition);
        }

    private:
        Buffer targetBuffer;
        uint64_t bufferPosition = 0;
    };

    //==============================================================================
    /// BufferStreamReader
    class BufferStreamReader : public StreamReader {
    public:
        BufferStreamReader(Buffer targetBuffer, uint64_t position = 0);

        BufferStreamReader(const BufferStreamReader&) = delete;

        virtual ~BufferStreamReader() override = default;

        bool isStreamGood() const final {
            return (bool)targetBuffer;
        }

        uint64_t getStreamPosition() override {
            return bufferPosition;
        }

        void setStreamPosition(uint64_t position) override {
            bufferPosition = position;
        }

        bool readData(char* destination, size_t size) override;

        // Returns Buffer with currently read size
        Buffer getBuffer() const {
            return Buffer(targetBuffer, bufferPosition);
        }

    private:
        Buffer targetBuffer;
        uint64_t bufferPosition = 0;
    };
}
