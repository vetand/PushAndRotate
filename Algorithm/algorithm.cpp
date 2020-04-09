#include "algorithm.h"

void PushAndRotate::nodes_list_init() {
    for (int id = 0; id < this->map.get_width() * this->map.get_height(); ++id) {
        this->nodes_list.push_back(Node(id, id % this->map.get_width(), 
                                            id / this->map.get_width(), -1));
    }
}

PushAndRotate::PushAndRotate(const std::string& file_name_input, 
                             const std::string& file_name_output) : 
                             logger(Logger(file_name_output.c_str())) {
    this->map.get_map(file_name_input.c_str());
    this->logger.print_log_first(this->map);
    this->nodes_list_init();
    BiconnectedPhase(this->map, this->nodes_list);
    JointPhase(this->map, this->nodes_list);
    MergePhase(this->map, this->nodes_list, this);
    AssigningPhase(this->map, this->nodes_list, this);
    if (!this->is_solution) {
        return;
    }
    SubgraphsSortPhase(this->map, this->nodes_list, this);
    MovingPhase(this->map, this->nodes_list, this);
    logger.print_log_second(this->map);
}