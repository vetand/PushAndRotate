#include "algorithm.h"

int PushAndRotate::MovingPhase::A_Star::get_heuristic(const Map& map, int current_id, 
                                                                      int finish_id) {
    int x1 = current_id % map.get_width();
    int y1 = current_id / map.get_width();
    int x2 = finish_id % map.get_width();
    int y2 = finish_id / map.get_width();
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

PushAndRotate::MovingPhase::A_Star::AstarNode::AstarNode(int id, const Map& map, int finish_id, 
                                                                                A_Star* owner) {
    this->id = id;
    this->x = id % map.get_width();
    this->y = id / map.get_width();
    this->g_value = 0;
    this->h_value = owner->get_heuristic(map, id, finish_id);
}

bool PushAndRotate::MovingPhase::A_Star::AstarNode::operator < (const AstarNode& other) const {
    if (this->h_value + this->g_value == other.h_value + other.g_value) {
        return (this->id < other.id);
    }
    return (this->h_value + this->g_value > other.h_value + other.g_value);
}

bool PushAndRotate::MovingPhase::A_Star::end_loop(int mode, int current_id, int finish_id,
                            const Map& map, const std::unordered_set<int>& checked) const {
    if (mode == 0) {
        return (current_id == finish_id);
    }
    if (checked.find(current_id) != checked.end()) {
        return false;
    }
    int x = current_id % map.get_width();
    int y = current_id / map.get_width();
    if (mode == 1) {
        return !map.is_start(x, y);
    }
    return (map.get_degree(current_id) >= 3);
}

PushAndRotate::MovingPhase::A_Star::A_Star() {}

PushAndRotate::MovingPhase::A_Star::A_Star(const Map& map, const std::set<int>& blocked, 
                                            int start_id, int finish_id, int mode,
                                            const std::unordered_set<int>& checked) {
    std::priority_queue<AstarNode> queue;
    std::set<int> visited;
    std::map<int, int> previous_id;
    AstarNode start_node = AstarNode(start_id, map, finish_id, this);
    queue.push(start_node);
    this->path_found = false;
    while (!queue.empty()) {
        AstarNode current = queue.top();
        queue.pop();
        if (this->end_loop(mode, current.id, finish_id, map, checked)) {
            this->path_found = true;
            finish_id = current.id;
            break;
        }
        std::vector<int> successors = map.find_successors(current.id);
        for (int neighbor: successors) {
            AstarNode new_neighbor = AstarNode(neighbor, map, finish_id, this);
            int dist_cur = new_neighbor.h_value * (mode == 0) + new_neighbor.g_value;
            int dist_new = current.h_value + current.g_value + 1;
            bool lower = (dist_new < dist_cur);
            if (blocked.find(new_neighbor.id) != blocked.end() || 
                        (visited.find(new_neighbor.id) != visited.end() && !lower)) {
                continue;
            }
            new_neighbor.g_value = current.g_value + 1;
            previous_id[new_neighbor.id] = current.id;
            queue.push(new_neighbor);
            visited.insert(new_neighbor.id);
        }
    }
    if (this->path_found == false) {
        this->answer.push_back(-1);
        return;
    }
    int current_id = finish_id;
    while (current_id != start_id) {
        this->answer.push_back(current_id);
        current_id = previous_id[current_id];
    }
    this->answer.push_back(current_id);
    for (int ind = 0; ind < this->answer.size() / 2; ++ind) {
        std::swap(this->answer[ind], this->answer[this->answer.size() - ind - 1]);
    }
}

bool PushAndRotate::MovingPhase::A_Star::get_path_found() const {
    return this->path_found;
}

std::vector<int> PushAndRotate::MovingPhase::A_Star::get_path() const {
    return this->answer;
}

void PushAndRotate::MovingPhase::move_agent(int current_id, int destination_id, Map& map, 
                                            std::vector<Movement>& moves) {
    map.replace_agent(current_id, destination_id);
    int x = destination_id % map.get_width();
    int y = destination_id / map.get_width();
    int agent_number = map.get_agent(x, y);
    Movement new_move = Movement(agent_number, current_id, destination_id);
    moves.push_back(new_move);
}

void PushAndRotate::MovingPhase::reverse(Map& map, std::vector<Movement>& moves) {
    std::vector<Movement> tmp;
    for (int ind = moves.size() - 1; ind >= 0; --ind) {
        move_agent(moves[ind].current_id, moves[ind].previous_id, map, tmp);
    }
    moves = tmp;
}

void PushAndRotate::MovingPhase::undo(Map& map, std::vector<Movement>& moves) {
    map.replace_agent(moves.back().current_id, moves.back().previous_id);
    moves.pop_back();
}

bool PushAndRotate::MovingPhase::clear_vertex(int current, Map& map, const std::set<int>& blocked, 
                                              std::vector<Movement>& moves) {
    int x = current % map.get_width();
    int y = current / map.get_width();
    if (!map.is_start(x, y)) {
        return true;
    }
    std::vector<int> path;

    std::unordered_set<int> checked;
    A_Star path_finder = A_Star(map, blocked, current, -1, 1, checked);
    if (path_finder.get_path_found()) {
        path = path_finder.get_path();
    } else {
        return false;
    }
    for (int pos_fin = 0; pos_fin < path.size(); ++pos_fin) {
        int current_id = path[pos_fin];
        x = current_id % map.get_width();
        y = current_id / map.get_width();
        if (!map.is_start(x, y)) {
            for (int pos = pos_fin - 1; pos >= 0; --pos) {
                this->move_agent(path[pos], path[pos + 1], map, moves);
            }
            return true;
        }
    }
    return false;
}

bool PushAndRotate::MovingPhase::multipush(int& center, int& second_agent,
                                           Map& map, std::vector<Movement>& moves,
                                           const std::vector<int>& input_path) {
    std::set<int> blocked;
    std::vector<int> path = input_path;
    if (!(path.size() >= 2 && path[1] == center)) {
        std::vector<int> tmp;
        tmp.push_back(center);
        for (int elem: path) {
            tmp.push_back(elem);
        }
        path = tmp;
        std::swap(center, second_agent);
    }
    std::vector<Movement> new_moves;
    blocked.insert(path[0]);
    blocked.insert(path[1]);
    for (int pos = 2; pos < path.size(); ++pos) {
        if (!this->clear_vertex(path[pos], map, blocked, new_moves)) {
            while (new_moves.size() > 0) {
                this->undo(map, new_moves);
            }
            return false;
        }
        blocked.erase(path[pos - 2]);
        blocked.insert(path[pos]);
        this->move_agent(path[pos - 1], path[pos], map, new_moves);
        this->move_agent(path[pos - 2], path[pos - 1], map, new_moves);
    }
    for (auto elem: new_moves) {
        moves.push_back(elem);
    }
    center = path[path.size() - 1];
    second_agent = path[path.size() - 2];
    return true;
}

bool PushAndRotate::MovingPhase::clear(int center, int second_agent, Map& map, 
                                                std::vector<Movement>& moves) {
    std::vector<int> clear_neighbors;
    std::vector<int> successors = map.find_successors(center);
    for (int neighbor: successors) {
        int x = neighbor % map.get_width();
        int y = neighbor / map.get_width();
        if (!map.is_start(x, y)) {
            clear_neighbors.push_back(neighbor);
        }
    }
    if (clear_neighbors.size() >= 2) {
        return true;
    }
    std::set<int> blocked;
    blocked.insert(center);
    blocked.insert(second_agent);
    if (clear_neighbors.size() == 1) {
        blocked.insert(clear_neighbors[0]);
    }
    std::vector<Movement> new_moves;
    for (int neighbor: successors) {
        if (neighbor == second_agent) {
            continue;
        }
        int x = neighbor % map.get_width();
        int y = neighbor / map.get_width();
        if (map.is_start(x, y) && this->clear_vertex(neighbor, map, blocked, new_moves)) {
            if (clear_neighbors.size() == 1) {
                for (auto elem: new_moves) {
                    moves.push_back(elem);
                }
                return true;
            }
            blocked.insert(neighbor);
            clear_neighbors.push_back(neighbor);
        }
    }
    if (clear_neighbors.size() == 0) {
        return false;
    }
    blocked.erase(clear_neighbors[0]);
    int control_point = new_moves.size();
    for (int neighbor: successors) {
        if (neighbor == second_agent || neighbor == clear_neighbors[0]) {
            continue;
        }
        if (this->clear_vertex(neighbor, map, blocked, new_moves)) {
            blocked.insert(neighbor);
            if (this->clear_vertex(clear_neighbors[0], map, blocked, new_moves)) {
                for (auto elem: new_moves) {
                    moves.push_back(elem);
                }
                return true;
            }
            blocked.erase(neighbor);
            break;
        }
    }
    while (new_moves.size() > control_point) {
        this->undo(map, new_moves);
    }
    this->move_agent(center, clear_neighbors[0], map, new_moves);
    this->move_agent(second_agent, center, map, new_moves);
    blocked.insert(clear_neighbors[0]);
    blocked.erase(second_agent);
    for (int neighbor: successors) {
        if (neighbor == second_agent || neighbor == clear_neighbors[0]) {
            continue;
        }
        if (this->clear_vertex(neighbor, map, blocked, new_moves)) {
            blocked.insert(neighbor);
            if (this->clear_vertex(second_agent, map, blocked, new_moves)) {
                this->move_agent(center, second_agent, map, new_moves);
                this->move_agent(clear_neighbors[0], center, map, new_moves);
                for (auto elem: new_moves) {
                    moves.push_back(elem);
                }
                return true;
            }
            blocked.erase(neighbor);
            break;
        }
    }
    while (new_moves.size() > control_point) {
        this->undo(map, new_moves);
    }
    if (!this->clear_vertex(second_agent, map, blocked, new_moves)) {
        while (!new_moves.empty()) {
            this->undo(map, new_moves);
        }
        return false;
    }
    blocked.insert(second_agent);
    int second_location = new_moves.back().current_id;
    blocked.insert(second_location);
    this->move_agent(center, second_agent, map, new_moves);
    if (!this->clear_vertex(clear_neighbors[0], map, blocked, new_moves)) {
        while (!new_moves.empty()) {
            this->undo(map, new_moves);
        }
        return false;
    }
    for (int neighbor: successors) {
        if (neighbor != clear_neighbors[0] && neighbor != second_agent) {
            this->move_agent(neighbor, center, map, new_moves);
            this->move_agent(center, clear_neighbors[0], map, new_moves);
            this->move_agent(second_agent, center, map, new_moves);
            this->move_agent(second_location, second_agent, map, new_moves);
            blocked.erase(second_location);
            blocked.insert(neighbor);
            if (this->clear_vertex(clear_neighbors[0], map, blocked, new_moves)) {
                for (auto elem: new_moves) {
                    moves.push_back(elem);
                }
                return true;
            } else {
                while (!new_moves.empty()) {
                    this->undo(map, new_moves);
                }
                return false;
            }
        }
    }
}

void PushAndRotate::MovingPhase::exchange(int center, int second_agent, Map& map, 
                                                   std::vector<Movement>& moves) {
    std::vector<int> clear_neighbors;
    std::vector<int> successors = map.find_successors(center);
    for (int neighbor: successors) {
        int x = neighbor % map.get_width();
        int y = neighbor / map.get_width();
        if (!map.is_start(x, y)) {
            clear_neighbors.push_back(neighbor);
        }
    }
    int v1 = clear_neighbors[0];
    int v2 = clear_neighbors[1];
    this->move_agent(center, v1, map, moves);
    this->move_agent(second_agent, center, map, moves);
    this->move_agent(center, v2, map, moves);
    this->move_agent(v1, center, map, moves);
    this->move_agent(center, second_agent, map, moves);
    this->move_agent(v2, center, map, moves);
}

bool PushAndRotate::MovingPhase::push(int current_id, int destination_id, Map& map, 
                                                        std::set<int>& blocked, 
                                                        std::vector<Movement>& moves) {
    int x = destination_id % map.get_width();
    int y = destination_id / map.get_width();
    if (blocked.find(destination_id) != blocked.end()) {
        return false;
    }
    if (map.is_start(x, y)) {
        blocked.insert(current_id);
        if (!clear_vertex(destination_id, map, blocked, moves)) {
            blocked.erase(current_id);
            return false;
        }
        blocked.erase(current_id);
    }
    this->move_agent(current_id, destination_id, map, moves);
    return true;
}

bool PushAndRotate::MovingPhase::swap(int first_id, int second_id, Map& map, 
                                           std::vector<Movement>& moves,
                                           const std::vector<Node>& nodes_list) {
    int source_first = first_id;
    int source_second = second_id;
    std::unordered_set<int> checked;
    std::set<int> blocked;
    while (true) {
        std::vector<Movement> new_moves;
        A_Star path_finder = A_Star(map, blocked, source_second, -1, 2, checked);
        if (path_finder.get_path_found() == false) {
            break;
        }
        checked.insert(path_finder.get_path().back());
        first_id = source_first;
        second_id = source_second;
        int log = this->multipush(first_id, second_id, map, new_moves, path_finder.get_path());
        if (!log) {
            continue;
        }
        log = this->clear(first_id, second_id, map, new_moves);
        if (log) {
            std::vector<Movement> exchange_moves;
            this->exchange(first_id, second_id, map, exchange_moves);
            for (auto elem: new_moves) {
                moves.push_back(elem);
            }
            for (auto elem: exchange_moves) {
                moves.push_back(elem);
            }
            this->reverse(map, new_moves);
            for (auto elem: new_moves) {
                moves.push_back(elem);
            }
            return true;
        } else {
            while (!new_moves.empty()) {
                this->undo(map, new_moves);
            }
        }
    }
    return false;
}

bool PushAndRotate::MovingPhase::rotate(Map& map, std::vector<Movement>& moves, 
                                                const std::vector<int>& cycle,
                                                const std::vector<Node>& nodes_list) {
    while (true) {
        std::cout << 1 << std::endl;
    }
    for (int ind = 0; ind < cycle.size(); ++ind) {
        int x = cycle[ind] % map.get_width();
        int y = cycle[ind] / map.get_width();
        if (!map.is_start(x, y)) {
            int pos = ind - 1;
            while (pos != ind) {
                this->move_agent(cycle[pos], cycle[(pos + 1) % cycle.size()], map, moves);
                --pos;
                if (pos < 0) {
                    pos += cycle.size();
                }
            }
            return true;
        }
    }
    std::set<int> blocked;
    for (int elem: cycle) {
        blocked.insert(elem);
    }
    for (int ind = 0; ind < cycle.size(); ++ind) {
        std::vector<Movement> new_moves;
        blocked.erase(cycle[ind]);
        if (this->clear_vertex(cycle[ind], map, blocked, new_moves)) {
            blocked.insert(cycle[ind]);
            int first_location = new_moves.back().current_id;
            std::vector<Movement> one_move;
            this->move_agent(cycle[(ind + 1) % cycle.size()], cycle[ind], map, one_move);
            std::vector<Movement> swap_moves;
            if (!this->swap(first_location, cycle[ind], map, swap_moves, nodes_list)) {
                this->undo(map, one_move);
                while (!new_moves.empty()) {
                    this->undo(map, new_moves);
                }
                break;
            }
            for (auto elem: new_moves) {
                moves.push_back(elem);
            }
            moves.push_back(one_move[0]);
            for (auto elem: swap_moves) {
                moves.push_back(elem);
            }
            int pos = (ind + 2) % cycle.size();
            while (pos != ind + 1) {
                int prev_pos = pos - 1;
                if (prev_pos < 0) {
                    prev_pos += cycle.size();
                }
                this->move_agent(cycle[pos], cycle[prev_pos], map, moves);
                pos = (pos + 1) % cycle.size();
            }
            this->reverse(map, new_moves);
            for (auto elem: new_moves) {
                moves.push_back(elem);
            }
            return true;
        }
        blocked.insert(cycle[ind]);
    }
    return false;
}

bool PushAndRotate::MovingPhase::is_polygon(const Map& map, const std::vector<Node>& nodes_list) {
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (map.is_free(x, y) && map.get_degree(id) != 2) {
            return false;
        }
    }
    return true;
}

