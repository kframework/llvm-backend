#include "runtime/header.h"

#include "immer/flex_vector_transient.hpp"

extern "C" {
  mapiter map_iterator(map *map) {
    return mapiter{map->begin(), map};
  }

  block *map_iterator_next(mapiter *iter) {
    if (iter->curr == iter->m->end()) {
      return nullptr;
    }
    return (iter->curr++)->first;
  }

  map hook_MAP_element(SortKItem key, SortKItem value) {
    return map().set(key, value);
  }

  map hook_MAP_unit() {
    return map();
  }

  map hook_MAP_concat(SortMap m1, SortMap m2) {
    auto from = m1->size() < m2->size() ? m1 : m2;
    auto to = m1->size() < m2->size() ? *m2 : *m1;
    for (auto iter = from->begin(); iter != from->end(); ++iter) {
      auto entry = *iter;
	if (to.find(entry.first)) {
        throw std::invalid_argument("Duplicate keys");
	}
      to = to.insert(*iter);
    }
    return to;
  }

  SortKItem hook_MAP_lookup_null(SortMap m, SortKItem key) {
    if (auto val = m->find(key)) {
      return *val;
    }
    return nullptr;
  }

  SortKItem hook_MAP_lookup(SortMap m, SortKItem key) {
    auto res = hook_MAP_lookup_null(m, key);
    if(!res) {
      throw std::invalid_argument("key not found");
    }
    return res;
  }

  SortKItem hook_MAP_lookupOrDefault(SortMap m, SortKItem key, SortKItem _default) {
    auto res = hook_MAP_lookup_null(m, key);
    if (!res) {
      return _default;
    }
    return res;
  }

  map hook_MAP_update(SortMap m, SortKItem key, SortKItem value) {
    return m->set(key, value);
  }

  map hook_MAP_remove(SortMap m, SortKItem key) {
    return m->erase(key);
  }

  map hook_MAP_difference(SortMap m1, SortMap m2) {
    auto from = m2;
    auto to = *m1;
    for (auto iter = from->begin(); iter != from->end(); ++iter) {
      auto entry = *iter;
	if (auto value = to.find(entry.first)) {
        if (*value == entry.second) {
          to = to.erase(entry.first);
	  }
	}
    }
    return to;
  }

  set hook_SET_unit(void);
  set hook_SET_element(block *);
  set hook_SET_concat(set *, set *);

  set hook_MAP_keys(SortMap m) {
    auto tmp = hook_SET_unit();
    for (auto iter = m->begin(); iter != m->end(); ++iter) {
      auto elem = hook_SET_element(iter->first);
      tmp = hook_SET_concat(&tmp, &elem);
    }
    return tmp;
  }

  list hook_MAP_keys_list(SortMap m) {
    auto tmp = list().transient();
    for (auto iter = m->begin(); iter != m->end(); ++iter) {
      tmp.push_back(iter->first);
    }
    return tmp.persistent();
  }

  bool hook_MAP_in_keys(SortKItem key, SortMap m) {
    return m->find(key);
  }

  list hook_MAP_values(SortMap m) {
    auto tmp = list().transient();
    for (auto iter = m->begin(); iter != m->end(); ++iter) {
      tmp.push_back(iter->second);
    }
    return tmp.persistent();
  }

  SortKItem hook_MAP_choice(SortMap m) {
    if (m->empty()) {
      throw std::invalid_argument("Map is empty");
    }
    return m->begin()->first;
  }

  size_t hook_MAP_size_long(SortMap m) {
    return m->size();
  }

  SortInt hook_MAP_size(SortMap m) {
    auto size = hook_MAP_size_long(m);
    mpz_t result;
    mpz_init_set_ui(result, size);
    return move_int(result);
  }

  bool hook_MAP_inclusion(SortMap m1, SortMap m2) {
    for (auto iter = m1->begin(); iter != m1->end(); ++iter) {
      auto entry = *iter;
      auto val = m2->find(entry.first);
      if (!val || *val != entry.second) {
        return false;
      }
    }
    return true;
  }

  map hook_MAP_updateAll(SortMap m1, SortMap m2) {
    auto from = m2;
    auto to = *m1;
    for (auto iter = from->begin(); iter != from->end(); ++iter) {
      to = to.insert(*iter);
    }
    return to;
  }

  map hook_MAP_removeAll(SortMap map, SortSet set) {
    auto tmp = *map;
    for (auto iter = set->begin(); iter != set->end(); ++iter) {
      tmp = tmp.erase(*iter);
    }
    return tmp;
  }

  bool hook_MAP_eq(SortMap m1, SortMap m2) {
    return (*m1) == (*m2);
  }

  void map_hash(map *m, void *hasher) {
    if (hash_enter()) {
      for (auto iter = m->begin(); iter != m->end(); ++iter) {
        auto entry = *iter;
        k_hash(entry.first, hasher);
        k_hash(entry.second, hasher);
      }
    }
    hash_exit();
  }

  void map_foreach(map *map, void(process)(block **)) {
    for (auto iter = map->begin(); iter != map->end(); ++iter) {
      process((block **)&iter->first);
      process((block **)&iter->second);
    }
  }

  map map_map(map *map, block *(process)(block *)) {
    auto tmp = *map;
    for (auto iter = map->begin(); iter != map->end(); ++iter) {
      auto entry = *iter;
      tmp = tmp.set(entry.first, process(entry.second));
    }
    return tmp;
  }

  void printMap(writer *file, map *map, const char *unit, const char *element, const char *concat) {
    size_t size = map->size();
    if (size == 0) {
      sfprintf(file, "%s()", unit);
      return;
    }

    sfprintf(file, "\\left-assoc{}(%s(", concat); 

    bool once = true;
    for (auto iter = map->begin(); iter != map->end(); ++iter) {
      if (once) {
        once = false;
      } else {
        sfprintf(file, ",");
      }

      sfprintf(file, "%s(", element);
      auto entry = *iter;
      printConfigurationInternal(file, entry.first, "SortKItem{}", false);
      sfprintf(file, ",");
      printConfigurationInternal(file, entry.second, "SortKItem{}", false);
      sfprintf(file, ")");
    }
    sfprintf(file, "))");
  }
}
