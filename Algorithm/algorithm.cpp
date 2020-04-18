#include "algorithm.h"

void PushAndRotate::nodes_list_init() {
    for (int id = 0; id < this->map.get_width() * this->map.get_height(); ++id) {
        this->nodes_list.push_back(Node(id, id % this->map.get_width(), 
                                            id / this->map.get_width(), -1));
    }
}

void PushAndRotate::reset_map() {
    std::vector<Movement> move_copy = this->logger.moves;
    while (!move_copy.empty()) {
        this->mover.undo(this->map, move_copy);
    }
}

bool PushAndRotate::check_answer() {
    this->reset_map();
    std::vector<std::set<int>> reserved;
    reserved.resize(this->logger.moves.back().step + 1);
    for (const auto& movement: this->logger.moves) {
        int x = movement.current_id % this->map.get_width();
        int y = movement.current_id / this->map.get_width();
        int step = movement.step;
        if (!this->map.within_map(x, y)) {
            std::cout << "Step " << step << ", agent " << movement.agent_number + 1
                                             << " goes out of a map!" << std::endl;
            return false;
        }
        if (this->map.is_obstacle(x, y)) {
            std::cout << "Step " << step << ", agent " << movement.agent_number + 1 
                                                 << " hits obstacle!" << std::endl;
            return false;
        }
        if (this->map.is_start(x, y)) {
            std::cout << "Step " << step << ", agent " << movement.agent_number + 1
                                            << " hits another agent!" << std::endl;
            return false;
        }
        int old_x = movement.previous_id % this->map.get_width();
        int old_y = movement.previous_id / this->map.get_width();
        if (std::abs(x - old_x) > 1 || std::abs(y - old_y) > 1) {
            std::cout << "Step " << step << ", too long jump from agent "
                               << movement.agent_number + 1 << std::endl;
            return false;
        }
        if (reserved[step].find(movement.current_id) != reserved[step].end()) {
            std::cout << "Step " << step << ", edge conflict caused by agent " <<
                                    movement.agent_number << "!" << std::endl;
        }
        reserved[step].insert(movement.current_id);
        reserved[step].insert(movement.previous_id);
        this->map.replace_agent(movement.previous_id, movement.current_id);
    }
    for (int ind = 0; ind < map.number_of_agents; ++ind) {
        if (map.agents[ind].start_x != map.agents[ind].finish_x || 
            map.agents[ind].start_y != map.agents[ind].finish_y) {
            std::cout << "Agent " << ind + 1 << " did not reach it`s goal position!" << std::endl;
            return false;
        }
    }
    this->reset_map();
    return true;
}

long long PushAndRotate::compute_quality() const {
    std::unordered_set<int> finished_agents;
    long long answer = 0;
    for (int ind = (int)this->logger.moves.size() - 1; ind >= 0; --ind) {
        if (finished_agents.find(this->logger.moves[ind].agent_number) == finished_agents.end()) {
            finished_agents.insert(this->logger.moves[ind].agent_number);
            answer += ind + 1;
        }
    }
    return answer;
}

PushAndRotate::PushAndRotate(const std::string& file_name_input, 
                             const std::string& file_name_output,
                             bool parallel_mode) : 
                             logger(Logger(file_name_output.c_str())) {
    auto begin = std::chrono::steady_clock::now();
    std::cout << "==================== New simulation ====================" << std::endl;
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
    auto moving_phase = std::chrono::steady_clock::now();
    if (parallel_mode) {
        this->reset_map();
    }
    PostProcess(this->map, this->logger.moves, parallel_mode);
    bool correct = this->check_answer();
    std::cout << std::endl;
    auto end = std::chrono::steady_clock::now();
    auto moving_phase_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(moving_phase - begin);
    auto post_processing_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(end - moving_phase);
    int total_steps = this->logger.moves.back().step + 1;
    logger.print_log_second(this->map, parallel_mode, total_steps, this->compute_quality(),
                                  moving_phase_time.count(), post_processing_time.count());
    if (correct) {
        std::cout << "Algorithm complited!" << std::endl << std::endl;
        std::cout << "Number of steps = " << total_steps << std::endl;
        std::cout << "Lengths summary = " << this->compute_quality() << std::endl;
        this->is_solution = true;
    } else {
        std::cout << "Algorithm got incorrect result!" << std::endl;
        this->is_solution = false;
    }
    std::cout << "Main part time: " << moving_phase_time.count() << "ms" << std::endl;
    std::cout << "Optimization part time: " << post_processing_time.count() << "ms" << std::endl;
    std::cout << "Total time: " << moving_phase_time.count() + 
                                   post_processing_time.count() << "ms" << std::endl;

    std::cout << "========================================================" << std::endl;
}