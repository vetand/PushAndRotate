#include "algorithm.h"

void PushAndRotate::PostProcess::init(const Map& map, std::vector<Movement>& moves) {
    this->personal_moves.clear();
    this->events.clear();
    this->deleted.clear();
    for (int id = 0; id < map.get_height() * map.get_width(); ++id) {
        std::set<int> zero;
        this->events.push_back(zero);
    }

    for (int ind = 0; ind < moves.size(); ++ind) {
        this->events[moves[ind].previous_id].insert(ind);
        this->events[moves[ind].current_id].insert(ind);
    }

    for (int ind = 0; ind < map.number_of_agents; ++ind) {
        std::set<int> zero;
        this->personal_moves.push_back(zero);
    }

    for (int ind = 0; ind < moves.size(); ++ind) {
        this->personal_moves[moves[ind].agent_number].insert(ind);
    }
}

void PushAndRotate::PostProcess::remove_redundant(const Map& map, std::vector<Movement>& moves) {
    for (int agent = 0; agent < map.number_of_agents; ++agent) {
        std::set<int>::iterator it = this->personal_moves[agent].begin();
        while (it != this->personal_moves[agent].end()) {
            int num = *it;
            int id = moves[num].previous_id;
            if (this->events[id].upper_bound(num) == this->events[id].end()) {
                ++it;
                continue;
            }
            int next_move = *this->events[id].upper_bound(num);
            if (moves[next_move].agent_number == agent) {
                while (*it != next_move) {
                    this->deleted.insert(*it);
                    ++it;
                }
                this->deleted.insert(*it);
            }
            ++it;
        }
    }

    std::vector<Movement> new_moves;
    for (int ind = 0; ind < moves.size(); ++ind) {
        if (deleted.find(ind) == deleted.end()) {
            new_moves.push_back(moves[ind]);
        }
    }
    moves = new_moves;
}

void PushAndRotate::PostProcess::assign_steps(std::vector<Movement>& moves, Map& map) {
    this->init(map, moves);
    std::vector<bool> performed;
    std::set<int> deleted;
    performed.assign(moves.size(), false);
    for (int step = 0; step < moves.size(); ++step) {
        std::vector<int> perform_now;
        std::set<int> agents;
        for (int ind = step; ind < moves.size(); ++ind) {
            if (performed[ind]) {
                continue;
            }
            int agent = moves[ind].agent_number;
            if (agents.find(agent) != agents.end()) {
                continue;
            }
            int dest_point = moves[ind].current_id;
            int x1 = map.agents[moves[ind].agent_number].start_x;
            int y1 = map.agents[moves[ind].agent_number].start_y;
            int x2 = moves[ind].current_id % map.get_width();
            int y2 = moves[ind].current_id / map.get_width();
            if (std::abs(x1 - x2) > 1 || std::abs(y1 - y2) > 1 || (x1 == x2 && y1 == y2)) {
                continue;
            }
            if (this->events[dest_point].lower_bound(step) != this->events[dest_point].end() &&
                    moves[*this->events[dest_point].lower_bound(step)].agent_number == agent) {
                perform_now.push_back(ind);
                performed[ind] = true;
                agents.insert(agent);
            }
        }
        for (int ind: perform_now) {
            Movement move = moves[ind];
            this->events[moves[ind].previous_id].erase(ind);
            this->events[moves[ind].current_id].erase(ind);
            int x = map.agents[moves[ind].agent_number].start_x;
            int y = map.agents[moves[ind].agent_number].start_y;
            moves[ind].previous_id = y * map.get_width() + x;
            map.replace_agent(y * map.get_width() + x, move.current_id);
            moves[ind].step = step;
        }
        agents.clear();
        for (int ind = moves.size() - 1; ind >= step; --ind) {
            if (performed[ind]) {
                agents.insert(moves[ind].agent_number);
            }
            if (!performed[ind] && agents.find(moves[ind].agent_number) != agents.end()) {
                this->events[moves[ind].previous_id].erase(ind);
                this->events[moves[ind].current_id].erase(ind);
                performed[ind] = true;
                deleted.insert(ind);
            }
        }
    }
    std::vector<Movement> new_moves;
    for (int ind = 0; ind < moves.size(); ++ind) {
        if (deleted.find(ind) == deleted.end()) {
            new_moves.push_back(moves[ind]);
        }
    }
    moves = new_moves;
    std::sort(moves.begin(), moves.end());
    int current_step = 0;
    int real_step = 0;
    for (int ind = 0; ind < moves.size(); ++ind) {
        if (ind > 0 && moves[ind].step != current_step) {
            ++real_step;
        }
        current_step = moves[ind].step;
        moves[ind].step = real_step;
    }
}

PushAndRotate::PostProcess::PostProcess(Map& map, std::vector<Movement>& moves,
                                                        bool parallel_mode) {
    this->init(map, moves);
    int prev_length = moves.size();
    this->remove_redundant(map, moves);
    int new_length = moves.size();
    while (new_length != prev_length) {
        prev_length = new_length;
        this->init(map, moves);
        this->remove_redundant(map, moves);
        new_length = moves.size();
    }
    for (int i = 0; i < moves.size(); ++i) {
        moves[i].step = i;
    }
    if (parallel_mode) {
        this->assign_steps(moves, map);
    }
}