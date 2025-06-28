#include "UIRenderer.hpp"

#include <memory>
#include "UIRenderInterface.hpp"
#include "UISystemInterface.hpp"
#include <RmlUi/Core.h>

#include "renderer/GLFW.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    UIRenderer::UIRenderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void UIRenderer::init(bool initGUI) {
        Timer& timer = m_systemsRegistry.getSystem<Timer>();
        m_renderInterface = std::make_unique<UIRenderInterface>();
        m_systemInterface = std::make_unique<UISystemInterface>(&timer);

        Rml::SetRenderInterface(m_renderInterface.get());
        Rml::SetSystemInterface(m_systemInterface.get());

        // 2. Initialize RmlUi itself
        if (initGUI) {
            Rml::Initialise();


            // Initialize our custom render interface (which creates shaders, etc.)
            m_renderInterface->Initialize();

            // 3. Create a context
            // The context is like a viewport or a "screen" for the UI.
            Rml::Vector2i windowDimensions(1280, 720);
            m_context = Rml::CreateContext("main", windowDimensions);

            if (!m_context) {
                // Handle error
                Rml::Shutdown();
                return;
            }

            // 4. Load a font (REQUIRED)
            // RmlUi cannot render anything, not even a colored div, without at least one font loaded.
            // Make sure you have a .ttf file in an 'assets' folder.
            if (!Rml::LoadFontFace("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\fonts\\Lato-Regular.ttf")) {
                // Handle font loading error
            }

            // 5. --- THE TEST CASE: Load a document from a string ---
            const Rml::String rml_string = R"(
    <rml>
        <head>
            <title>Sample RML</title>
            <style>
                body {
                    /* Use the font family name that RmlUi reported: 'Lato' */
                    font-family: "Lato";
                    background-color: #222222; /* Use full hex for clarity */
                    color: #ffffff;
                    margin: 0;
                    padding: 20px;
                }

                h1 {
                    color: #00aaff;
                }

                p {
                    color: #dddddd;
                }

                button {
                    background-color: #0077cc;
                    color: white;
                    padding: 10px 20px;
                    /* FIX: Replaced 'border: none;' with a compatible property */
                    border-width: 0;
                    cursor: pointer;
                }

                button:hover {
                    background-color: #0055aa;
                }
            </style>
        </head>
        <body>
            <h1>Hello RmlUi!</h1>
            <p>This is a sample RML document.</p>
            <button>Click Me</button>
        </body>
    </rml>
)";

            Rml::ElementDocument* document = m_context->LoadDocument("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\sample.rml");
            //Rml::ElementDocument* document = m_context->LoadDocumentFromMemory(rml_string);
            if (document) {
                document->Show();
            }
            m_initialized = true;
        }
    }

    void UIRenderer::onUpdate() {
        if (m_context) {
            // Update the RmlUi context
            m_context->Update();
            m_context->Render();
        }
    }

    UIRenderer::~UIRenderer() {
    }


    void UIRenderer::shutdown() {
        if (m_initialized) {
            Rml::Shutdown();
        }
    }

    // Input handling methods
    void UIRenderer::processMouseButtonDown(int button, int mods) {
        if (m_context) {
            m_context->ProcessMouseButtonDown(button, mods); // GLFW mods can be passed directly
        }
    }

    void UIRenderer::processMouseButtonUp(int button, int mods) {
        if (m_context) {
            m_context->ProcessMouseButtonUp(button, mods);
        }
    }

    void UIRenderer::processMouseMove(double xpos, double ypos) {
        if (m_context) {
            // GLFW mods are not typically passed with mouse move, use current state if necessary
            // For simplicity, passing 0 or Rml::Input::GetKeyModifierState() if available globally
            m_context->ProcessMouseMove(static_cast<int>(xpos), static_cast<int>(ypos), 0);
        }
    }

    void UIRenderer::processMouseScroll(double yoffset) {
        if (m_context) {
            m_context->ProcessMouseWheel(-yoffset, 0);
        }
    }

    void UIRenderer::processKeyChar(unsigned int codepoint) {
        if (m_context) {
            m_context->ProcessTextInput(Rml::Character(codepoint));
        }
    }

    // Helper to translate GLFW key codes to RmlUi key codes (Based on RmlUi_Backend_GLFW)
    Rml::Input::KeyIdentifier GlfwToRmlKey(int glfw_key) {
        switch (glfw_key) {
            case GLFW_KEY_UNKNOWN: return Rml::Input::KI_UNKNOWN;
            case GLFW_KEY_SPACE: return Rml::Input::KI_SPACE;
            case GLFW_KEY_APOSTROPHE: return Rml::Input::KI_OEM_7;
            case GLFW_KEY_COMMA: return Rml::Input::KI_OEM_COMMA;
            case GLFW_KEY_MINUS: return Rml::Input::KI_OEM_MINUS;
            case GLFW_KEY_PERIOD: return Rml::Input::KI_OEM_PERIOD;
            case GLFW_KEY_SLASH: return Rml::Input::KI_OEM_2;
            case GLFW_KEY_0: return Rml::Input::KI_0;
            case GLFW_KEY_1: return Rml::Input::KI_1;
            case GLFW_KEY_2: return Rml::Input::KI_2;
            case GLFW_KEY_3: return Rml::Input::KI_3;
            case GLFW_KEY_4: return Rml::Input::KI_4;
            case GLFW_KEY_5: return Rml::Input::KI_5;
            case GLFW_KEY_6: return Rml::Input::KI_6;
            case GLFW_KEY_7: return Rml::Input::KI_7;
            case GLFW_KEY_8: return Rml::Input::KI_8;
            case GLFW_KEY_9: return Rml::Input::KI_9;
            case GLFW_KEY_SEMICOLON: return Rml::Input::KI_OEM_1;
            case GLFW_KEY_EQUAL: return Rml::Input::KI_OEM_PLUS;
            case GLFW_KEY_A: return Rml::Input::KI_A;
            case GLFW_KEY_B: return Rml::Input::KI_B;
            case GLFW_KEY_C: return Rml::Input::KI_C;
            case GLFW_KEY_D: return Rml::Input::KI_D;
            case GLFW_KEY_E: return Rml::Input::KI_E;
            case GLFW_KEY_F: return Rml::Input::KI_F;
            case GLFW_KEY_G: return Rml::Input::KI_G;
            case GLFW_KEY_H: return Rml::Input::KI_H;
            case GLFW_KEY_I: return Rml::Input::KI_I;
            case GLFW_KEY_J: return Rml::Input::KI_J;
            case GLFW_KEY_K: return Rml::Input::KI_K;
            case GLFW_KEY_L: return Rml::Input::KI_L;
            case GLFW_KEY_M: return Rml::Input::KI_M;
            case GLFW_KEY_N: return Rml::Input::KI_N;
            case GLFW_KEY_O: return Rml::Input::KI_O;
            case GLFW_KEY_P: return Rml::Input::KI_P;
            case GLFW_KEY_Q: return Rml::Input::KI_Q;
            case GLFW_KEY_R: return Rml::Input::KI_R;
            case GLFW_KEY_S: return Rml::Input::KI_S;
            case GLFW_KEY_T: return Rml::Input::KI_T;
            case GLFW_KEY_U: return Rml::Input::KI_U;
            case GLFW_KEY_V: return Rml::Input::KI_V;
            case GLFW_KEY_W: return Rml::Input::KI_W;
            case GLFW_KEY_X: return Rml::Input::KI_X;
            case GLFW_KEY_Y: return Rml::Input::KI_Y;
            case GLFW_KEY_Z: return Rml::Input::KI_Z;
            case GLFW_KEY_LEFT_BRACKET: return Rml::Input::KI_OEM_4;
            case GLFW_KEY_BACKSLASH: return Rml::Input::KI_OEM_5;
            case GLFW_KEY_RIGHT_BRACKET: return Rml::Input::KI_OEM_6;
            case GLFW_KEY_GRAVE_ACCENT: return Rml::Input::KI_OEM_3;
            case GLFW_KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
            case GLFW_KEY_ENTER: return Rml::Input::KI_RETURN;
            case GLFW_KEY_TAB: return Rml::Input::KI_TAB;
            case GLFW_KEY_BACKSPACE: return Rml::Input::KI_BACK;
            case GLFW_KEY_INSERT: return Rml::Input::KI_INSERT;
            case GLFW_KEY_DELETE: return Rml::Input::KI_DELETE;
            case GLFW_KEY_RIGHT: return Rml::Input::KI_RIGHT;
            case GLFW_KEY_LEFT: return Rml::Input::KI_LEFT;
            case GLFW_KEY_DOWN: return Rml::Input::KI_DOWN;
            case GLFW_KEY_UP: return Rml::Input::KI_UP;
            case GLFW_KEY_PAGE_UP: return Rml::Input::KI_PRIOR;
            case GLFW_KEY_PAGE_DOWN: return Rml::Input::KI_NEXT;
            case GLFW_KEY_HOME: return Rml::Input::KI_HOME;
            case GLFW_KEY_END: return Rml::Input::KI_END;
            case GLFW_KEY_CAPS_LOCK: return Rml::Input::KI_CAPITAL;
            case GLFW_KEY_SCROLL_LOCK: return Rml::Input::KI_SCROLL;
            case GLFW_KEY_NUM_LOCK: return Rml::Input::KI_NUMLOCK;
            case GLFW_KEY_PRINT_SCREEN: return Rml::Input::KI_SNAPSHOT;
            case GLFW_KEY_PAUSE: return Rml::Input::KI_PAUSE;
            case GLFW_KEY_F1: return Rml::Input::KI_F1;
            case GLFW_KEY_F2: return Rml::Input::KI_F2;
            case GLFW_KEY_F3: return Rml::Input::KI_F3;
            case GLFW_KEY_F4: return Rml::Input::KI_F4;
            case GLFW_KEY_F5: return Rml::Input::KI_F5;
            case GLFW_KEY_F6: return Rml::Input::KI_F6;
            case GLFW_KEY_F7: return Rml::Input::KI_F7;
            case GLFW_KEY_F8: return Rml::Input::KI_F8;
            case GLFW_KEY_F9: return Rml::Input::KI_F9;
            case GLFW_KEY_F10: return Rml::Input::KI_F10;
            case GLFW_KEY_F11: return Rml::Input::KI_F11;
            case GLFW_KEY_F12: return Rml::Input::KI_F12;
            case GLFW_KEY_KP_0: return Rml::Input::KI_NUMPAD0;
            case GLFW_KEY_KP_1: return Rml::Input::KI_NUMPAD1;
            case GLFW_KEY_KP_2: return Rml::Input::KI_NUMPAD2;
            case GLFW_KEY_KP_3: return Rml::Input::KI_NUMPAD3;
            case GLFW_KEY_KP_4: return Rml::Input::KI_NUMPAD4;
            case GLFW_KEY_KP_5: return Rml::Input::KI_NUMPAD5;
            case GLFW_KEY_KP_6: return Rml::Input::KI_NUMPAD6;
            case GLFW_KEY_KP_7: return Rml::Input::KI_NUMPAD7;
            case GLFW_KEY_KP_8: return Rml::Input::KI_NUMPAD8;
            case GLFW_KEY_KP_9: return Rml::Input::KI_NUMPAD9;
            case GLFW_KEY_KP_DECIMAL: return Rml::Input::KI_DECIMAL;
            case GLFW_KEY_KP_DIVIDE: return Rml::Input::KI_DIVIDE;
            case GLFW_KEY_KP_MULTIPLY: return Rml::Input::KI_MULTIPLY;
            case GLFW_KEY_KP_SUBTRACT: return Rml::Input::KI_SUBTRACT;
            case GLFW_KEY_KP_ADD: return Rml::Input::KI_ADD;
            case GLFW_KEY_KP_ENTER: return Rml::Input::KI_NUMPADENTER;
            case GLFW_KEY_KP_EQUAL: return Rml::Input::KI_OEM_NEC_EQUAL;
            case GLFW_KEY_LEFT_SHIFT: return Rml::Input::KI_LSHIFT;
            case GLFW_KEY_LEFT_CONTROL: return Rml::Input::KI_LCONTROL;
            case GLFW_KEY_LEFT_ALT: return Rml::Input::KI_LMENU;
            case GLFW_KEY_LEFT_SUPER: return Rml::Input::KI_LWIN;
            case GLFW_KEY_RIGHT_SHIFT: return Rml::Input::KI_RSHIFT;
            case GLFW_KEY_RIGHT_CONTROL: return Rml::Input::KI_RCONTROL;
            case GLFW_KEY_RIGHT_ALT: return Rml::Input::KI_RMENU;
            case GLFW_KEY_RIGHT_SUPER: return Rml::Input::KI_RWIN;
            case GLFW_KEY_MENU: return Rml::Input::KI_APPS;
            default: break;
        }
        return Rml::Input::KI_UNKNOWN;
    }

    Rml::Context* UIRenderer::getContext() {
        return m_context;
    }
}
