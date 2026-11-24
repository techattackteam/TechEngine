#pragma once

#include <TechEngine/app/Command.hpp>

#include <cstdint>

namespace TechEngine {
    class App;
    class EditSession;
    class FileAccess;

    enum class AfterSave : std::uint8_t {
        KeepOpen,
        Close,
    };

    class SaveSceneCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        FileAccess* m_files = nullptr;
        App* m_app = nullptr;
        AfterSave m_after = AfterSave::KeepOpen;

    public:
        // Close stops the app only after a successful write, from the simulation thread, so the
        // close never cancels the save it waits for.
        SaveSceneCommand(EditSession& session, FileAccess& files, App& app, AfterSave after);

        void apply(Scene& scene) override;
    };
}
