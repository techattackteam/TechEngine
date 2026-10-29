#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <string>

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
        Entity m_entity;

    public:
        DestroyEntityCommand(EditSession& session, Entity entity);

        void apply(Scene& scene) override;
    };

    class ReparentEntityCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        Entity m_child;
        Entity m_parent;
        std::size_t m_position = 0;

    public:
        // A parent that is not valid makes the child a root, and the position is then ignored.
        ReparentEntityCommand(EditSession& session, Entity child, Entity parent, std::size_t position);

        void apply(Scene& scene) override;
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
        Entity m_entity;

    public:
        // An entity that is not valid clears the selection.
        SelectEntityCommand(EditSession& session, Entity entity);

        void apply(Scene& scene) override;
    };
}
