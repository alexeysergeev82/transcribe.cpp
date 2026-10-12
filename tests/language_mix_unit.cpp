// language_mix_unit.cpp - unit test for transcribe::parse_language_mix.
//
// The `language` hint may carry a weighted mix of locales
// ("ru-RU:0.75,en-US:0.25"). The parser must reject every malformed mix:
// weights that do not sum to 1 make the prompt-conditioned parakeet
// models return an empty transcript instead of an error.

#include "transcribe-language-mix.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

bool parses(const char * hint) {
    std::vector<transcribe::LanguageWeight> mix;
    return transcribe::parse_language_mix(hint, mix);
}

}  // namespace

int main() {
    // A plain locale is not a mix.
    CHECK(!transcribe::is_language_mix(nullptr));
    CHECK(!transcribe::is_language_mix(""));
    CHECK(!transcribe::is_language_mix("ru-RU"));
    CHECK(transcribe::is_language_mix("ru-RU:0.75,en-US:0.25"));
    CHECK(!parses(nullptr));
    CHECK(!parses("ru-RU"));

    // A valid mix keeps the order and the weights.
    std::vector<transcribe::LanguageWeight> mix;
    CHECK(transcribe::parse_language_mix("ru-RU:0.75,en-US:0.25", mix));
    CHECK(mix.size() == 2);
    if (mix.size() == 2) {
        CHECK(mix[0].locale == "ru-RU");
        CHECK(std::fabs(mix[0].weight - 0.75f) < 1e-6f);
        CHECK(mix[1].locale == "en-US");
        CHECK(std::fabs(mix[1].weight - 0.25f) < 1e-6f);
    }
    CHECK(parses("ru-RU:1"));
    CHECK(parses("ru-RU:0.7,en-US:0.2,de-DE:0.1"));
    CHECK(parses("ru-RU:0.755,en-US:0.25"));  // within the 0.01 tolerance

    // The weights must sum to 1.
    CHECK(!parses("ru-RU:1,en-US:1"));
    CHECK(!parses("ru-RU:0.5,en-US:0.25"));
    CHECK(!parses("ru-RU:0.78,en-US:0.25"));

    // Every weight is a positive finite number.
    CHECK(!parses("ru-RU:abc,en-US:1"));
    CHECK(!parses("ru-RU:0.75x,en-US:0.25"));
    CHECK(!parses("ru-RU:0,en-US:1"));
    CHECK(!parses("ru-RU:-0.5,en-US:1.5"));
    CHECK(!parses("ru-RU:nan,en-US:1"));
    CHECK(!parses("ru-RU:inf"));

    // Malformed items.
    CHECK(!parses(":1"));
    CHECK(!parses("ru-RU:"));
    CHECK(!parses("ru-RU:0.75,"));
    CHECK(!parses("ru-RU:0.75,en-US"));
    CHECK(!parses("ru-RU:0.75:0.25"));

    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("language_mix_unit: ok\n");
    return 0;
}
