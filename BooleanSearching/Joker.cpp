#include "Joker.h"

std::vector<size_t> Joker::trigram(const std::wstring& s) {
    std::wstring ms = s + L"$";

    std::vector<size_t> result;

    for (size_t i = 0; i < ms.size(); i++) {
        if ((ms[i] != L'*') and 
            (ms[(i + 1) % ms.size()] != L'*') and 
            (ms[(i + 2) % ms.size()] != L'*')) 
        {
            size_t cur = 0;
            cur |= ms[(i + 2) % ms.size()]; cur <<= sizeof(wchar_t) * 8;
            cur |= ms[(i + 1) % ms.size()]; cur <<= sizeof(wchar_t) * 8;
            cur |= ms[i];
            result.push_back(cur);
        }
    }

    return result;
}

std::wstring Joker::trigram_from_size_t(size_t s) {
    std::wstring result = L"000";

    auto shift = (1llu << 16) - 1;
    for (int i = 0; i < 3; i++) {
        wchar_t w = (((s & (shift << (16 * i))) >> (16 * i)));
        result[i] = w;
    }

    return result;
}