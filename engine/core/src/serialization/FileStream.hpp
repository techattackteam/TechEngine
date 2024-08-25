#pragma once

#include "StreamWriter.hpp"
#include "StreamReader.hpp"

#include <filesystem>
#include <fstream>

namespace TechEngine {
    //==============================================================================
    /// FileStreamWriter
    class FileStreamWriter : public StreamWriter {
    public:
        explicit FileStreamWriter(const std::filesystem::path& path);

        FileStreamWriter(const FileStreamWriter&) = delete;

        virtual ~FileStreamWriter();

        bool isStreamGood() const override {
            return m_Stream.good();
        }

        uint64_t getStreamPosition() override {
            return m_Stream.tellp();
        }

        void setStreamPosition(uint64_t position) override {
            m_Stream.seekp(position);
        }

        bool writeData(const char* data, size_t size) override;

    private:
        std::filesystem::path m_Path;
        std::ofstream m_Stream;
    };

    //==============================================================================
    /// FileStreamReader
    class FileStreamReader : public StreamReader {
    public:
        explicit FileStreamReader(const std::filesystem::path& path);

        FileStreamReader(const FileStreamReader&) = delete;

        ~FileStreamReader() override;

        bool isStreamGood() const override {
            return m_Stream.good();
        }

        uint64_t getStreamPosition() override {
            return m_Stream.tellg();
        }

        void setStreamPosition(uint64_t position) override {
            m_Stream.seekg(position);
        }

        bool readData(char* destination, size_t size) override;

    private:
        std::filesystem::path m_Path;
        std::ifstream m_Stream;
    };
}
