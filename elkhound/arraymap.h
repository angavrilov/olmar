// arraymap.h
// template class to maintain an array-based map from
// integers to object pointers; the map owns all of
// the objects referred-to

#ifndef ARRAYMAP_H
#define ARRAYMAP_H

// map: int -> T
template <class T>
class ArrayMap {
private:     // data
  T **map;               // array[0,nextId-1] of owner ptr
  int nextId;            // next id to assign
  int mapSize;           // allocated size of 'map'

public:
  ArrayMap();
  ~ArrayMap();

  // # of elements defined
  int count() const { return nextId; }

  // insert a new element and yield its assigned id
  int insert(T * /*owner*/ t);

  // retrieve by id
  T *lookup(int id);
};

template <class T>
ArrayMap<T>::ArrayMap()
{
  mapSize = 100;      // TODO: reset; just for testing
  nextId = 0;
  map = new T* [mapSize];
}

template <class T>
ArrayMap<T>::~ArrayMap()
{
  loopi(nextId) {
    delete map[i];
  }
  delete[] map;
}

template <class T>
int ArrayMap<T>::insert(T *t)
{
  if (nextId == mapSize) {
    // make it bigger
    int newMapSize = mapSize * 2;
    T **newMap = new T* [newMapSize];

    // copy the old contents to the new map
    loopi(mapSize) {
      newMap[i] = map[i];
    }
    mapSize = newMapSize;

    // blow away the old map
    delete[] map;

    // grab the new map
    map = newMap;
  }
  
  int ret = nextId++;
  map[ret] = t;
  return ret;
}

template <class T>
T *ArrayMap<T>::lookup(int id)
{
  xassert(0 <= id && id < nextId);
  return map[id];
}

#endif // ARRAYMAP_H
