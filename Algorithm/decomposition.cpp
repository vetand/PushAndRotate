#include "algorithm.h"

void PushAndRotate::BiconnectedPhase::bridge_dfs(int current, const Map& map, int prev_id) {
    this->time_in[current] = current_time;
    this->visited[current] = true;
    this->min_descendant[current] = current;
    std::vector<int> successors = map.find_successors(current);
    for (int i = 0; i < successors.size(); ++i) {
        if (!visited[successors[i]]) {
            ++this->current_time;
            bridge_dfs(successors[i], map, current);
        }
    }
    for (int i = 0; i < successors.size(); ++i) {
        int id_current_best = this->min_descendant[current];
        int id_candidate_best = this->min_descendant[successors[i]];
        if (successors[i] != prev_id &&
                    this->time_in[id_current_best] > this->time_in[id_candidate_best]) {
            this->min_descendant[current] = id_candidate_best;
        }
    }
    if (time_in[this->min_descendant[current]] == time_in[current]) {
        this->bridge_point[current] = true;
    }
    time_out[current] = this->current_time;
    ++this->current_time;
}

void PushAndRotate::BiconnectedPhase::component_dfs(int current, const Map& map, 
                                                    std::vector<Node>& nodes_list,
                                                    int current_color) {
    this->visited[current] = true;
    if (this->bridge_point[current]) {
        ++this->current_time;
        current_color = current_time;
    }
    std::vector successors = map.find_successors(current);
    nodes_list[current].component = current_color;
    for (int i = 0; i < successors.size(); ++i) {
        if (!visited[successors[i]]) {
            component_dfs(successors[i], map, nodes_list, current_color);
        }
    }
}

PushAndRotate::BiconnectedPhase::BiconnectedPhase(const Map& map, std::vector<Node>& nodes_list) {
    if (nodes_list.size() == 0) {
        return;
    }
    int nodes_number = map.get_height() * map.get_width();
    this->time_in.resize(nodes_number + 1);
    this->time_out.resize(nodes_number + 1);
    this->min_descendant.assign(nodes_number + 1, nodes_number);
    this->time_in[nodes_number] = nodes_number;
    this->visited.assign(nodes_number, false);
    this->bridge_point.assign(nodes_number, false);
    this->current_time = 0;
    for (int id = 0; id < map.get_width() * map.get_height(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (this->visited[id] == false && map.is_free(x, y)) {
            bridge_dfs(id, map, -1);
        }
    }
    this->current_time = 0;
    this->visited.assign(nodes_number, false);
    for (int id = 0; id < map.get_width() * map.get_height(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (this->visited[id] == false && map.is_free(x, y)) {
            component_dfs(id, map, nodes_list, -1);
        }
    }
    std::cout << "Biconnected phase passed!" << std::endl;
}

void PushAndRotate::JointPhase::dfs(int current, const Map& map, std::vector<Node>& nodes_list,
                                                                        int component_color) {
    nodes_list[current].subgraph = this->current_subgraph;
    this->visited[current] = true;
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (!this->visited[neighbor] && 
                        nodes_list[neighbor].component == component_color) {
            dfs(neighbor, map, nodes_list, component_color); 
        }
    }
}

bool PushAndRotate::JointPhase::is_trivial(int current, const Map& map, 
                                           const std::vector<Node>& nodes_list) {
    int current_color = nodes_list[current].component;
    std::vector<int> successors = map.find_successors(current);
    for (auto neighbor: successors) {
        if (nodes_list[neighbor].component == current_color) {
            return false;
        }
    }
    return true;
}

bool PushAndRotate::JointPhase::is_joint(int current, const Map& map, 
                                         std::vector<Node>& nodes_list) {
    if (map.get_degree(current) >= 3 && 
            is_trivial(current, map, nodes_list)) {
        return true;
    }
    return false;
}

PushAndRotate::JointPhase::JointPhase(const Map& map, std::vector<Node>& nodes_list) {
    for (int id = 0; id < map.get_width() * map.get_height(); ++id) {
        nodes_list[id].subgraph = -1;
    }
    this->visited.assign(nodes_list.size(), false);
    this->current_subgraph = 0;
    for (int id = 0; id < map.get_width() * map.get_height(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (this->visited[id] == false && map.is_free(x, y)) {
            if (!this->is_trivial(id, map, nodes_list)) {
                ++this->current_subgraph;
                this->dfs(id, map, nodes_list, nodes_list[id].component);
            } else if (this->is_joint(id, map, nodes_list)) {
                ++this->current_subgraph;
                nodes_list[id].subgraph = this->current_subgraph;
            }
        }
        this->visited[id] = true;
    }
    std::cout << "Joint phase passed!" << std::endl;
}

void PushAndRotate::MergePhase::count_dfs(int current, const Map& map, 
                                            std::vector<Node>& nodes_list,
                                            PushAndRotate* owner) {
    this->visited[current] = true;
    nodes_list[current].connect = this->current_connect;
    if (map.is_free(nodes_list[current].x, nodes_list[current].y)) {
        if (!map.is_start(nodes_list[current].x, nodes_list[current].y)) {
            ++owner->connect_empty[current_connect];
        }
    }
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (!this->visited[neighbor]) {
            count_dfs(neighbor, map, nodes_list, owner);
        }
    }
}

bool PushAndRotate::MergePhase::merge_dfs(int current, const Map& map, 
                                      std::vector<Node>& nodes_list,
                                      int current_subgraph,
                                      int steps_passed, PushAndRotate *owner) {
    bool answer = false;
    bool too_far = false;
    int remember_subgraph = nodes_list[current].subgraph;
    if (nodes_list[current].subgraph != -1) {
        if (steps_passed <= owner->connect_empty[nodes_list[current].connect] - 2) {
            answer = true;
        } else {
            current_subgraph = nodes_list[current].subgraph;
            too_far = true;
        }
        nodes_list[current].subgraph = current_subgraph;
        steps_passed = 0;
    }
    this->visited[current] = true;
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (!this->visited[neighbor]) {
            if (nodes_list[current].subgraph != -1 && 
                                nodes_list[neighbor].subgraph == remember_subgraph) {
                int log = merge_dfs(neighbor, map, nodes_list, 
                                    current_subgraph, 0, owner);
            } else {
                answer = answer | merge_dfs(neighbor, map, nodes_list, 
                                    current_subgraph, steps_passed + 1, owner);
            }
        }
    }
    if (nodes_list[current].subgraph == -1 && answer == true) {
        nodes_list[current].subgraph = current_subgraph;
    }
    return answer & (!too_far);
}

