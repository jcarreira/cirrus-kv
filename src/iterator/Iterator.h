/* Copyright 2016 Joao Carreira */

#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "src/cache_manager/CacheManager.h"


/*
 * An iterator over the contents of an object store
 *
 *
 */

namespace cirrus {

class StoreIterator {
  public:
    StoreIterator(int first_id, int last_id, int current_id,
                          cirrus::CacheManager *cm) :
        first_id(first_id), last_id(last_id), current_id(current_id),
                            cm(cm){}
    int first_id;
    int last_id;
    int current_id;
    cirrus::CacheManager *cm;

    int& operator*();
    StoreIterator& operator++();
    StoreIterator& operator++(int i); 
    StoreIterator begin() const;
    StoreIterator end() const;
    bool operator!=(const StoreIterator& it) const;
    bool operator==(const StoreIterator& it) const;
    int get_curr_id() const;


};


}

#endif // _ITERATOR_H_
