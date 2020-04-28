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
        std::list<int> zero;
        this->personal_moves.push_back(zero);
    }

    for (int ind = 0; ind < moves.size(); ++ind) {
        this->personal_moves[moves[ind].agent_number].push_back(ind);
    }
    this->local_moves.clear();
    this->local_moves.resize(map.number_of_agents);
    for (int ind = 0; ind < map.number_of_agents; ++ind) {
        this->local_moves[ind].clear();
        this->local_moves[ind].resize(map.get_width() * map.get_height());
    }
    for (int ind = 0; ind < moves.size(); ++ind) {
        int dest_point = moves[ind].current_id;
        int agent = moves[ind].agent_number;
        std::vector<int> successors = map.find_successors(dest_point);
        for (int neighbor: successors) {
            this->local_moves[agent][neighbor][dest_point].insert(ind);
        }
    }
}

void PushAndRotate::PostProcess::remove_redundant(const Map& map, std::vector<Movement>& moves) {
    for (int agent = 0; agent < map.number_of_agents; ++agent) {
        std::list<int>::iterator it = this->personal_moves[agent].begin();
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
    this->deleted.clear();
    performed.assign(moves.size(), false);
    int assigned = 0;
    std::vector<int> last_performed;
    last_performed.assign(map.number_of_agents, -1);
    for (int step = 0; step < moves.size(); ++step) {
        std::vector<int> perform_now;
        for (int ind = 0; ind < map.number_of_agents; ++ind) {
            int x = map.agents[ind].start_x;
            int y = map.agents[ind].start_y;
            int position = map.get_width() * y + x;
            int max_ind = -1;
            for (auto par: this->local_moves[ind][position]) {
                int dest = par.first;
                int move_ind = *par.second.upper_bound(last_performed[ind]); 
                if (this->events[dest].begin() != this->events[dest].end() &&
                        moves[*this->events[dest].begin()].agent_number == ind) {
                    if (move_ind > max_ind) {
                        max_ind = move_ind;
                    }
                }
            }
            if (max_ind != -1) {
                perform_now.push_back(max_ind);
                last_performed[ind] = max_ind;
                performed[max_ind] = true;
            }
        }
        if (perform_now.size() == 0) {
            break;
        }
        for (int ind: perform_now) {
            Movement move = moves[ind];
            int id_1 = moves[ind].previous_id;
            int id_2 = moves[ind].current_id;
            this->events[id_1].erase(ind);
            this->events[id_2].erase(ind);
            int x = map.agents[moves[ind].agent_number].start_x;
            int y = map.agents[moves[ind].agent_number].start_y;
            moves[ind].previous_id = y * map.get_width() + x;
            map.replace_agent(y * map.get_width() + x, move.current_id);
            moves[ind].step = step;
        }
        for (int move_ind = 0; move_ind < moves.size(); ++move_ind) {
            int agent = moves[move_ind].agent_number;
            int id_1 = moves[move_ind].previous_id;
            int id_2 = moves[move_ind].current_id;
            if (last_performed[agent] >= move_ind && performed[move_ind] == false) {
                deleted.insert(move_ind);
                this->events[id_1].erase(move_ind);
                this->events[id_2].erase(move_ind);
                performed[move_ind] = true;
            }
        }
    }
    std::vector<Movement> new_moves;
    for (int ind = 0; ind < moves.size(); ++ind) {
        if (this->deleted.find(ind) == this->deleted.end()) {
            new_moves.push_back(moves[ind]);
        }
    }
    moves = new_moves;
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
        this->init(map, moves);
        this->assign_steps(moves, map);
    }
}