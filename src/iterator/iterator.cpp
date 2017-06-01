/* Copyright 2016 Joao Carreira */

#include "src/iterator/Iterator.h"
#include "src/cache_manager/cache_manager.h"

namespace cirrus {


StoreIterator StoreIterator::begin() const {
  return StoreIterator(first_id, last_id, first_id, store);
}

StoreIterator StoreIterator::end() const {
  return StoreIterator(first_id, last_id, last_id + 1, store);
}

int* StoreIterator::operator*() {
  if (current_id < last_id) {
    cm->prefetch(current_id + 1);
  }
  return cm->get(current_id);
}

StoreIterator& StoreIterator::operator++() {
  current_id++;
  return *this;
}

StoreIterator& StoreIterator::operator++(int /*i */) {
  current_id ++;
  return *this;
}

bool StoreIterator::operator!=(const StoreIterator& it) const {
  return current_id != it.get_curr_id();
}

bool StoreIterator::operator==(const StoreIterator& it) const {
  return current_id == it.get_curr_id();
}

int StoreIterator::get_curr_id() const {
  return current_id;
}

}
