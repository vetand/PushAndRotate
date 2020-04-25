#include "map.h"

Map::Map() {
    this->initialized = false;
}

Map::Map(const Map& other) {
    this->height = other.height;
    this->width = other.width;
    this->cell_size = other.cell_size;
    this->number_of_agents = other.number_of_agents;
    this->initialized = other.initialized;
    this->grid.clear();
    this->agents.clear();
    for (int i = 0; i < this->height; ++i) {
        std::vector<bool> zero;
        this->grid.push_back(zero);
        for (int j = 0; j < this->width; ++j) {
            this->grid[i].push_back(other.is_obstacle(i, j));
        }
    }
    for (int i = 0; i < this->number_of_agents; ++i) {
        this->agents.push_back(other.agents[i]);
    }
}

void Map::get_map(const char* file_name) {
    this->number_of_agents = 0;
    this->agents.clear();
    this->grid.clear();
    this->agent_starts.clear();
    tinyxml2::XMLDocument document;
    try {
        if (document.LoadFile(file_name) != tinyxml2::XMLError::XML_SUCCESS) {
            std::cout << "Error opening XML file!" << std::endl;
            return;
        }
        std::cout << "File opened!" << std::endl;
        tinyxml2::XMLElement *root = document.FirstChildElement("root");
        tinyxml2::XMLElement *map = root->FirstChildElement("map");
        tinyxml2::XMLElement *height_tag = map->FirstChildElement("height");
        tinyxml2::XMLElement *width_tag = map->FirstChildElement("width");
        this->height = atoi(height_tag->GetText());
        this->width = atoi(width_tag->GetText());
        tinyxml2::XMLElement *agents_tag = map->FirstChildElement("agents");
        tinyxml2::XMLElement *agent_tag;
        if (agents_tag == NULL) {
            agents_tag = map;
        }
        for (agent_tag = agents_tag->FirstChildElement("agent"); agent_tag != NULL; 
                         agent_tag = agent_tag->NextSiblingElement("agent")) {
            Agent new_agent;
            const tinyxml2::XMLAttribute* id_attribute = agent_tag->FirstAttribute();
            new_agent.id = atoi(id_attribute->Value());
            tinyxml2::XMLElement *startx_tag = agent_tag->FirstChildElement("startx");
            tinyxml2::XMLElement *starty_tag = agent_tag->FirstChildElement("starty");
            tinyxml2::XMLElement *finishx_tag = agent_tag->FirstChildElement("finishx");
            tinyxml2::XMLElement *finishy_tag = agent_tag->FirstChildElement("finishy");
            new_agent.start_x = atoi(startx_tag->GetText());
            new_agent.start_y = atoi(starty_tag->GetText());
            new_agent.finish_x = atoi(finishx_tag->GetText());
            new_agent.finish_y = atoi(finishy_tag->GetText());
            this->agent_starts[{new_agent.start_x, new_agent.start_y}] = this->number_of_agents;
            this->agent_finishes[{new_agent.finish_x, new_agent.finish_y}] = this->number_of_agents;
            this->agents.push_back(new_agent);
            ++this->number_of_agents;
        }
        tinyxml2::XMLElement *grid_tag = map->FirstChildElement("grid");
        tinyxml2::XMLElement *row_tag;
        int counter = 0;
        for (row_tag = grid_tag->FirstChildElement(); row_tag != NULL; 
                       row_tag = row_tag->NextSiblingElement()) {
            std::vector<bool> new_row;
            std::string row_value = row_tag->GetText();
            for (int j = 0; j < this->width; ++j) {
                new_row.push_back(row_value[2 * j] == '1');
            }
            this->grid.push_back(new_row);
            ++counter;
        }
        if (counter != this->height) {
            return;
        }
        tinyxml2::XMLElement *diagonal_tag = map->FirstChildElement("allowdiagonal");
        this->allow_diagonal = !strcmp(diagonal_tag->GetText(), (char*)("true"));
        tinyxml2::XMLElement *corners_tag = map->FirstChildElement("cutcorners");
        this->cut_corners = !strcmp(corners_tag->GetText(), (char*)("true"));
        tinyxml2::XMLElement *squeeze_tag = map->FirstChildElement("allowsqueeze");
        this->allow_squeeze = !strcmp(squeeze_tag->GetText(), (char*)("true"));
        this->initialized = true;
    } catch (...) {
        this->initialized = false;
        std::cout << "Incorrect params!" << std::endl;
    }
    std::cout << "Map parsed successfully!" << std::endl;
    return;
}

