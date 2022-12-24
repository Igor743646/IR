#include "Compressor.h"

size_t Compressor::s(size_t n) {
    return n == 0 ? 0 : 1 + s(n >> 1);
}

void Compressor::write_pos(FILE* file, size_t n) {
    char byte = 0;

    size_t sn = s(n);

    while (sn > 7) {
        byte |= (n >> ((sn - 1) / 7 * 7));
        fwrite(&byte, sizeof(char), 1, file);

        byte = 0;
        n &= (1ull << ((sn - 1) / 7 * 7)) - 1ull;
        sn -= 7;
    }

    byte |= 0x80ull | n;
    fwrite(&byte, sizeof(char), 1, file);
}

void Compressor::compress(FILE* file, const std::vector<size_t>& poses) {
    if (poses.size() == 0) {
        throw std::exception("poses array is empty");
    }

    size_t last_element = poses[0];

    write_pos(file, last_element);

    for (size_t i = 1; i < poses.size(); i++) {
        if (poses[i] <= last_element) {
            throw std::exception("poses array is not sorted");
        }

        size_t dif = poses[i] - last_element;
        write_pos(file, dif);

        last_element = poses[i];
    }
}

size_t Compressor::read_pos(FILE* file) {
    char byte;
    size_t res = 0;

    fread(&byte, sizeof(char), 1, file);
    res = byte & 0x7F;

    while ((byte & 0x80) == 0) {
        res <<= 7;
        fread(&byte, sizeof(char), 1, file);
        res |= (byte & 0x7F);
    }

    return res;
}

void Compressor::decompress(FILE* file, std::vector<size_t>& poses, size_t size) {
    size_t last_element;

    last_element = read_pos(file);
    poses.push_back(last_element);

    for (size_t i = 1; i < size; i++) {
        size_t dif = read_pos(file);

        poses.push_back(last_element + dif);
        last_element = last_element + dif;
    }
}
