// transcribe-language-mix.h - a weighted mix of locales in the `language` hint.
//
// transcribe_run_params::language normally names one locale ("ru-RU"). The
// prompt-conditioned parakeet models (nemotron-3.5) select the language with
// a one-hot vector over their locale table, so they can also take a weighted
// row: "ru-RU:0.75,en-US:0.25". A colon in the hint marks a mix.
//
// Why: autodetect on Russian speech drifts into Ukrainian spelling, and a
// single fixed locale suppresses the other language. A mix keeps both.

#pragma once

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace transcribe {

struct LanguageWeight {
    std::string locale;
    float       weight;
};

inline bool is_language_mix(const char * hint) {
    return hint != nullptr && std::strchr(hint, ':') != nullptr;
}

// Parses "ll-RR:w,ll-RR:w". Returns false when the hint is not a mix, an item
// is malformed, a weight is not a positive finite number, or the weights do
// not sum to 1 within 0.01. No normalization: a sum other than 1 makes the
// model return an empty transcript, and fixing it silently would hide the
// caller's mistake.
inline bool parse_language_mix(const char * hint, std::vector<LanguageWeight> & out) {
    out.clear();
    if (!is_language_mix(hint)) {
        return false;
    }
    const std::string spec(hint);
    float             sum = 0.0f;
    size_t            pos = 0;
    while (pos <= spec.size()) {
        size_t end = spec.find(',', pos);
        if (end == std::string::npos) {
            end = spec.size();
        }
        const std::string item  = spec.substr(pos, end - pos);
        const size_t      colon = item.find(':');
        if (colon == std::string::npos || colon == 0 || colon + 1 == item.size()) {
            return false;
        }
        const std::string number = item.substr(colon + 1);
        char *            tail   = nullptr;
        // strtof follows LC_NUMERIC; hosts that set a decimal-comma locale must pass weights accordingly.
        const float weight = std::strtof(number.c_str(), &tail);
        if (tail != number.c_str() + number.size() || !std::isfinite(weight) || !(weight > 0.0f)) {
            return false;
        }
        out.push_back({ item.substr(0, colon), weight });
        sum += weight;
        pos = end + 1;
    }
    return std::fabs(sum - 1.0f) <= 0.01f;
}

}  // namespace transcribe
