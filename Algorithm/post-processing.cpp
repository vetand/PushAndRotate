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


PushAndRotate::PostProcess::PostProcess(const Map& map, std::vector<Movement>& moves) {
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
}