PushAndRotate::MergePhase::MergePhase(const Map& map, std::vector<Node>& nodes_list, 
                                                             PushAndRotate* owner) {
    this->visited.assign(nodes_list.size(), false);
    this->current_connect = -1;
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (map.is_free(x, y) && this->visited[id] == false) {
            ++this->current_connect;
            owner->connect_empty.push_back(0);
            this->count_dfs(id, map, nodes_list, owner);
        }
    }
    this->visited.assign(nodes_list.size(), false);
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (map.is_free(x, y) && this->visited[id] == false) {
            bool log = merge_dfs(id, map, nodes_list, -1, nodes_list.size(), owner);
        }
    }
    std::cout << "Merge phase connected! Subgraphs found!" << std::endl;
}

bool PushAndRotate::AssigningPhase::is_inner(int current, const Map& map, 
                                             const std::vector<Node> nodes_list) {
    std::vector<int> successors = map.find_successors(current);
    for (auto neighbor: successors) {
        if (nodes_list[neighbor].subgraph != nodes_list[current].subgraph) {
            return false;
        }
    }
    return true;
}

int PushAndRotate::AssigningPhase::count_dfs(int current, Map& map, 
                                             const std::vector<Node>& nodes_list) {
    this->visited[current] = true;
    int x = current % map.get_width();
    int y = current / map.get_width();
    int answer = 1 - (int)map.is_start(x, y);
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (!this->visited[neighbor]) {
            answer += this->count_dfs(neighbor, map, nodes_list);
        }
    }
    return answer;
}

void PushAndRotate::AssigningPhase::assign_dfs(int current, Map& map, 
                                               const std::vector<Node>& nodes_list, int prev,
                                               int subgraph, int slots_remain) {
    if (slots_remain == 0) {
        return;
    }
    int x = current % map.get_width();
    int y = current / map.get_width();
    if (map.is_start(x, y) && map.not_assigned(x, y)) {
        --slots_remain;
        map.agents[map.get_agent(x, y)].subgraph = subgraph;
    } else if (map.is_start(x, y)) {
        return;
    }
    if (nodes_list[current].subgraph != -1) {
        return;
    }
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (neighbor != prev) {
            this->assign_dfs(neighbor, map, nodes_list, current, subgraph, slots_remain);
        }
    }
}

