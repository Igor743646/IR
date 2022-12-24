#pragma once
#include <vector>
#include <string>

class Joker
{
public:

    static std::vector<size_t> trigram(const std::wstring&);
    static std::wstring trigram_from_size_t(size_t);
};

