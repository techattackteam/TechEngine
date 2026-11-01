#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TechEngine {
    class EditSession;

    class CreateEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        Entity m_parent;

    public:
        CreateEntityCommand(EditSession& session, Entity parent);

        void apply(Scene& scene) override;
    };

    class DestroyEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        std::vector<Entity> m_entities;

    public:
        DestroyEntityCommand(EditSession& session, std::vector<Entity> entities);

        void apply(Scene& scene) override;
    };

    class ReparentEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        std::vector<Entity> m_children;
        Entity m_parent;
        std::size_t m_position = 0;

    public:
        ReparentEntityCommand(EditSession& session, std::vector<Entity> children, Entity parent, std::size_t position);

        void apply(Scene& scene) override;

    private:
        bool isMoved(Entity entity) const;

        bool isInMovedSubtree(const Scene& scene, Entity entity) const;

        Entity getInsertAnchor(const Scene& scene) const;

        std::size_t getPositionAfter(const Scene& scene, Entity child, Entity anchor) const;
    };

    class RenameEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        Entity m_entity;
        std::string m_name;

    public:
        RenameEntityCommand(EditSession& session, Entity entity, std::string name);

        void apply(Scene& scene) override;
    };

    class SelectEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        std::vector<Entity> m_selection;
        Entity m_active;
        std::uint64_t m_sequence = 0;

    public:
        // A dead entity leaves the set, but the sequence is still stored, or main would never adopt a snapshot again.
        SelectEntityCommand(EditSession& session, std::vector<Entity> selection, Entity active, std::uint64_t sequence);

        void apply(Scene& scene) override;
    };
}
