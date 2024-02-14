#pragma once
#include <list>
#include <stdexcept>
#include <iostream>

class Topology {
private:
    std::list<std::list<int>> container;


public:
    void insert(int id, int parent_id) {
        if (parent_id == -1) {
            std::list<int> new_list;
            new_list.push_back(id);
            container.push_back(new_list);
        } else {
            int list_id = find_list(parent_id);
            if (list_id == -1) {
                throw std::runtime_error("Wrong parent id");
            }
            auto it1 = container.begin();
            std::advance(it1, list_id);
            for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
                if (*it2 == parent_id) {
                    it1->insert(++it2, id);
                    return;
                }
            }
        }
    }

    int find_list(int id) {
        int cur_list_id = 0;
        for (auto it1 = container.begin(); it1 != container.end(); ++it1) {
            for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
                if (*it2 == id) {
                    return cur_list_id;
                }
            }
            ++cur_list_id;
        }
        return -1;
    }

    int get_first_id(int list_id) {
        auto it1 = container.begin();
        std::advance(it1, list_id);
        if (it1->begin() == it1->end()) {
            return -1;
        }
        return *(it1->begin());
    }

    std::list<int> get_nodes() {
        std::list<int> res;
        for (auto& node_list : container) {
            for (auto& node : node_list) {
                res.push_back(node);
            }
        }
        return res;
    }

    friend std::ostream& operator<<(std::ostream& os, const Topology& Topology) {
        for (auto& node_list : Topology.container) {
            os << "{ ";
            for (auto& node : node_list) {
                os << node << " ";
            }
            os << "}" << std::endl;
        }
        return os;
    }
};