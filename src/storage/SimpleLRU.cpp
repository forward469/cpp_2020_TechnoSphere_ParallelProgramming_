#include "SimpleLRU.h"
#include <cassert>

namespace Afina {
namespace Backend {


//
void SimpleLRU::SimplePut(const std::string &key, const std::string &value) {
    Clear(key.size() + value.size());

    auto new_node = std::unique_ptr<lru_node>(new lru_node(key, value));

    _lru_index.insert(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
    _current_size += key.size() + value.size();

    if (!_lru_head) {
        new_node->prev = new_node.get();
        new_node->next.reset();
        _lru_head = std::move(new_node);
    } else {
        new_node->prev = _lru_head->prev;
        new_node->next.reset();
        _lru_head->prev->next = std::move(new_node);
        _lru_head->prev = _lru_head->prev->next.get();
    }
}

void SimpleLRU::PutToTail(std::unique_ptr<lru_node> &&node) {
    if (!node->next){
      if (!_lru_head) {
        _lru_head = std::move(node);
      }
      else {
        node->prev->next = std::move(node);
      }
    }

    else if(!_lru_head) {
      _lru_head = std::move(node->next);
      auto tail = node->prev;
      tail->next = std::move(node);
      _lru_head->prev->next.reset();
    }
    else {
      node->next->prev = node->prev;
      node->prev->next = std::move(node->next);
      auto tail = _lru_head->prev;
      _lru_head->prev = node.get();
      node->prev = tail;
      tail->next = std::move(node);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }

    if (!Set(key, value)) {
        SimplePut(key, value);
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }
    if (_lru_index.find(std::ref(key)) == _lru_index.end()) {
        SimplePut(key, value);
        return true;
    }
    return false;
 }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }

    auto FindIter = _lru_index.find(std::ref(key));
    if (FindIter == _lru_index.end()) {
        return false;
    }

    auto &node = FindIter->second.get();
    auto ptr = std::move((&node == _lru_head.get()) ? _lru_head : node.prev->next);
    PutToTail(std::move(ptr));
    std::size_t to_free = value.size() > node.value.size() ? value.size() - node.value.size() : 0;
    Clear(to_free);
    _current_size += to_free;
    node.value = value;
    return true;
 }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto FindIter = _lru_index.find(std::ref(key));
    if (FindIter != _lru_index.end()) {
        auto &node = FindIter->second.get();
        _lru_index.erase(FindIter);
        _current_size -= node.key.size() + node.value.size();

        if (node.prev == &node) {
            // only head exists
            _lru_head.reset();
        } else {
            if (&node == _lru_head.get()) {
                // deleting head
                node.next->prev = node.prev;
                _lru_head = std::move(node.next);
            } else if (!node.next) {
                // deleting tail
                _lru_head->prev = node.prev;
                node.prev->next.reset();
            } else {
                node.next->prev = node.prev;
                node.prev->next = std::move(node.next);
            }
        }

        return true;
    }
    return false;
}

bool SimpleLRU::Clear(std::size_t size) {
    if (size > _max_size) {
        return false;
    }
    while (_current_size + size > _max_size) {
       Delete(_lru_head->key);
    }
    return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto FindIter = _lru_index.find(std::ref(key));
    if (FindIter == _lru_index.end()) {
        return false;
    } else {
        auto &node = FindIter->second.get();
        auto ptr = std::move((&node == _lru_head.get()) ? _lru_head : node.prev->next);
        PutToTail(std::move(ptr));
        value = node.value;
        return true;
    }
}

} // namespace Backend
} // namespace Afina
