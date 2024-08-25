#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace TechEngine {
    struct LineVertex {
        glm::vec3 position;
        glm::vec4 color;

        inline LineVertex(glm::vec3 position, glm::vec4 color) : position(position), color(color) {
        };
    };

    class Line {
    private:
        LineVertex start;
        LineVertex end;

    public:
        Line(glm::vec3 start, glm::vec3 end, glm::vec4 color) : start(start, color), end(end, color) {
        }

        LineVertex getStart() {
            return start;
        }

        LineVertex getEnd() {
            return end;
        }

        glm::vec4 getColor() {
            return start.color;
        }
    };
}
