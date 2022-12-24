#include "Lematization.h"

namespace {
    static const std::unordered_set<std::wstring> ADJECTIVE = {
        L"��", L"���", L"���", L"��", L"��",
        L"��", L"���", L"���", L"��", L"��",
        L"��", L"��", L"��",

        L"��", L"�",

        L"��", L"��", L"��", L"��", L"�",

        L"��", L"��", L"���", L"�",
    };

    static const std::unordered_set<std::wstring> VERB = {
        L"�", L"���", L"��", L"��", L"���",

        L"��", L"�",

        L"��", L"�", L"���", L"���", L"���",

        L"�",
    };
}

std::wstring Lematization::normalization(const std::wstring& word) {
    if (word.size() <= 3) {
        return word;
    }

    for (int k = 3; k > 0; k--) {
        if (ADJECTIVE.find(word.substr(word.size() - 1, k)) != ADJECTIVE.end()) {
            return word.substr(0, word.size() - k);
        }
    }

    for (int k = 3; k > 0; k--) {
        if (VERB.find(word.substr(word.size() - 1, k)) != VERB.end()) {
            return word.substr(0, word.size() - k);
        }
    }

    return word;
}