bool Map::get_initialized() const {
    return this->initialized;
}

int Map::get_height() const {
    return this->height;
}

int Map::get_width() const {
    return this->width;
}

bool Map::is_free(int i, int j) const {
    return !this->grid[j][i];
}

bool Map::is_obstacle(int i, int j) const {
    return this->grid[j][i];
}

bool Map::within_map(int i, int j) const {
    return (i >= 0 && j >= 0 && i < this->width && j < this->height);
}

bool Map::is_start(int x, int y) const {
    return (this->agent_starts.find({x, y}) != this->agent_starts.end());
}

bool Map::is_finish(int x, int y) const {
    return (this->agent_finishes.find({x, y}) != this->agent_finishes.end());
}

bool Map::not_assigned(int x, int y) const {
    return (this->agents[this->agent_starts.find({x, y})->second].subgraph == -1);
}

int Map::get_agent(int x, int y) const {
    return this->agent_starts.find({x, y})->second;
}

int Map::get_by_finish(int x, int y) const {
    return this->agent_finishes.find({x, y})->second;
}

void Map::replace_agent(int current_id, int destination_id) {
    int old_x = current_id % this->get_width();
    int old_y = current_id / this->get_width();
    int number = this->get_agent(old_x, old_y);
    this->agent_starts.erase({old_x, old_y});
    int new_x = destination_id % this->get_width();
    int new_y = destination_id / this->get_width();
    this->agents[number].start_x = new_x;
    this->agents[number].start_y = new_y;
    this->agent_starts[{new_x, new_y}] = number;
}

void Map::swap_start_finish() {
    for (int ind = 0; ind < this->number_of_agents; ++ind) {
        std::swap(this->agents[ind].start_x, this->agents[ind].finish_x);
        std::swap(this->agents[ind].start_y, this->agents[ind].finish_y);
    }
    for (auto& agent: this->agents) {
        agent.subgraph = -1;
    }
    std::swap(this->agent_starts, this->agent_finishes);
}

int Map::get_degree(int id) const {
    int node_x = id % this->get_width();
    int node_y = id / this->get_width();
    int counter = 0;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            int new_x = node_x + dx;
            int new_y = node_y + dy;
            if (!this->within_map(new_x, new_y) || !this->is_free(new_x, new_y)) {
                continue;
            }
            if (std::abs(dx) == 1 && std::abs(dy) == 1) {
                if (!this->allow_diagonal) {
                    continue;
                }
                int cnt_near = 0;
                if (this->is_obstacle(new_x, node_y)) {
                    ++cnt_near;
                }
                if (this->is_obstacle(node_x, new_y)) {
                    ++cnt_near;
                }
                if ((cnt_near >= 1 && !this->cut_corners) || 
                    (cnt_near >= 2 && !this->allow_squeeze)) {
                    continue;
                }
            }
            ++counter;
        }
    }
    return counter;
}

std::vector<int> Map::find_successors(int id) const {
    std::vector<int> answer;
    int node_x = id % this->get_width();
    int node_y = id / this->get_width();
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            int new_x = node_x + dx;
            int new_y = node_y + dy;
            if (!this->within_map(new_x, new_y) || !this->is_free(new_x, new_y)) {
                continue;
            }
            if (std::abs(dx) == 1 && std::abs(dy) == 1) {
                if (!this->allow_diagonal) {
                    continue;
                }
                int cnt_near = 0;
                if (this->is_obstacle(new_x, node_y)) {
                    ++cnt_near;
                }
                if (this->is_obstacle(node_x, new_y)) {
                    ++cnt_near;
                }
                if ((cnt_near >= 1 && !this->cut_corners) || 
                    (cnt_near >= 2 && !this->allow_squeeze)) {
                    continue;
                }
            }
            int id = new_y * this->width + new_x;
            answer.push_back(id);
        }
    }
    return answer;
}