std::vector<int> PushAndRotate::MovingPhase::detect_cycle(int current, int start, 
                                                          std::vector<int> resolving,
                                                          const Map& map, 
                                                          const std::vector<Node>& nodes_list) {
    std::vector<int> answer;
    answer.push_back(current);
    this->used[current] = true;
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (neighbor == start) {
            return answer;
        }
        if (std::find(resolving.begin(), resolving.end(), neighbor) != resolving.end() && 
                                                                !this->used[neighbor]) {
            std::vector<int> following = detect_cycle(neighbor, start, 
                                                            resolving, map, nodes_list);
            if (following.size() > 0) {
                for (int elem: following) {
                    answer.push_back(elem);
                }
                return answer;
            }
        }
    }
    answer.clear();
    return answer;
}

void PushAndRotate::MovingPhase::update_blocked(std::set<int>& finished, const Map& map, 
                                                const std::set<int> finished_agents) const {
    finished.clear();
    for (int number: finished_agents) {
        finished.insert(map.agents[number].start_x +
                        map.agents[number].start_y * map.get_width());
    }
}

void PushAndRotate::MovingPhase::full_debug_print(const Map& map) const {
    std::cout << "=============== Debug print map ===============" << std::endl;
    for (int y = 0; y < map.get_height(); ++y) {
        for (int x = 0; x < map.get_width(); ++x) {
            if (map.is_start(x, y)) {
                std::cout << map.get_agent(x, y) + 1;
            } else if (map.is_obstacle(x, y)) {
                std::cout << "*";
            } else {
                std::cout << ".";
            }
            std::cout << "\t";
        }
        std::cout << std::endl;
    }
    std::cout << "===============================================" << std::endl;
}

