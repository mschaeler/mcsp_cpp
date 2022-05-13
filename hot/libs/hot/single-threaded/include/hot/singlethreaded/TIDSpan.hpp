#pragma once

#include <array>
#include <vector>
#include <cstddef>
#include <new>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace hot {
    namespace singlethreaded {


        template<class ValueType, class TIDType>
        constexpr unsigned int determineStackSize() {
            return (hardware_destructive_interference_size - sizeof(ValueType) - sizeof(size_t) -
                    sizeof(std::vector<TIDType>)) /
                   sizeof(TIDType);
        }

//TODO: set semantic, disallow duplicate TIDs
        template<class ValueType, class TIDType, unsigned int StackSize = determineStackSize<ValueType, TIDType>()>
        struct TIDSpan {
            static_assert(StackSize > 0);
            ValueType value;
            size_t _size;
            std::array<TIDType, StackSize> stackValues;
            std::vector<TIDType> furtherValues;

            TIDSpan() = default;

            TIDSpan(const ValueType value, const TIDType tid) : value{value}, _size{1}, stackValues{}, furtherValues{} {
                stackValues[0] = tid;
            }

            TIDType &operator[](const size_t idx) {
                if (idx < StackSize)
                    return stackValues[idx];
                else
                    return furtherValues[idx - StackSize];
            }

            void push_back(TIDType &val) {
                if (_size < StackSize)
                    stackValues[_size] = val;
                else
                    furtherValues.push_back(val);
                ++_size;
            }

            size_t size() const {
                return _size;
            }

            bool empty() const {
                return _size == 0;
            }

            bool contains(const TIDType &tid) {
                return std::find(begin(), end(), tid) == end();
            }

            struct Iterator {
                Iterator(size_t start_pos, TIDSpan *const span) : pos{start_pos}, span{span} {

                }

                explicit Iterator(TIDSpan const *span) : pos{0}, span{span} {

                }

                TIDType &operator*() {
                    return span->operator[](pos);
                }

                Iterator &operator++() {
                    ++pos;
                    return *this;
                }

                Iterator &operator--() {
                    --pos;
                    return *this;
                }

                bool operator==(const Iterator &other) const {
                    return span == other.span && pos == other.pos;
                }

                bool operator!=(const Iterator &other) const {
                    return !operator==(other);
                }

                using iterator_category = std::random_access_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = TIDType;
                using pointer = value_type *;
                using reference = value_type &;

                difference_type operator-(const Iterator &other) const {
                    return other.pos - pos;
                }

                difference_type operator+=(const std::ptrdiff_t size) {
                    return pos += size;
                }

            private:
                size_t pos;
                TIDSpan *const span;
            };

            Iterator begin() {
                return Iterator(0, this);
            }

            Iterator end() {
                return Iterator(_size, this);
            }
        };
    }
}