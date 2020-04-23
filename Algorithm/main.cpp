#include <string>

#include "algorithm.h"
#include "priorities.cpp"

int main(int argc, char** argv) {
    std::string input_name(argv[1]);
    std::string output_name(argv[2]);
    bool parallel_mode = (argc >= 4 && strcmp(argv[3], "parallel") == 0);
    bool priority_mode = (argc >= 5 && strcmp(argv[4], "priorities") == 0);
    PushAndRotate algo(input_name, output_name, parallel_mode);
    if (priority_mode) {
        std::string new_output_name;
        for (int ind = 0; ind < (int)output_name.size() - 4; ++ind) {
            new_output_name += output_name[ind];
        }
        new_output_name += "_pr.xml";
        std::cout << std::endl << std::endl;
        Priorities another_algo(input_name, new_output_name, algo.logger.moves.size());
    }
    if (algo.is_solution) {
        return 0;
    } else {
        return 1;
    }
}