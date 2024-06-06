#include <iostream>
#include <string>
#include <map>
using namespace std;

typedef long my_off_t;

typedef struct {
    my_off_t addr;
    short idx;
} my_RID;

map<pair<my_off_t, short>, string> m;

extern "C" void m_init() {
    m.clear();
}

extern "C" void m_insert(my_RID rid, char* s) {
    pair<my_off_t, short> p{rid.addr, rid.idx};
    if (m.count(p)) {
        std::terminate();
    }
    m[p] = s;
}

extern "C" void m_erase(my_RID rid) {
    pair<my_off_t, short> p{rid.addr, rid.idx};
    if (!m.count(p)) {
        std::terminate();
    }
    m.erase(p);
}

extern "C" size_t m_get_total() {
    return m.size();
}

extern "C" my_RID m_get_rid(int idx) {
    auto it = m.begin();
    for (int i = 0; i < idx; i++) {
        ++it;
    }
    auto p = it->first;
    return {p.first, p.second};
}

extern "C" int m_equal(my_RID rid, char* s) {
    pair<my_off_t, short> p{rid.addr, rid.idx};
    if (!m.count(p))
        std::terminate();
    return m[p] == s;
}