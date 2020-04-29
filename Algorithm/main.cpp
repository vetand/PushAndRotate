#include <string>

#include "algorithm.h"
#include "priorities.cpp"

int main(int argc, char** argv) {
    std::string input_name(argv[1]);
    std::string output_name(argv[2]);
    bool parallel_mode = (argc >= 4 && strcmp(argv[3], "parallel") == 0);
    bool priority_mode = (argc >= 4 && strcmp(argv[3], "priorities") == 0);
    PushAndRotate algo(input_name, output_name, parallel_mode || priority_mode);
    if (priority_mode) {
        Priorities another_algo(input_name, output_name, algo.logger.moves);
    }
    if (algo.is_solution) {
        return 0;
    } else {
        return 1;
    }
}