#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <stdexcept>
#include <iostream>

template <typename T>
class CircularBuffer
{
  public:
    typedef size_t size_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* pointer;
    typedef const T* const_pointer;

    explicit CircularBuffer(size_type capacity);
    CircularBuffer(const CircularBuffer<T> &rhs);
    CircularBuffer(CircularBuffer<T>&& rhs);
    ~CircularBuffer() { if (_buffer) delete[] _buffer; }

    CircularBuffer<T>& operator=(CircularBuffer<T> rhs);

    size_type size() const { return (_full ? _capacity : _front); }
    size_type capacity() const { return _capacity; }
    bool is_full() const { return _full; }

    const_reference operator[](size_type index) const;
    reference operator[](size_type index);

    void add(T item);
    void resize(size_type new_capacity);

    friend void swap(CircularBuffer<T> &a, CircularBuffer<T> &b)
    {
      std::swap(a._buffer, b._buffer);
      std::swap(a._capacity, b._capacity);
      std::swap(a._front, b._front);
      std::swap(a._full, b._full);
    }

  private:
    pointer _buffer;
    size_type _capacity;
    size_type _front;
    bool _full;

    CircularBuffer();
};

template<typename T>
CircularBuffer<T>::CircularBuffer()
: _buffer(nullptr)
, _capacity(0)
, _front(0)
  , _full(false)
{
}

template<typename T>
CircularBuffer<T>::CircularBuffer(size_type capacity)
  : CircularBuffer()
{
  if (capacity < 1) throw std::length_error("Invalid capacity");

  _buffer = new T[capacity];
  _capacity = capacity;
}

template<typename T>
CircularBuffer<T>::CircularBuffer(const CircularBuffer<T> &rhs)
: _buffer(new T[rhs._capacity])
, _capacity(rhs._capacity)
, _front(rhs._front)
  , _full(rhs._full)
{
  std::copy(rhs._buffer, rhs._buffer + _capacity, _buffer);
}

template<typename T>
CircularBuffer<T>::CircularBuffer(CircularBuffer<T>&& rhs)
  : CircularBuffer()
{
  swap(*this, rhs);
}

template<typename T>
typename CircularBuffer<T>::const_reference
CircularBuffer<T>::operator[](size_type index) const
{
  static const std::out_of_range ex("index out of range");
  if (index < 0) throw ex;

  if (_full)
  {
    if (index >= _capacity) throw ex;
    return _buffer[(_front + index) % _capacity];
  }
  else
  {
    if (index >= _front) throw ex;
    return _buffer[index];
  }
}

template<typename T>
typename CircularBuffer<T>::reference 
CircularBuffer<T>::operator[](size_type index)
{
  return const_cast<reference>(static_cast<const CircularBuffer<T>&>(*this)[index]);
}

template<typename T>
CircularBuffer<T>& 
CircularBuffer<T>::operator=(CircularBuffer<T> rhs)
{
  swap(*this, rhs);
  return *this;
}

template<typename T>
void 
CircularBuffer<T>::add(T item)
{
  _buffer[_front++] = item;
  if (_front == _capacity) {
    _front = 0;
    _full = true;
  }
}

template<typename T>
void 
CircularBuffer<T>::resize(size_type new_capacity)
{
  if (new_capacity < 1) throw std::length_error("Invalid capacity");
  if (new_capacity == _capacity) return;

  size_type num_items = size();
  size_type offset = 0;
  if (num_items > new_capacity)
  {
    offset = num_items - new_capacity;
    num_items = new_capacity;
  }

  pointer new_buffer = new T[new_capacity];
  for (size_type item_no = 0; item_no < num_items; ++item_no)
  {
    new_buffer[item_no] = (*this)[item_no + offset];
  }

  pointer old_buffer = _buffer;

  _buffer = new_buffer;
  _capacity = new_capacity;
  _front = (num_items % _capacity);
  _full = (num_items == _capacity);

  delete[] old_buffer;
}

#endif // CIRCULAR_BUFFER_H
