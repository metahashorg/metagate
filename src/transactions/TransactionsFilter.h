#ifndef TRANSACTIONSFILTER_H
#define TRANSACTIONSFILTER_H

namespace transactions {

enum class FilterType {
    None, True, False
};

struct Filters {
    FilterType isInput = FilterType::None;
    FilterType isOutput = FilterType::None;
    FilterType isTesting = FilterType::None;
    FilterType isForging = FilterType::None;
    FilterType isDelegate = FilterType::None;
    FilterType isSuccess = FilterType::None;
};

} // namespace transactions

#endif // TRANSACTIONSFILTER_H
