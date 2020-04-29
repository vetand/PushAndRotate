#include "map.h"
#include "logger.h"

#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <unordered_set>

const int SCALE = 5;
const int FINISH_WEIGHT = 5;

class Priorities {
private:
    std::vector<std::unordered_set<int>> reserved;
    std::vector<std::vector<int>> path;
    int max_time;

    void init(const Map& map, const std::vector<Movement>& moves) {
        this->path.resize(map.number_of_agents);
        this->reserved.resize(moves.back().step + FINISH_WEIGHT + 2);
        this->max_time = moves.back().step + FINISH_WEIGHT;
        for (int ind = 0; ind < map.number_of_agents; ++ind) {
            int id = map.agents[ind].start_y * map.get_width() + map.agents[ind].start_x;
            this->path[ind].push_back(id);
            this->reserved[0].insert(id);
        }
        int pos = 0;
        for (int step = 0; step <= moves.back().step; ++step) {
            int ind = pos;
            for (int number = 0; number < map.number_of_agents; ++number) {
                this->reserved[step + 1].insert(this->path[number].back());
                this->path[number].push_back(this->path[number].back());
            }
            while (ind < moves.size() && moves[ind].step == step) {
                int agent = moves[ind].agent_number;
                int from = moves[ind].previous_id;
                int to = moves[ind].current_id;
                int step = moves[ind].step;
                this->reserved[step + 1].insert(to);
                this->reserved[step + 1].erase(from);
                this->path[agent].back() = to;
                ++ind;
            }
            pos = ind;
        }
    }

    class AstarNode {
    public:
        int id;
        int g_value;
        int h_value;
        bool is_finish;

        AstarNode(const Map& map, int current_id, int finish_id, int input_g_value) {
            this->id = current_id;
            int x1 = current_id % map.get_width();
            int y1 = current_id / map.get_width();
            int x2 = finish_id % map.get_width();
            int y2 = finish_id / map.get_width();
            this->h_value = std::max(std::abs(x1 - x2), std::abs(y1 - y2));
            if (map.is_finish(x1, y1)) {
                this->h_value += FINISH_WEIGHT;
            }
            this->g_value = input_g_value;
            this->is_finish = (current_id == finish_id);
        }

        bool operator < (const AstarNode& other) const {
            if (this->is_finish && other.is_finish) {
                return (this->g_value > other.g_value);
            }
            if (this->is_finish) {
                return 0;
            }
            if (other.is_finish) {
                return 1;
            }
            if (this->g_value + this->h_value == other.g_value + other.h_value) {
                if (this->g_value == other.g_value) {
                    return (this->id < other.id);
                } else {
                    return (this->g_value < other.g_value);
                }
            } else {
                return (this->g_value + this->h_value > other.g_value + other.h_value);
            }
        }
    };

    std::vector<int> a_star(int start_id, int finish_id) {
        std::priority_queue<AstarNode> queue;
        std::set<std::pair<int, int>> visited;
        std::map<std::pair<int, int>, std::pair<int, int>> previous_id;
        AstarNode start_node = AstarNode(map, start_id, finish_id, 0);
        queue.push(start_node);
        bool path_found = false;
        int finish_time;
        while (!queue.empty()) {
            AstarNode current = queue.top();
            queue.pop();
            if (current.id == finish_id) {
                bool able_stop = true;
                for (int step = current.g_value; step < this->max_time; ++step) {
                    if (this->reserved[std::max(step, 0)].find(current.id) != 
                                        this->reserved[std::max(step, 0)].end()) {
                        able_stop = false;
                        break;
                    }
                }
                if (able_stop) {
                    path_found = true;
                    finish_time = current.g_value;
                    break;
                }
            }
            if (current.g_value == this->max_time) {
                break;
            }
            std::vector<int> successors = map.find_successors(current.id);
            successors.push_back(current.id);
            for (int neighbor: successors) {
                AstarNode new_neighbor = AstarNode(this->map, neighbor, 
                                       finish_id, current.g_value + 1);
                new_neighbor.g_value = current.g_value + 1;
                if (visited.find({new_neighbor.id, new_neighbor.g_value}) != visited.end()) {
                    continue;
                }
                if (this->reserved[current.g_value].find(neighbor) != 
                                                this->reserved[current.g_value].end()) {
                    continue;
                }
                if (current.g_value != this->max_time - 1 && 
                                            this->reserved[current.g_value + 1].find(neighbor) != 
                                            this->reserved[current.g_value + 1].end()) {
                    continue;
                }
                if (current.g_value < this->max_time - 2 && 
                                this->reserved[current.g_value + 2].find(neighbor) != 
                                this->reserved[current.g_value + 2].end()) {
                    continue;
                }
                previous_id[{new_neighbor.id, new_neighbor.g_value}] = 
                                         {current.id, current.g_value};
                queue.push(new_neighbor);
                visited.insert({new_neighbor.id, new_neighbor.g_value});
            }
        }
        std::vector<int> answer;
        if (path_found == false) {
            answer.push_back(-1);
            std::cout << "One of the agents was blocked!" << std::endl;
            return answer;
        }
        int current_id = finish_id;
        int current_time = finish_time;
        while (current_id != start_id || current_time > 0) {
            answer.push_back(current_id);
            auto tmp = previous_id[{current_id, current_time}];
            current_id = tmp.first;
            current_time = tmp.second;
        }
        answer.push_back(current_id);
        for (int ind = 0; ind < answer.size() / 2; ++ind) {
            std::swap(answer[ind], answer[answer.size() - ind - 1]);
        }
        return answer;
    }

