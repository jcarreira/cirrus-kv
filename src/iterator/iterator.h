/* Copyright 2016 Joao Carreira */

#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "src/object_store/FullBladeObjectStore.h"


/*
 * An iterator over the contents of an object store
 *
 *
 */

namespace cirrus {

class StoreIterator {
    int first_id;
    int last_id;
    int current_id;
    cirrus::ostore::FullBladeObjectStoreTempl<> store;

    StoreIterator(int first_id, int, last_id, int current_id,
                          cirrus::ostore::FullBladeObjectStoreTempl<> store) :
        first_id(first_id), last_id(last_id), current_id(current_id),
                            store(store){}
  public:
    StoreIterator(int first_id, int, last_id,
                          cirrus::ostore::FullBladeObjectStoreTempl<> store) :
          first_id(first_id), last_id(last_id), store(store), current_id(0) {}



  int operator*();
  StoreIterator& operator++();
  StoreIterator& begin();
  StoreIterator& end();
  bool operator!=(const StoreIterator& it);
  bool operator==(const StoreIterator& it);
  int get_curr_id();


};


}

#endif // _ITERATOR_H_
