#include <map>
#include <exception>

using namespace std;

typedef long my_off_t;

map<my_off_t, short> m;

extern "C" void m_init() {
    m.clear();
}

extern "C" void m_insert(my_off_t addr, short idx) {
    if (m.count(addr))
        std::terminate();
    m[addr] = idx;
}

extern "C" void m_erase(my_off_t addr) {
    if (!m.count(addr))
        std::terminate();
    m.erase(addr);
}

extern "C" short m_get_idx(my_off_t addr) {
    if (!m.count(addr))
        std::terminate();
    return m[addr];
}

extern "C" int m_contain(my_off_t addr) {
    return m.count(addr) != 0;
}

extern "C" size_t m_get_total() {
    return m.size();
}

extern "C" my_off_t m_get_addr(int idx) {
    auto it = m.begin();
    for (int i = 0; i < idx; i++)
        it++;
    return it->first;
}