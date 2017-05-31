/* Copyright 2016 Joao Carreira */

#include "src/iterator/iterator.h"
#include "src/object_store/FullBladeObjectStore.h"

namespace cirrus {


StoreIterator& StoreIterator::begin() {
  return StoreIterator(first_id, last_id, first_id - 1, store);
}

StoreIterator& StoreIterator::end() {
  return StoreIterator(first_id, last_id, last_id + 1, store);
}

int StoreIterator::operator*() {
  int retval;
  return store.get(current_id, &retval);
}

StoreIterator& StoreIterator::operator++() {
  current_id++;
  return *this;
}

bool StoreIterator::operator!=(const StoreIterator& it) {
  return current_id != it.get_curr_id();
}

bool StoreIterator::operator==(const StoreIterator& it) {
  return current_id == it.get_curr_id();
}

int get_curr_id() {
  return current_id;
}

}
