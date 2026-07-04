#pragma once

#include <cstdint>
#include <iterator>

namespace cyberfluids::backend {

/// A minimal random-access iterator over an integer index range. Lets the
/// parallel STL algorithms iterate `[0, n)` without materializing an index
/// buffer. Dependency-free (no third-party ranges library).
struct CountingIterator {
    using value_type = std::int64_t;
    using difference_type = std::int64_t;
    using reference = std::int64_t;
    using pointer = void;
    using iterator_category = std::random_access_iterator_tag;

    std::int64_t i = 0;

    std::int64_t operator*() const { return i; }
    std::int64_t operator[](difference_type n) const { return i + n; }

    CountingIterator& operator++() { ++i; return *this; }
    CountingIterator operator++(int) { auto t = *this; ++i; return t; }
    CountingIterator& operator--() { --i; return *this; }
    CountingIterator operator--(int) { auto t = *this; --i; return t; }
    CountingIterator& operator+=(difference_type n) { i += n; return *this; }
    CountingIterator& operator-=(difference_type n) { i -= n; return *this; }

    friend CountingIterator operator+(CountingIterator a, difference_type n) { a += n; return a; }
    friend CountingIterator operator+(difference_type n, CountingIterator a) { a += n; return a; }
    friend CountingIterator operator-(CountingIterator a, difference_type n) { a -= n; return a; }
    friend difference_type operator-(CountingIterator a, CountingIterator b) { return a.i - b.i; }

    friend bool operator==(CountingIterator a, CountingIterator b) { return a.i == b.i; }
    friend bool operator!=(CountingIterator a, CountingIterator b) { return a.i != b.i; }
    friend bool operator<(CountingIterator a, CountingIterator b) { return a.i < b.i; }
    friend bool operator>(CountingIterator a, CountingIterator b) { return a.i > b.i; }
    friend bool operator<=(CountingIterator a, CountingIterator b) { return a.i <= b.i; }
    friend bool operator>=(CountingIterator a, CountingIterator b) { return a.i >= b.i; }
};

}  // namespace cyberfluids::backend
