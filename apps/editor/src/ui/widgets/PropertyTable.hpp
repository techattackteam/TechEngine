#pragma once

namespace TechEngine {
    // Two columns: labels at labelWidth, values stretched. Tables that share a labelWidth line their
    // labels up, so every component body passes the same one.
    bool beginPropertyTable(const char* id, float labelWidth);

    // Draws label in the first column and leaves the cursor in the second.
    void propertyRow(const char* label);

    void endPropertyTable();
}
