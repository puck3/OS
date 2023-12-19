#include <list>
#include <iostream>

class Topology {
private:
    std::list<std::list<int>> nodes;

public:
    auto find_it(int id) {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
                if (*it2 == id) {
                    return std::make_pair(it, it2);
                }
            }
        }
        return std::make_pair(nodes.end(), nodes.back().end());
    }

    void insert(int id, int parent_id) {
        if (parent_id == -1) {
            std::list<int> new_list;
            new_list.push_back(id);
            nodes.push_back(new_list);
        } else {
            auto [parent_list_it, parent_it] = find_it(parent_id);
            parent_list_it->insert(++parent_it, id);
        }
    }

    void throw_if_new_node_invalid(int id, int parent_id) {
        auto [list_it, it] = find_it(id);
        auto [parent_list_it, parent_it] = find_it(parent_id);
        if (id == -1) {
            throw std::runtime_error("Invalid node id");
        }
        if (list_it != nodes.end()) {
            throw std::runtime_error("Already exist");
        }
        if (parent_id != -1 && parent_list_it == nodes.end()) {
            throw std::runtime_error("Parent not found");
        }
    }

    void erase(int id) {
        auto [list_it, it] = find_it(id);
        list_it->erase(it, list_it->end());
        if (list_it->empty()) {
            nodes.erase(list_it);
        }
    }

    int find(int id) {
        int list_idx;
        auto [list_it, elem_it] = find_it(id);
        if (list_it == nodes.end()) list_idx = -1;
        else list_idx = std::distance(nodes.begin(), list_it);
        return list_idx;
    }

    int get_first_id(int list_idx) {
        auto it = nodes.begin();
        std::advance(it, list_idx);
        if (it->begin() == it->end()) {
            return -1;
        }
        return *(it->begin());
    }

    friend std::ostream& operator<<(std::ostream& os, const Topology& topology) {
        for (auto& list_it : topology.nodes) {
            os << "{ ";
            for (auto& it : list_it) {
                os << it << " ";
            }
            os << "}" << std::endl;
        }
        return os;
    }
};