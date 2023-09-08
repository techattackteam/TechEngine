#include <iostream>
#include "Test.hpp"
#include "C:/dev/TechEngine/bin/TechEngineEditor/debug/TechEngineAPI/include/TechEngineGame/script/ScriptEngine.hpp"

void Test::onFixedUpdate() {
    std::cout << "Hello" << std::endl;
}

void Test::onUpdate() {

}

RegisterScript(Test);