#pragma once

#include <memory>

template<class T, class X, int N>
T& GetInstaenceX() {
  static T v;
  return v;
}

template<class T, class X, int N>
std::shared_ptr<T> GetInstaenceX() {
  static std::shared_ptr<T> v(new T);
  return v;
}


template<class T, class X = void, int N = 0>
class Singleton {
public:
  static T* GetInstance() {
    static T v;
    return &v;
    //return &GetInstanceX<T, X, N>();
  }
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
  static std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> v(new T);
    return v;
    //return GetInstancePtr<T, X, N>();
  }
};
