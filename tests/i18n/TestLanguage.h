#pragma once

#include <string>

namespace snotes::i18n::test
{

/// resources/Languages/de.txt.
[[nodiscard]] std::string germanFileText();

/// The tests run in German, so they also check the German translations (F-UI-01).
void useGerman();
void useEnglish();

/// English for one test, then German again.
class ScopedEnglish
{
public:
    ScopedEnglish() { useEnglish(); }
    ~ScopedEnglish() { useGerman(); }

    ScopedEnglish(const ScopedEnglish&) = delete;
    ScopedEnglish& operator=(const ScopedEnglish&) = delete;
    ScopedEnglish(ScopedEnglish&&) = delete;
    ScopedEnglish& operator=(ScopedEnglish&&) = delete;
};

} // namespace snotes::i18n::test