    void drop_path(int number) {
        for (int ind = 0; ind < this->path[number].size(); ++ind) {
            this->reserved[ind].erase(path[number][ind]);
        }
        this->path[number].clear();
    }

    void perform_agent(int number, const Map& map) {
        this->drop_path(number);
        int start_id = map.agents[number].start_x + map.agents[number].start_y * map.get_width();
        int finish_id = map.agents[number].finish_x + 
                        map.agents[number].finish_y * map.get_width();
        std::vector<int> new_path = this->a_star(start_id, finish_id);
        while (new_path.size() < this->max_time) {
            new_path.push_back(new_path.back());
        }
        for (int ind = 0; ind < (int)new_path.size(); ++ind) {
            this->reserved[ind].insert(new_path[ind]);
        }
        this->path[number] = new_path;
    }

    bool check_answer() {
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
            if (this->map.is_start(x, y) && this->map.get_agent(x, y) != 
                                                movement.agent_number) {
                std::cout << "Step " << step << ", agent " << movement.agent_number + 1
                        << " hits agent " << this->map.get_agent(x, y) + 1 << std::endl;
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
        return true;
    }

    long long compute_quality() const {
        std::unordered_set<int> finished_agents;
        long long answer = 0;
        for (int ind = (int)this->logger.moves.size() - 1; ind >= 0; --ind) {
            if (finished_agents.find(this->logger.moves[ind].agent_number) == 
                                                                finished_agents.end()) {
                finished_agents.insert(this->logger.moves[ind].agent_number);
                answer += this->logger.moves[ind].step;
            }
        }
        return answer;
    }

public:
    bool is_solution;
    Logger logger;
    Map map;

    Priorities(const std::string& file_name_input, const std::string& file_name_output,
                                                    const std::vector<Movement> moves):
                                              logger(Logger(file_name_output.c_str())) {
        auto begin = std::chrono::steady_clock::now();
        std::cout << "==================== New simulation ====================" << std::endl;
        this->map.get_map(file_name_input.c_str());
        if (this->map.number_of_agents == 0) {
            this->is_solution = false;
            return;
        }
        this->logger.print_log_first(this->map);
        this->init(this->map, moves);
        for (int cnt = 0; cnt < SCALE; ++cnt) {
            for (int agent = 0; agent < this->map.number_of_agents; ++agent) {
                this->perform_agent(agent, this->map);
            }
        }
        for (int agent = 0; agent < map.number_of_agents; ++agent) {
            for (int ind = 0; ind < this->path[agent].size() - 1; ++ind) {
                if (this->path[agent][ind] != this->path[agent][ind + 1]) {
                    this->logger.moves.push_back(Movement(agent, this->path[agent][ind], 
                                                     this->path[agent][ind + 1]));
                    this->logger.moves.back().step = ind;
                }
            }
        }
        auto moving_phase = std::chrono::steady_clock::now();
        this->logger.prepare_answer(map);
        bool correct = this->check_answer();
        std::cout << std::endl;
        auto end = std::chrono::steady_clock::now();
        auto moving_phase_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(moving_phase - begin);
        auto post_processing_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(end - moving_phase);
        int total_steps = this->logger.moves.back().step + 1;
        logger.print_log_second(this->map, total_steps, this->compute_quality(),
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
        std::cout << "Checker part time: " << 
                                       post_processing_time.count() << "ms" << std::endl;
        std::cout << "Total time: " << moving_phase_time.count() + 
                                       post_processing_time.count() << "ms" << std::endl;

        std::cout << "========================================================" << std::endl;
    }
};