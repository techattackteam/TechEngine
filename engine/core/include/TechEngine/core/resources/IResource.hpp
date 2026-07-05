#pragma once

#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"
#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/core/UUID.hpp"

namespace TechEngine {
    class CORE_DLL IResource {
    protected:
        UUID m_uuid;
        std::string m_name;

    public:
        IResource() : m_uuid(), m_name("Unitialized Resource") {
        }

        IResource(const std::string& name) : m_uuid(UUID::generate()), m_name(name) {
        }

        virtual ~IResource() = default;

        const UUID& getUUID() const {
            return m_uuid;
        }

        const std::string& getName() const {
            return m_name;
        }

        static void serialize(StreamWriter* writer, const IResource& resource) {
            writer->writeRaw<UUID>(resource.m_uuid);
            writer->writeString(resource.m_name);
        }

        static void deserialize(StreamReader* reader, IResource& resource) {
            UUID uuid;
            reader->readRaw<UUID>(uuid);
            resource.m_uuid = UUID(static_cast<uint64_t>(uuid)); // This is need to register the UUID in the set of registered UUIDs, otherwise it might be generated again for another resource
            reader->readString(resource.m_name);
        }
    };
}
