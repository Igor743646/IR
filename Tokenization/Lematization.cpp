#include "Lematization.h"

namespace {
    static const std::unordered_set<std::wstring> ADJECTIVE = {
        L"ый", L"ому", L"ого", L"ым", L"ом",
        L"ий", L"ему", L"его", L"юю", L"ие",
        L"им", L"ей", L"ее",

        L"ое", L"о",

        L"ая", L"ой", L"ую", L"ою", L"а",

        L"ые", L"ых", L"ыми", L"ы",
    };

    static const std::unordered_set<std::wstring> VERB = {
        L"у", L"ишь", L"ит", L"им", L"ите",

        L"ят", L"я",

        L"ив", L"и", L"ить", L"ешь", L"ите",

        L"а",
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