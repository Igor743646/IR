#pragma once
#include <stdio.h>
#include <vector>

class Compressor
{
public:
	Compressor() = delete;
	static size_t s(size_t);
	static void write_pos(FILE*, size_t);
	static void compress(FILE*, const std::vector<size_t>&);
	static size_t read_pos(FILE*);
	static void decompress(FILE*, std::vector<size_t>&, size_t);
};
