# CielLab

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

This library is header-only and requires at least C++11. It's only guaranteed to be compatible with GCC and Clang.

## Installation

Download the entire repository into your project directory, such as a _third_party_ folder. Then, add the following lines to your _CMakeLists.txt_:

```cmake
target_link_libraries(${PROJECT_NAME} ciellab)

add_subdirectory(third_party/ciellab)
```

## Utilities

### vector.hpp

`ciel::vector` is similar to `std::vector`, with the following differences:

#### 1. We don't provide a specialization of `vector` for `bool`.

#### 2. We don't support incomplete types, since we need to verify whether T is trivially copyable throughout the entire class.

#### 3. Proposal P1144 supported. We provide an `is_trivially_relocatable` trait, which defaults to `std::is_trivially_copyable`. You can partially specialize this trait for specific classes. When it comes to operations like expansions, insertions, and deletions, we will use `memcpy` or `memmove` for trivially relocatable objects.

Most types in C++ are trivially relocatable, except for those that have self references.

```cpp
#include <memory>
#include <ciel/vector>

template<class T>
struct ciel::is_trivially_relocatable<std::unique_ptr<T>> : std::true_type {};
template<class T>
struct ciel::is_trivially_relocatable<std::shared_ptr<T>> : std::true_type {};

struct NotRelocatable {
    unsigned char buffer_[8]{};
    void* p = buffer_; // Points to a member variable, so it can't be trivially relocated.
};
template<>
struct ciel::is_trivially_relocatable<NotRelocatable> : std::false_type {};
```

Sadly, we currently lack any methods to check if a type is trivially relocatable.

Regarding expansions, when a type is trivially relocatable, we can use `memcpy` instead of move/copy constructing it at the new location and destructing it at the old location. This approach allows for a "destructible move," eliminating the need for constructions and destructions.

#### 4. Provide `unchecked_emplace_back`, it serves as an `emplace_back` operation that does not check for available space, under the assumption that the container has sufficient capacity. It's crucial to call `reserve` in advance to allocate the necessary memory.

```cpp
#include <ciel/vector>

ciel::vector<int> v;
v.reserve(100);
for (int i = 0; i < 100; ++i) {
    v.unchecked_emplace_back(i);
}
```

#### 5. Add `std::initializer_list` overloads for `emplace` and `emplace_back`.

```cpp
ciel::vector<ciel::vector<int>> v;
v.emplace_back({1, 2});
v.emplace(v.end(), {3, 4});
```

Since `{...}` has no type and can't be deduced to `std::initializer_list` by the compiler, the default `emplace` and `emplace_back` in `std::vector` do not support this functionality directly without providing these overloads.

#### 6. LWG 526 fully supported.

For the following functions, it is safe when the element is from the same vector.

```cpp
v.assign(5, v[0]);
v.insert(v.begin(), 2, v[0]);
v.insert(v.begin(), v[0]);
v.insert(v.begin(), std::move(v[0]));
v.emplace(v.begin(), v[0]);
v.emplace(v.begin(), std::move(v[0]));
v.emplace_back(v[0]);
v.emplace_back(std::move(v[0]));
v.push_back(v[0]);
v.push_back(std::move(v[0]));
v.resize(5, v[0]);
```

#### 7. Move every element in the range if the range type is rvalue.

For the following functions, ciel::vector will perform move insertions.

```cpp
ciel::vector<T> v(ciel::from_range, std::move(rg));
v.assign_range(std::move(rg));
v.insert_range(std::move(rg));
v.append_range(std::move(rg));
```

#### 8. Can preallocate sufficient space for member variable initializations.

```cpp
ciel::vector<T> v;
v.reserve(N);
```

equals to

```cpp
ciel::vector<T> v{reserve_capacity, N};
```

It can be used in member variable initializations.

The behavior is undefined if N == 0.

### function.hpp

`ciel::function` is an enhancement of `std::function` that adds optimization for trivially relocatable objects.

It internally contains a stack buffer whose size is equal to `sizeof(void*) * 3` and whose alignment is equal to `alignof(void*)`, which stores the callable object in the buffer only if its size and alignment do not exceed the buffer's and it's trivially relocatable; otherwise, it will be allocated on the heap.

As a result, it guarantees that it is nothrow_movable and trivially_relocatable.

However, under the existing framework, determining whether a type is trivially_relocatable is still quite difficult. For example, even though `std::vector<int>` is trivially_relocatable in most implementations, and we can make functions aware of it through template specialization, however, for a lambda that captures `std::vector<int>`, even though it is also trivially_relocatable, we cannot discover and utilize its properties.

Therefore, we provide an overloaded version of the constructor that takes the first parameter as `ciel::assume_trivially_relocatable` tag. This way, we will treat callable objects as trivially_relocatable. Similarly, for assignment, we also provide an overloaded version of `assign(ciel::assume_trivially_relocatable_t, F&&)`.

```cpp
std::vector<int> v{1, 2, 3};
// allocated on the heap since
// ciel::is_trivially_relocatable<decltype([v] { (void)v; })>::value == false
ciel::function<void()> f1{[v] { (void)v; }};
// allocated on the stack buffer
ciel::function<void()> f2{ciel::assume_trivially_relocatable, [v] { (void)v; }};
```

### TODO: other .hpp

## Benchmark

You can find the benchmarks in the GitHub workflows section.

Note that benchmark results may vary between different compilers and compiler options. Compiler optimizations and resulting code generation may coincidentally favor one implementation over another, even when they appear visually identical.

## Usage of Third Party Libraries

This library has no dependencies of its own.

We utilize GoogleTest for unit testing and Google Benchmark for benchmarking and comparisons.

Please refer to the [ThirdPartyNotices.txt](./ThirdPartyNotices.txt) file for details regarding the licensing of GoogleTest and Google Benchmark.
