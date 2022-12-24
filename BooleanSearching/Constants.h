#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#define DEFAULT_PORT "27015"

#define OPENMACROS(func) do {                                  \
    auto err = func;    \
        if (err != 0) {                                     \
            std::cout << "Bad opening file for writing " << __LINE__ << std::endl;  \
            exit(-1);                                       \
        }                                                   \
} while (0);

#define CSC_SOCKET_ERROR(err, err_type, message, mode) do { \
    if (err == err_type) { \
        fprintf(stderr, "ERROR in %s on line %d: %s", __FILE__, __LINE__, message); \
        if (mode & 1) freeaddrinfo(result); \
		if (mode & 2) closesocket(ListenSocket); \
		if (mode & 4) closesocket(ClientSocket); \
		if (mode & 8) WSACleanup(); \
        exit(-1); \
    } \
} while (false);

enum TYPE;
struct termInfo;
struct straightInfo;
struct ELEM;
struct CORPUS;
struct CompareType;

constexpr auto MAXSTRINGSIZE = 300llu;
using index_dictionary_type = std::unordered_map<size_t, long>;
using trigrams_dictionary_type = std::unordered_map<size_t, long>;
using straight_dictionary_type = std::unordered_map<size_t, straightInfo>;
using coords_dictionary_type = std::unordered_map<size_t, long>;
using term_docs_type = std::unordered_map<size_t, long>;

enum TYPE {
	OPENBRACKET = 0,
	CLOSEBRACKET = 1,
	OPAND = 2,
	OPOR = 3,
};

#pragma pack (4)
struct termInfo {
	size_t hash;
	size_t freq;
	long pointer;
};

#pragma pack (4)
struct trigInfo {
	size_t trig;
	size_t freq;
	long pointer;
};

struct straightInfo {
	std::wstring title;
	size_t count_of_words = 0;
};

struct ELEM {
	TYPE type;
};

struct CORPUS {
	std::string& path;
	std::string& trigrams_path;
	std::string& straight_path;
	std::string& coords_path;
	index_dictionary_type& dictionary;
	trigrams_dictionary_type& trigrams_dictionary;
	straight_dictionary_type& straight_dictionary;
	coords_dictionary_type& coords_dictionary;
};

struct CompareType {
	size_t doc;
	double rank;

	bool operator>(const CompareType& a) const {
		return doc > a.doc;
	}

	bool operator<(const CompareType& a) const {
		return doc < a.doc;
	}

	bool operator==(const CompareType& a) const {
		return doc == a.doc;
	}

	CompareType operator+(const CompareType& b) const {
		return { doc, rank + b.rank };
	}
};