#ifndef LOGGER_H
#define LOGGER_H

const int NUMBER_LENGTH = 20;

#include <fstream>
#include <string>
#include <vector>

class Movement {
public:
    int agent_number;
    int previous_id;
    int current_id;
    int step;

    Movement(int input_number, int input_prev, int input_current):
                                agent_number(input_number),
                                previous_id(input_prev),
                                current_id(input_current) {}

    bool operator < (const Movement& other) const {
        return (std::tie(this->step, this->agent_number, this->previous_id, this->current_id) <
               std::tie(other.step, other.agent_number, other.previous_id, other.current_id));
    }
};

class Logger {
public:
    std::vector<Movement> moves;
    char const* file_name;

    Logger(char const* input_name): file_name(input_name) {}

    void print_log_first(const Map& map) const {
        tinyxml2::XMLDocument document;
        tinyxml2::XMLElement* root_tag = document.NewElement("root");
        document.InsertEndChild(root_tag);
        tinyxml2::XMLElement* map_tag = document.NewElement("map");
        root_tag->InsertEndChild(map_tag);
        tinyxml2::XMLElement* width_tag = document.NewElement("width");
        tinyxml2::XMLElement* height_tag = document.NewElement("height");
        tinyxml2::XMLElement* cellsize_tag = document.NewElement("cellsize");
        map_tag->InsertEndChild(width_tag);
        map_tag->InsertEndChild(height_tag);
        map_tag->InsertEndChild(cellsize_tag);
        char buffer[NUMBER_LENGTH];
        snprintf(buffer, sizeof(buffer), "%d", map.get_width());
        width_tag->InsertEndChild(document.NewText(buffer));
        snprintf(buffer, sizeof(buffer), "%d", map.get_height());
        height_tag->InsertEndChild(document.NewText(buffer));
        snprintf(buffer, sizeof(buffer), "%g", map.get_cell_size());
        cellsize_tag->InsertEndChild(document.NewText(buffer));
        tinyxml2::XMLElement* agents_tag = document.NewElement("agents");
        map_tag->InsertEndChild(agents_tag);
        for (int ind = 0; ind < map.number_of_agents; ++ind) {
            tinyxml2::XMLElement* agent_tag = document.NewElement("agent");
            agents_tag->InsertEndChild(agent_tag);
            tinyxml2::XMLElement* startx_tag = document.NewElement("startx");
            tinyxml2::XMLElement* starty_tag = document.NewElement("starty");
            tinyxml2::XMLElement* finishx_tag = document.NewElement("finishx");
            tinyxml2::XMLElement* finishy_tag = document.NewElement("finishy");
            agent_tag->InsertEndChild(startx_tag);
            agent_tag->InsertEndChild(starty_tag);
            agent_tag->InsertEndChild(finishx_tag);
            agent_tag->InsertEndChild(finishy_tag);
            snprintf(buffer, sizeof(buffer), "%d", map.agents[ind].start_x);
            startx_tag->InsertEndChild(document.NewText(buffer));
            snprintf(buffer, sizeof(buffer), "%d", map.agents[ind].start_y);
            starty_tag->InsertEndChild(document.NewText(buffer));
            snprintf(buffer, sizeof(buffer), "%d", map.agents[ind].finish_x);
            finishx_tag->InsertEndChild(document.NewText(buffer));
            snprintf(buffer, sizeof(buffer), "%d", map.agents[ind].finish_y);
            finishy_tag->InsertEndChild(document.NewText(buffer));
        }
        tinyxml2::XMLElement* grid_tag = document.NewElement("grid");
        map_tag->InsertEndChild(grid_tag);
        for (int y = 0; y < map.get_height(); ++y) {
            std::string str_value;
            for (int x = 0; x < map.get_width(); ++x) {
                str_value += ('0' + (map.is_obstacle(x, y)));
                if (x != map.get_width() - 1) {
                    str_value += ' ';
                }
            }
            tinyxml2::XMLElement* row_tag = document.NewElement("row");
            grid_tag->InsertEndChild(row_tag);
            row_tag->InsertEndChild(document.NewText(str_value.c_str()));
        }
        tinyxml2::XMLElement* algo_tag = document.NewElement("algorithm");
        root_tag->InsertEndChild(algo_tag);
        tinyxml2::XMLElement* diagonal_tag = document.NewElement("allowdiagonal");
        tinyxml2::XMLElement* corner_tag = document.NewElement("cutcorners");
        tinyxml2::XMLElement* squeeze_tag = document.NewElement("allowsqueeze");
        algo_tag->InsertEndChild(diagonal_tag);
        algo_tag->InsertEndChild(corner_tag);
        algo_tag->InsertEndChild(squeeze_tag);
        if (map.allow_diagonal) {
            diagonal_tag->InsertEndChild(document.NewText("true"));
        } else {
            diagonal_tag->InsertEndChild(document.NewText("false"));
        }
        if (map.cut_corners) {
            corner_tag->InsertEndChild(document.NewText("true"));
        } else {
            corner_tag->InsertEndChild(document.NewText("false"));
        }
        if (map.allow_squeeze) {
            squeeze_tag->InsertEndChild(document.NewText("true"));
        } else {
            squeeze_tag->InsertEndChild(document.NewText("false"));
        }
        document.SaveFile(this->file_name);
    }

    void print_log_second(const Map& map, bool parallel_mode) const {
        tinyxml2::XMLDocument document;
        document.LoadFile(this->file_name);
        tinyxml2::XMLElement* root_tag = document.FirstChildElement("root");
        tinyxml2::XMLElement* log_tag = document.NewElement("log");
        root_tag->InsertEndChild(log_tag);
        for (auto move: this->moves) {
            tinyxml2::XMLElement* turn_tag = document.NewElement("turn");
            turn_tag->SetAttribute("agent", move.agent_number + 1);
            turn_tag->SetAttribute("x", move.current_id % map.get_width());
            turn_tag->SetAttribute("y", move.current_id / map.get_width());
            turn_tag->SetAttribute("step", move.step);
            log_tag->InsertEndChild(turn_tag);
        }
        document.SaveFile(this->file_name);
    }
};

#endif