#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <random>
#include <string_view>

#include <sqlite3.h>

namespace detail::util {
    std::mt19937& GetRandomGenerator();
    size_t getRandomIndex(size_t max);

    //    std::string convertWstringToUtf8String(const std::wstring& str);
    //    std::wstring convertUtf8ToWString(const std::string& str);

    template <typename _RandomAccessIter>
    void shuffleQuestions(_RandomAccessIter begin, _RandomAccessIter end) {
        std::shuffle(begin, end, GetRandomGenerator());
    }

    template <typename _ForwardIterator>
    decltype(auto) getRandomIterator(_ForwardIterator begin,
                                     _ForwardIterator end) {
        assert(begin < end);
        const size_t dist = std::distance(begin, end);
        const auto rand = getRandomIndex(dist);
        std::advance(begin, rand);
        return begin;
    }

    template <typename T> T getRandomValue(const T& min, const T& max) {
        assert(min < max);
        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> dis(min, max);
            return dis(GetRandomGenerator());
        } else if constexpr (std::is_floating_point_v<T>) {
            std::uniform_real_distribution<T> dis(min, max);
            return dis(GetRandomGenerator());
        } else
            assert(false);
    }

    template <typename _Container>
    decltype(auto) getRandomIterator(_Container& container) {
        return getRandomIterator(std::begin(container), std::end(container));
    }

    template <typename _Container>
    std::wstring combineWStringContainerToWstring(const _Container& container) {
        std::wstring res;
        const std::wstring_view separator = L"; ";
        for (const auto& e : container) {
            res.insert(res.end(), std::begin(e), std::end(e));
            res.insert(res.end(), separator.begin(), separator.end());
        }
        assert(res.size() > separator.size());
        res.resize(res.size() - separator.size());
        return res;
    }

    struct Sqlite3OpenCloseHelper {
        Sqlite3OpenCloseHelper(std::string_view filepath) {
            sqlite3* ptr = nullptr;
            const auto res = sqlite3_open(filepath.data(), &ptr);
            this->m_Db.reset(ptr);
            if (res)
                this->m_Db = nullptr;
        }

        operator bool() const { return bool(this->m_Db); }
        operator sqlite3*() { return this->m_Db.get(); }
        operator const sqlite3*() const { return this->m_Db.get(); }

        ~Sqlite3OpenCloseHelper() {}

      private:
        struct Deleter {
            void operator()(sqlite3* toDelete) const {
                (void)sqlite3_close(toDelete);
            }
        };
        std::unique_ptr<sqlite3, Deleter> m_Db;
    };

} // namespace detail::util
