#ifndef MAP_H
#define MAP_H

#include "node.h"
#include "tinyxml2.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Agent {
    int id;
    int start_x;
    int start_y;
    int finish_x;
    int finish_y;
    int subgraph;
};

class Map {
private:
    bool initialized;
    int height;
    int width;
    double cell_size;
    std::vector<std::vector<bool>> grid;
    std::map<std::pair<int, int>, int> agent_starts;
    std::map<std::pair<int, int>, int> agent_finishes;

    struct LocalNode {
        int x;
        int y;
    };

public:
    int number_of_agents;
    bool allow_diagonal;
    bool cut_corners;
    bool allow_squeeze;
    std::vector<Agent> agents;

    Map();
    Map(const Map& other);
    void get_map(const char* file_name);
    bool get_initialized() const;
    int get_height() const;
    int get_width() const;
    bool is_free(int i, int j) const;
    bool is_obstacle(int i, int j) const;
    bool within_map(int i, int j) const;
    bool is_start(int x, int y) const;
    bool not_assigned(int x, int y) const;
    int get_agent(int x, int y) const;
    bool is_finish(int x, int y) const;
    int get_by_finish(int x, int y) const;
    void replace_agent(int current_id, int destination_id);
    void swap_start_finish();

    int get_degree(int id) const;

    std::vector<int> find_successors(int id) const;
};

#endif