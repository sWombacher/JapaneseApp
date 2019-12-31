#include "detail/util.hpp"

namespace detail::util {

size_t getRandomIndex(size_t max) {
    assert(max >= 1);
    std::uniform_int_distribution<size_t> dis(0, max - 1);
    return dis(GetRandomGenerator());
}

std::mt19937& GetRandomGenerator() {
    static std::mt19937 mersenne((std::random_device())());
    return mersenne;
}

}
