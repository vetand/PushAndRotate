#include <string>

#include "algorithm.h"

int main(int argc, char** argv) {
    std::string input_name(argv[1]);
    std::string output_name(argv[2]);
    PushAndRotate algo(input_name, output_name);
    return 0;
}