#include "algorithm.h"

void PushAndRotate::nodes_list_init() {
    for (int id = 0; id < this->map.get_width() * this->map.get_height(); ++id) {
        this->nodes_list.push_back(Node(id, id % this->map.get_width(), 
                                            id / this->map.get_width(), -1));
    }
}

bool PushAndRotate::check_answer() {
    int turn = 0;
    std::vector<Movement> move_copy = this->logger.moves;
    while (!move_copy.empty()) {
        this->mover.undo(this->map, move_copy);
    }
    for (const auto& movement: this->logger.moves) {
        ++turn;
        int x = movement.current_id % this->map.get_width();
        int y = movement.current_id / this->map.get_width();
        if (!this->map.within_map(x, y)) {
            std::cout << "Turn " << turn << ", agent " << movement.agent_number + 1
                                             << " goes out of a map!" << std::endl;
            return false;
        }
        if (this->map.is_obstacle(x, y)) {
            std::cout << "Turn " << turn << ", agent " << movement.agent_number + 1 
                                                 << " hits obstacle!" << std::endl;
            return false;
        }
        if (this->map.is_start(x, y)) {
            std::cout << "Turn " << turn << ", agent " << movement.agent_number + 1
                                            << " hits another agent!" << std::endl;
            return false;
        }
        int old_x = movement.previous_id % this->map.get_width();
        int old_y = movement.previous_id / this->map.get_width();
        if (std::abs(x - old_x) > 1 || std::abs(y - old_y) > 1) {
            std::cout << "Turn " << turn << ", too long jump from agent "
                               << movement.agent_number + 1 << std::endl;
            return false;
        }
        this->map.replace_agent(movement.previous_id, movement.current_id);
    }
    for (int ind = 0; ind < map.number_of_agents; ++ind) {
        if (map.agents[ind].start_x != map.agents[ind].finish_x || 
            map.agents[ind].start_y != map.agents[ind].finish_y) {
            std::cout << "Agent " << ind + 1 << " did not reach it`s goal position!" << std::endl;
            return false;
        }
    }
    return true;
}

PushAndRotate::PushAndRotate(const std::string& file_name_input, 
                             const std::string& file_name_output) : 
                             logger(Logger(file_name_output.c_str())) {
    std::srand(1);
    this->map.get_map(file_name_input.c_str());
    if (this->map.number_of_agents == 0) {
        this->is_solution = false;
        return;
    }
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
    PostProcess(this->map, this->logger.moves);
    bool correct = this->check_answer();
    logger.print_log_second(this->map);
    if (correct) {
        std::cout << "Result length = " << this->logger.moves.size() << std::endl;
        std::cout << "Algorithm complited!" << std::endl;
        this->is_solution = true;
    } else {
        std::cout << "Algorithm got incorrect result!" << std::endl;
        this->is_solution = false;
    }
}