#include <fstream>

int main() {
    std::ofstream marker{"ProcessStubRan.txt"};
    marker << "ran";
    return 42;
}
