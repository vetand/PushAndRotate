#ifndef ALGO_H
#define ALGO_H

#include "map.h"
#include "logger.h"

#include <algorithm>
#include <chrono>
#include <list>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>

class PushAndRotate {
private:
    std::vector<Node> nodes_list;
    std::vector<int> connect_empty;
    std::vector<int> subgraphs_order;
    std::vector<int> agents_order;

    class BiconnectedPhase {
    private:

        std::vector<int> time_in;
        std::vector<int> time_out;
        std::vector<int> min_descendant;
        std::vector<bool> visited;
        std::vector<bool> bridge_point;
        int current_time;

        void bridge_dfs(int current, const Map& map, int prev_id);

        void component_dfs(int current, const Map& map, std::vector<Node>& nodes_list,
                                                                   int current_color);

    public:
        BiconnectedPhase(const Map& map, std::vector<Node>& nodes_list);
    };

    class JointPhase {
    private:
        std::vector<bool> visited;
        int current_subgraph;

        void dfs(int current, const Map& map, std::vector<Node>& nodes_list,
                                                       int component_color);

        bool is_trivial(int current, const Map& map, const std::vector<Node>& nodes_list);

        bool is_joint(int current, const Map& map, std::vector<Node>& nodes_list);

    public:
        JointPhase(const Map& map, std::vector<Node>& nodes_list);
    };

    class MergePhase {
    private:
        std::vector<bool> visited;
        int current_connect;

        void count_dfs(int current, const Map& map, std::vector<Node>& nodes_list,
                                                            PushAndRotate* owner);

        bool merge_dfs(int current, const Map& map, std::vector<Node>& nodes_list,
                                              int current_subgraph,
                                              int steps_passed, PushAndRotate *owner);

    public:
        MergePhase(const Map& map, std::vector<Node>& nodes_list, PushAndRotate* owner);
    };

    class AssigningPhase {
    private:
        std::vector<bool> visited;

        bool is_inner(int current, const Map& map, const std::vector<Node> nodes_list);

        int count_dfs(int current, Map& map, const std::vector<Node>& nodes_list);

        void assign_dfs(int current, Map& map, const std::vector<Node>& nodes_list, int prev,
                                                             int subgraph, int slots_remain);

        void check_vertex(int current, Map& map, const std::vector<Node>& nodes_list,
                                                                PushAndRotate* owner);

    public:
        AssigningPhase(Map& map, const std::vector<Node>& nodes_list, PushAndRotate* owner);
    };

    class SubgraphsSortPhase {
    private:
        std::vector<std::vector<int>> constraints;
        std::vector<bool> visited;

        int get_next_subgraph(int current, const Map& map, 
                              const std::vector<Node>& nodes_list, int prev);

        void check_plank(int current, const Map& map, const std::vector<Node>& nodes_list,
                                            int prev, int low_subgraph, int high_subgraph);

        void top_sort(PushAndRotate* owner, int subgraph);

        void reverse_order(PushAndRotate* owner) const;

        bool check_sort(PushAndRotate* owner) const;

        void print_order(PushAndRotate* owner) const;

    public:
        SubgraphsSortPhase(const Map& map, const std::vector<Node>& nodes_list, 
                                                         PushAndRotate* owner);
    };

    class MovingPhase {
    private:

        class A_Star {
        private:
            std::vector<int> answer;
            bool path_found;

            int get_heuristic(const Map& map, int current_id, int finish_id);

            bool end_loop(int mode, int current_id, int finish_id, const Map& map, 
                                    const std::unordered_set<int>& checked) const;

            class AstarNode {
            public:
                int id;
                int x;
                int y;
                int g_value;
                int h_value;

                AstarNode(int id, const Map& map, int finish_id, A_Star* owner);

                bool operator < (const AstarNode& other) const;
            };

        public:
            A_Star();

            A_Star(const Map& map, const std::set<int>& blocked, int start_id,
                                            int finish_id,
                                            int mode,
                                            const std::unordered_set<int>& checked);

            bool get_path_found() const;

            std::vector<int> get_path() const;
        };

        void move_agent(int current_id, int destination_id, Map& map, 
                                        std::vector<Movement>& moves);

        void reverse(Map& map, std::vector<Movement>& moves);

        bool clear_vertex(int current, Map& map, const std::set<int>& blocked, 
                                                 std::vector<Movement>& moves);

        bool multipush(int& center, int& second_agent,Map& map, std::vector<Movement>& moves,
                                                         const std::vector<int>& input_path);

        bool clear(int center, int second_agent, Map& map, std::vector<Movement>& moves);

        void exchange(int center, int second_agent, Map& map, std::vector<Movement>& moves);

        bool push(int current_id, int destination_id, Map& map, std::set<int>& blocked, 
                                                                std::vector<Movement>& moves);

        bool swap(int first_id, int second_id, Map& map, std::vector<Movement>& moves,
                                                   const std::vector<Node>& nodes_list);

        bool rotate(Map& map, std::vector<Movement>& moves, const std::vector<int>& cycle,
                                                            const std::vector<Node>& nodes_list);

        bool is_polygon(const Map& map, const std::vector<Node>& nodes_list);

        std::vector<int> detect_cycle(int current, int start, std::vector<int> resolving,
                                      const Map& map, const std::vector<Node>& nodes_list);

        void update_blocked(std::set<int>& finished, const Map& map, 
                                const std::set<int> finished_agents) const;

        void full_debug_print(const Map& map) const;

        std::vector<bool> used;

    public:
        MovingPhase();

        MovingPhase(Map& map, const std::vector<Node>& nodes_list, PushAndRotate* owner);

        void undo(Map& map, std::vector<Movement>& moves);
    };

    class PostProcess {
    private:
        std::unordered_set<int> deleted;
        std::vector<std::set<int>> events;
        std::vector<std::list<int>> personal_moves;
        std::vector<std::vector<std::unordered_map<int, std::set<int>>>> local_moves;

        void init(const Map& map, std::vector<Movement>& moves);

        void init_local_moves(const Map& map, std::vector<Movement>& moves);

        void remove_redundant(const Map& map, std::vector<Movement>& moves);

        void assign_steps(std::vector<Movement>& moves, Map& map);

    public:
        PostProcess(Map& map, std::vector<Movement>& moves, bool parallel_mode);
    };

    void nodes_list_init();

    void reset_map();

    bool check_answer();

    long long compute_quality() const;

    MovingPhase mover;

public:
    bool is_solution;
    Logger logger;
    Map map;

    std::vector<int> get_agents_order() const;
    
    PushAndRotate(const std::string& file_name_input, const std::string& file_name_output,
                                                  bool parallel_mode);

};

#endif