void PushAndRotate::AssigningPhase::check_vertex(int current, Map& map, 
                                                 const std::vector<Node>& nodes_list,
                                                 PushAndRotate* owner) {
    if (nodes_list[current].subgraph == -1) {
        return;
    }
    int x = current % map.get_width();
    int y = current / map.get_width();
    if (this->is_inner(current, map, nodes_list)) {
        if (map.is_start(x, y) && map.not_assigned(x, y)) {
            map.agents[map.get_agent(x, y)].subgraph = nodes_list[current].subgraph;
        }
    } else {
        this->visited.assign(nodes_list.size(), false);
        this->visited[current] = true;
        std::vector<int> empty_numbers;
        std::vector<int> successors = map.find_successors(current);
        int except_vertex = 0;
        for (int neighbor: successors) {
            empty_numbers.push_back(this->count_dfs(neighbor, map, nodes_list));
            if (nodes_list[neighbor].subgraph == nodes_list[current].subgraph) {
                except_vertex += empty_numbers.back();
            }
        }
        int all_empty = owner->connect_empty[nodes_list[current].connect];
        for (int ind = 0; ind < successors.size(); ++ind) {
            int neighbor = successors[ind];
            if (nodes_list[neighbor].subgraph == nodes_list[current].subgraph) {
                continue;
            }
            int except_edge = all_empty - empty_numbers[ind];
            if ((except_edge >= 1 && except_edge < all_empty) || except_vertex >= 1) {
                if (map.is_start(x, y) && map.not_assigned(x, y)) {
                    map.agents[map.get_agent(x, y)].subgraph = 
                                                    nodes_list[current].subgraph;
                }
            }
            int slots_remain = except_edge - 1;
            this->assign_dfs(neighbor, map, nodes_list, current,
                                nodes_list[current].subgraph, slots_remain);
        }
    }
}

PushAndRotate::AssigningPhase::AssigningPhase(Map& map, const std::vector<Node>& nodes_list, 
                                                                     PushAndRotate* owner) {
    map.swap_start_finish();
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (map.is_free(x, y)) {
            this->check_vertex(id, map, nodes_list, owner);
        }
    }
    std::vector<int> first_assigment;
    for (auto& agent: map.agents) {
        first_assigment.push_back(agent.subgraph);
    }

    map.swap_start_finish();
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        if (map.is_free(x, y)) {
            this->check_vertex(id, map, nodes_list, owner);
        }
    }
    int pos = 0;
    owner->is_solution = true;
    for (auto& agent: map.agents) {
        int number = agent.subgraph;
        if (number != first_assigment[pos]) {
            std::cout << "There is no solution!" << std::endl;
            owner->is_solution = false;
            return;
        }
        ++pos;
    }
    std::cout << "Assigning phase passed!" << std::endl;
}

int PushAndRotate::SubgraphsSortPhase::get_next_subgraph(int current, const Map& map, 
                                    const std::vector<Node>& nodes_list, int prev) {
    if (nodes_list[current].subgraph != -1) {
        return nodes_list[current].subgraph;
    }
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (neighbor != prev) {
            return get_next_subgraph(neighbor, map, nodes_list, current);
        }
    }
    return -1;
}

void PushAndRotate::SubgraphsSortPhase::check_plank(int current, const Map& map, 
                                                    const std::vector<Node>& nodes_list,
                                                    int prev, int low_subgraph, int high_subgraph) {
    int x = current % map.get_width();
    int y = current / map.get_width();
    if (!map.is_finish(x, y)) {
        return;
    }
    if (map.is_finish(x, y) && 
                map.agents[map.get_by_finish(x, y)].subgraph == high_subgraph) {
        this->constraints[high_subgraph].push_back(low_subgraph);
        return;
    }
    if (nodes_list[current].subgraph != -1) {
        return;
    }
    std::vector<int> successors = map.find_successors(current);
    for (int neighbor: successors) {
        if (neighbor != prev) {
            check_plank(neighbor, map, nodes_list, current, low_subgraph,
                                                            high_subgraph);
        }
    }
}

