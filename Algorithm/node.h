#ifndef NODE_H
#define NODE_H

class Node {
public:
    int id;
    int x;
    int y;
    int component;
    int subgraph;
    int connect;

    Node(int input_id, int input_x, int input_y, int input_component) :
                                                id(input_id),
                                                x(input_x),
                                                y(input_y),
                                                component(input_component) {}

    bool operator < (const Node& other) {
        return (this->id < other.id);
    }
};

#endif