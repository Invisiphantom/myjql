#include <iostream>
#include <vector>
#include <set>
#include <map>
using namespace std;

typedef long my_off_t;

vector<set<my_off_t>> v;
map<my_off_t, short> m;

extern "C" void m_init(size_t size) {
    v.clear();
    m.clear();
    v.resize(size);
}

extern "C" void m_add(short size, my_off_t id) {
    if (v[size].count(id)) {
        std::terminate();
    }
    if (m.count(id)) {
        std::terminate();
    }
    v[size].insert(id);
    m[id] = size;
}

extern "C" void m_erase(short size, my_off_t id) {
    if (!v[size].count(id)) {
        std::terminate();
    }
    if (!m.count(id)) {
        std::terminate();
    }
    v[size].erase(id);
    m.erase(id);
}

extern "C" int m_contain(short size, my_off_t id) {
    return v[size].count(id) != 0;
}

extern "C" int m_empty(short size) {
    return v[size].empty();
}

extern "C" short m_get_size(my_off_t id) {
    if (!m.count(id)) {
        std::terminate();
    }
    return m[id];
}

extern "C" size_t m_get_total() {
    return m.size();
}

extern "C" my_off_t m_get_item(int idx) {
    auto it = m.begin();
    for (int i = 0; i < idx; ++i) {
        ++it;
    }
    return it->first;
}