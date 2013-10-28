//#####################################################################
// Class NestedField
//#####################################################################
//
// A wrapper around Nested for accessing with ids
//
//#####################################################################
#pragma once

#include <othercore/array/Nested.h>
#include <othercore/array/RawField.h>

namespace other {

template<class T, class Id>
class NestedField {
 public:
  Nested<T> raw;

  NestedField(Nested<T>&& _raw) : raw(_raw) {}
  NestedField(RawField<const int,Id> lengths, bool initialize=true) : raw(lengths.flat, initialize) {}
  NestedField(const Field<const int, Id> offsets, const Array<T>& flat) : raw(offsets.flat, flat) {}

  template<class S,class Id2> static NestedField empty_like(const NestedField<S,Id2>& other) {
    return NestedField(Nested<T>::empty_like(other.raw));
  }

  template<class S,bool f> static NestedField empty_like(const Nested<S,f>& other) {
    return NestedField(Nested<T>::empty_like(other));
  }

  int size() const { return raw.size(); }
  int size(const Id i) const { return raw.size(i.idx()); }
  bool empty() const { return raw.empty(); }
  bool valid(const Id i) const { return raw.valid(i.idx()); }
  int total_size() const { return raw.total_size(); }
  Array<int> sizes() const { return raw.sizes(); }
  T& operator()(const Id i, const int j) const { return raw(i.idx(),j); }
  decltype(raw[0]) operator[](const Id i) const { return raw[i.idx()]; }

  // return index into raw.flat for (*this)[i].front()
  int front_offset(const Id i) const { return raw.offsets[i.idx()]; }
  // return index into raw.flat for (*this)[i].back()
  int back_offset(const Id i) const { return raw.offsets[i.idx()+1]-1; }

};

} // namespace other