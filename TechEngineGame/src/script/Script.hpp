#pragma once

namespace TechEngine {
    class Script {
    public:
        Script();

        virtual ~Script();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onClose();
    };
}