PushAndRotate::MovingPhase::MovingPhase() {}

PushAndRotate::MovingPhase::MovingPhase(Map& map, const std::vector<Node>& nodes_list, 
                                                                PushAndRotate* owner) {
    bool polygon = this->is_polygon(map, nodes_list);
    std::set<int> finished;
    std::set<int> finished_agents;
    std::vector<int> resolving;
    int current_agent = -1;
    int current_index = 0;
    while (current_index != map.number_of_agents) {
        if (current_agent == -1) {
            current_agent = owner->agents_order[current_index] - 1;
            ++current_index;
        }
        A_Star path_finder;
        std::unordered_set<int> ZERO;
        int start = map.agents[current_agent].start_y * map.get_width() +
                                                   map.agents[current_agent].start_x;
        int finish = map.agents[current_agent].finish_y * map.get_width() +
                                                   map.agents[current_agent].finish_x;
        if (polygon) {
            path_finder = A_Star(map, finished, start, finish, 0, ZERO);
        } else {
            std::set<int> blocked_zero;
            path_finder = A_Star(map, blocked_zero, start, finish, 0, ZERO);
        }
        resolving.push_back(start);
        if (path_finder.get_path_found() == false) {
            std::cout << "Agent number " << current_agent + 1 << " has no path to its ";
            std::cout << "goal position!" << std::endl;
            return;
        }
        std::vector<int> path = path_finder.get_path();
        for (int ind = 0; ind < path.size() - 1; ++ind) {
            if (std::find(resolving.begin(), resolving.end(), path[ind + 1]) != 
                                                                    resolving.end()) {
                this->used.assign(nodes_list.size(), -1);
                std::vector<int> cycle = this->detect_cycle(path[ind], path[ind], 
                                                            resolving, map, nodes_list);
                this->rotate(map, owner->logger.moves, cycle, nodes_list);
                for (int elem: cycle) {
                    resolving.erase(std::find(resolving.begin(), resolving.end(), elem));
                }
            } else {
                if (!this->push(path[ind], path[ind + 1], map, finished, 
                                                            owner->logger.moves)) {
                    int log = this->swap(path[ind], path[ind + 1], map, 
                                                owner->logger.moves, 
                                                nodes_list);
                }
                this->update_blocked(finished, map, finished_agents);
            }
            resolving.push_back(path[ind]);
        }
        finished.insert(finish);
        finished_agents.insert(current_agent);
        current_agent = -1;
        while (!resolving.empty()) {
            int vertex = *resolving.rbegin();
            int x = vertex % map.get_width();
            int y = vertex / map.get_width();
            if (map.is_start(x, y) && 
                    finished_agents.find(map.get_agent(x, y)) != finished_agents.end()) {
                Agent agent = map.agents[map.get_agent(x, y)];
                std::pair<int, int> finish = {agent.finish_x, agent.finish_y};
                if (finish != std::make_pair(x, y)) {
                    bool long_jump = (std::abs(x - agent.finish_x) > 1) || 
                                     (std::abs(y - agent.finish_y) > 1);
                    if (!map.is_start(agent.finish_x, agent.finish_y) && !long_jump) {
                        int finish_id = agent.finish_y * map.get_width() + agent.finish_x;
                        this->move_agent(vertex, finish_id, map, owner->logger.moves);
                    } else {
                        current_agent = map.get_by_finish(agent.finish_x, agent.finish_y);
                        break;
                    }
                }
            }
            resolving.pop_back();
        }
        this->update_blocked(finished, map, finished_agents);
    }
    std::cout << "Moving phase complited!" << std::endl;
}