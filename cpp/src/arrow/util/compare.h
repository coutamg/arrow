// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "arrow/util/macros.h"

namespace arrow {
namespace util {

/// CRTP helper for declaring equality comparison. Defines operator== and operator!=
/*
 * CRTP（Curiously Recurring Template Pattern）是一种常用的C++模板编程技巧，它允许通过
 * 基类模板参数来实现对派生类的递归。这种模式在多种场合下都非常有用，比如实现类型安全的访问
 * 者模式、多态基类等。
 *
 * 一个 case：
 * // 基类模板
 * template <typename T>
 * struct EqualityComparable {
 *     // 静态比较函数，使用SFINAE来确保只有当T具有operator==时才可用
 *     static bool isEqual(const T& lhs, const T& rhs) {
 *         return lhs == rhs;
 *     }
 * };
 * 
 * // 特化版本，用于不支持operator==的类型
 * template <typename T>
 * struct EqualityComparable<T,
 *        typename std::enable_if<!std::is_arithmetic<T>::value>::type> {
 *     static bool isEqual(const T& lhs, const T& rhs) {
 *         std::cerr << "Equality comparison not supported for this type." << std::endl;
 *         return false;
 *     }
 * };
 * 
 * // CRTP辅助类
 * template <typename Derived>
 * struct Base {
 *     bool operator==(const Derived& other) const {
 *         return EqualityComparable<Derived>::isEqual(*static_cast<const Derived*>(this), other);
 *     }
 * };
 * 
 * // 使用CRTP的派生类
 * struct MyType : Base<MyType> {
 *     int value;
 * 
 *     MyType(int val) : value(val) {}
 * };
 * 
 * int main() {
 *     MyType a(10);
 *     MyType b(20);
 * 
 *     std::cout << "a == b: " << (a == b ? "true" : "false") << std::endl;
 *     return 0;
 * }
 * EqualityComparable 是一个基类模板，提供了一个静态成员函数 isEqual。
 * 为不支持算术类型（即不支持 operator==）的类型提供了一个特化版本。
 * Base 是一个使用CRTP的基类，它将派生类类型 Derived 作为模板参数，并提供了 operator== 的实现。
 * MyType 是一个派生类，它继承自 Base，并重载了 operator==。
 * 这种方法允许你在基类中定义比较逻辑，同时在派生类中重用这些逻辑，同时保持类型安全。通过这种
 * 方式，你可以很容易地为不同的类型添加或修改比较行为。
 * */
template <typename T>
class EqualityComparable {
 public:
  ~EqualityComparable() {
    static_assert(
        /*
         * 这段代码使用了C++的类型特质来检查类型T的Equals方法是否返回bool类型。
         * std::declval<T>(): 这个函数生成一个假想的（不会在编译时或运行时实际创建）T类型
         * 的临时对象。这使得我们可以在不实例化T的情况下，对T类型进行操作和检查。
         *
         * const T().Equals(const T()): 假设T类型有一个名为Equals的方法，这里尝试调用
         * 这个方法。const关键字确保我们不会修改任何东西，这对于只读操作是必要的。
         *
         * decltype(...): 这个关键字用于确定表达式的类型。在这里，它确定了Equals方法的返回类型。
         *
         * std::is_same<A, B>::value: 这是一个类型特质，用来检查两个类型A和B是否相同。如果相
         * 同，value为true（即1），否则为false（即0）。
         * 
         * 整体来看，这段代码检查Equals方法的返回类型是否为bool。如果Equals返回的是bool，那么
         * std::is_same的结果就是true，否则为false。
         */
        std::is_same<decltype(std::declval<const T>().Equals(std::declval<const T>())),
                     bool>::value,
        "EqualityComparable depends on the method T::Equals(const T&) const");
  }

  // 在基类中定义比较逻辑，同时在派生类中重用这些逻辑，同时保持类型安全
  template <typename... Extra>
  bool Equals(const std::shared_ptr<T>& other, Extra&&... extra) const {
    if (other == NULLPTR) {
      return false;
    }
    return cast().Equals(*other, std::forward<Extra>(extra)...);
  }

  struct PtrsEqual {
    bool operator()(const std::shared_ptr<T>& l, const std::shared_ptr<T>& r) const {
      return l->Equals(*r);
    }
  };

  friend bool operator==(T const& a, T const& b) { return a.Equals(b); }
  friend bool operator!=(T const& a, T const& b) { return !(a == b); }

 private:
  const T& cast() const { return static_cast<const T&>(*this); }
};

}  // namespace util
}  // namespace arrow
