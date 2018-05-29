#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <vector>
#include <algorithm>
#include <random>

template<typename ReturnElement, typename Element, class ExtractInfo>
std::vector<ReturnElement> getRandom(const std::vector<Element> &elements, size_t limit, size_t count, const ExtractInfo &extracter) {
    CHECK(count <= limit, "Incorrect count value");

    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<unsigned int> indices(std::min(limit, elements.size()));
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), g);

    std::vector<ReturnElement> result;
    for (size_t i = 0; i < count; i++) {
        result.emplace_back(extracter(elements[indices[i]]));
    }
    return result;
}

template<typename Element>
Element getRandom(const std::vector<Element> &elements) {
    return ::getRandom<Element>(elements, elements.size(), 1, [](const auto &element) {return element;})[0];
}

#endif // ALGORITHMS_H