void PushAndRotate::SubgraphsSortPhase::top_sort(PushAndRotate* owner, int subgraph) {
    this->visited[subgraph - 1] = true;
    for (int i = 0; i < this->constraints[subgraph].size(); ++i) {
        if (!this->visited[this->constraints[subgraph][i] - 1]) {
            top_sort(owner, this->constraints[subgraph][i]);
        }
    }
    owner->subgraphs_order.push_back(subgraph);
}

void PushAndRotate::SubgraphsSortPhase::reverse_order(PushAndRotate* owner) const {
    for (int ind = 0; ind < owner->subgraphs_order.size(); ++ind) {
        std::swap(owner->subgraphs_order[ind],
                  owner->subgraphs_order[owner->subgraphs_order.size() - ind - 1]);
    }
}

bool PushAndRotate::SubgraphsSortPhase::check_sort(PushAndRotate* owner) const {
    std::vector<int> positions(owner->subgraphs_order.size());
    for (int ind = 0; ind < owner->subgraphs_order.size(); ++ind) {
        positions[owner->subgraphs_order[ind] - 1] = ind;
    }
    for (int low = 0; low < this->constraints.size(); ++low) {
        for (int high: this->constraints[low]) {
            if (positions[low] > positions[high]) {
                std::cout << "There is no solution!" << std::endl;
                return false;
            }
        }
    }
    std::cout << "There is a possible solution!" << std::endl;
    return true;
}

void PushAndRotate::SubgraphsSortPhase::print_order(PushAndRotate* owner) const {
    std::cout << "Subgraphs order: ";
    for (int ind = 0; ind < owner->subgraphs_order.size(); ++ind) {
        std::cout << owner->subgraphs_order[ind] << ' ';
    }
    std::cout << std::endl << "Agents order: ";
    for (int ind = 0; ind < owner->agents_order.size(); ++ind) {
        std::cout << owner->agents_order[ind] << ' ';
    }
    std::cout << std::endl;
}

PushAndRotate::SubgraphsSortPhase::SubgraphsSortPhase(const Map& map, 
                                                      const std::vector<Node>& nodes_list, 
                                                      PushAndRotate* owner) {
    int subgraphs_number = 0;
    this->constraints.resize(nodes_list.size());
    for (int id = 0; id < nodes_list.size(); ++id) {
        int x = id % map.get_width();
        int y = id / map.get_width();
        int low_subgraph = nodes_list[id].subgraph;
        subgraphs_number = std::max(subgraphs_number, low_subgraph);
        if (map.is_obstacle(x, y) || nodes_list[id].subgraph == -1) {
            continue;
        }
        if (!map.is_finish(x, y)) {
            continue;
        }
        std::vector<int> successors = map.find_successors(id);
        for (int neighbor: successors) {
            int high_subgraph = this->get_next_subgraph(neighbor, map, nodes_list, id);
            if (nodes_list[neighbor].subgraph != nodes_list[id].subgraph) {
                if (high_subgraph == -1) {
                    continue;
                }
                if (map.agents[map.get_by_finish(x, y)].subgraph == high_subgraph) {
                    this->constraints[high_subgraph].push_back(low_subgraph);
                    continue;
                } else if (map.agents[map.get_by_finish(x, y)].subgraph != -1) {
                    continue;
                }
                this->check_plank(neighbor, map, nodes_list, id, low_subgraph,
                                                                 high_subgraph);
            }
        }
    }
    this->visited.assign(subgraphs_number, false);
    for (int ind = 1; ind <= subgraphs_number; ++ind) {
        if (!this->visited[ind - 1]) {
            this->top_sort(owner, ind);
        }
    }
    this->reverse_order(owner);
    this->check_sort(owner);
    for (int subgraph: owner->subgraphs_order) {
        for (int ind = 0; ind < map.agents.size(); ++ind) {
            if (map.agents[ind].subgraph == subgraph) {
                owner->agents_order.push_back(ind + 1);
            }
        }
    }
    for (int ind = 0; ind < map.agents.size(); ++ind) {
        if (map.agents[ind].subgraph == -1) {
            owner->agents_order.push_back(ind + 1);
        }
    }
    std::cout << "Subgraph sort phase completed!" << std::endl;
    this->print_order(owner);
}