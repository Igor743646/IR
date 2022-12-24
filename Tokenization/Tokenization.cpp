#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <set>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <locale>
#include <codecvt>
#include <chrono>
#include <unordered_map>
#include <map>

#include "Compressor.h"
#include "Lematization.h"
#include "Joker.h"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#define OPENMACROS(func) do {                                           \
    auto err = func;                                                    \
        if (err != 0) {                                                 \
            fprintf(stderr, "ERROR Bad opening in %s:%d.\n", __FILE__, __LINE__);           \
            exit(-1);                                                   \
        }                                                               \
} while (0);

constexpr auto MAXPOCKETSIZE = 300000;
constexpr auto MAXSTRINGSIZE = 300;
constexpr auto SORTINGPATH = "C:\\Users\\Igor\\Desktop\\инфопоиск\\sortingpath\\";
constexpr auto SORTINGPATH2 = "C:\\Users\\Igor\\Desktop\\инфопоиск\\sortingpath2\\";

const std::unordered_map<wchar_t, bool> alphabet{ {L'а',true}, {L'б',true}, {L'в',true}, {L'г',true}, {L'д',true}, {L'ё',true},
    {L'е',true}, {L'ж',true}, {L'з',true}, {L'и',true}, {L'й',true}, {L'к',true}, {L'л',true}, {L'м',true}, {L'н',true},
    {L'о',true}, {L'п',true}, {L'р',true}, {L'с',true}, {L'т',true}, {L'у',true}, {L'ф',true}, {L'х',true}, {L'ц',true},
    {L'ч',true}, {L'ш',true}, {L'щ',true}, {L'ъ',true}, {L'ы',true}, {L'ь',true}, {L'э',true}, {L'ю',true}, {L'я',true},
    {L'А',true}, {L'Б',true}, {L'В',true}, {L'Г',true}, {L'Д',true}, {L'Е',true}, {L'Ж',true}, {L'З',true}, {L'И',true}, {L'Ё',true},
    {L'Й',true}, {L'К',true}, {L'Л',true}, {L'М',true}, {L'Н',true}, {L'О',true}, {L'П',true}, {L'Р',true}, {L'С',true},
    {L'Т',true}, {L'У',true}, {L'Ф',true}, {L'Х',true}, {L'Ц',true}, {L'Ч',true}, {L'Ш',true}, {L'Щ',true}, {L'Ъ',true},
    {L'Ы',true}, {L'Ь',true}, {L'Э',true}, {L'Ю',true}, {L'Я',true},
};

struct TOKEN {
    size_t hash;
    size_t docID;
    size_t pos;

    inline friend bool operator<(const TOKEN& a, const TOKEN& b) {
        return (a.hash < b.hash) or 
            ((a.hash == b.hash) and (a.docID < b.docID)) or
            ((a.hash == b.hash) and (a.docID == b.docID) and (a.pos < b.pos));
    }
};

struct TRIGRAM {
    size_t trig;
    size_t hash;

    inline friend bool operator<(const TRIGRAM& a, const TRIGRAM& b) {
        return (a.trig < b.trig) or
            ((a.trig == b.trig) and (a.hash < b.hash));
    }

    inline friend bool operator==(const TRIGRAM& a, const TRIGRAM& b) {
        return ((a.trig == b.trig) and (a.hash == b.hash));
    }
};

std::size_t token_to_term(std::wstring s) {
    static std::hash<std::wstring> hash_wstr;

    for (size_t i = 0; i < s.size(); i++) {
        s[i] = towlower(s[i]);
    }

    return hash_wstr(s);
}

size_t file_id(const std::string& path, const std::string& name) {
    size_t result = 0;

    for (size_t i = path.size(); i < name.size(); i++) {
        if (name[i] == '_') break;
        result = result * 10 + (static_cast<size_t>(name[i] - '0'));
    }

    return result;
}

void tokenize(const std::string& name, std::set<TOKEN>& dictionary, std::set<TRIGRAM>& trigram_dictionary, 
    size_t docID, FILE* straight_index, FILE* straight_index_ID) {
    
    std::wstring row;
    
    FILE* input_file;
    OPENMACROS(fopen_s(&input_file, name.c_str(), "r, ccs=UTF-8"));
    
    wchar_t word[MAXSTRINGSIZE] = { L'\0' };
    size_t buffer_size = 0;
    wchar_t buffer[MAXSTRINGSIZE] = { L'\0' };

    /* TITLE READING */
    fgetws(word, MAXSTRINGSIZE, input_file);
    fgetws(word, MAXSTRINGSIZE, input_file);

    std::wstring title(word, wcslen(word) - 1);
    size_t title_size = title.size();
    long pointer = ftell(straight_index_ID);

    fwrite(&docID, sizeof(size_t), 1, straight_index);
    fwrite(&title_size, sizeof(size_t), 1, straight_index);
    fwrite(title.data(), sizeof(wchar_t), title_size, straight_index);
    

    size_t position = 0;
    while (fwscanf_s(input_file, L"%ls", word, MAXSTRINGSIZE) >= 0) {
        row = std::wstring(word);
        for (size_t i = 0; i < row.size(); i++) {
            wchar_t c = row[i];
            bool in_alpha = (alphabet.find(c) != alphabet.end());

            if (in_alpha) {
                buffer[buffer_size] = towlower(c);
                buffer_size++;
            }

            if ((!in_alpha or (i == row.size() - 1)) and buffer_size) {

                fwrite(&buffer_size, sizeof(size_t), 1, straight_index_ID);
                fwrite(buffer, sizeof(wchar_t), buffer_size, straight_index_ID);

                std::wstring word1 = std::wstring(buffer, buffer_size);
                size_t hash = token_to_term(word1);
                size_t normal_hash = token_to_term(Lematization::normalization(word1));
                
                dictionary.insert({ normal_hash, docID, position });
                dictionary.insert({hash, docID, position});

                for (size_t trig : Joker::trigram(word1)) {
                    trigram_dictionary.insert({trig, hash});
                }

                position++;
                buffer_size = 0;
            }
        }
    }

    fwrite(&position, sizeof(size_t), 1, straight_index);
    fwrite(&pointer, sizeof(long), 1, straight_index);

    fclose(input_file);
}

void process_files(const std::string& Path, const std::string& Out2, size_t& COUNT_OF_FILES, size_t& COUNT_OF_OUT_FILES, size_t& COUNT_OF_OUT_FILES2) {
    std::set<TOKEN> dictionary;
    std::set<TRIGRAM> trigram_dictionary;
    std::filesystem::path path(Path);
    std::filesystem::directory_iterator iter(path);
    std::string name;
    COUNT_OF_FILES = 0;
    COUNT_OF_OUT_FILES = 0;
    COUNT_OF_OUT_FILES2 = 0;

    FILE* straight_index, * straight_index_ID;
    FILE* file_with_tokens_c, * file_with_trigram_c;
    OPENMACROS(fopen_s(&straight_index, Out2.c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&straight_index_ID, (Out2 + "_ID").c_str(), "wb, ccs=UNICODE"));
    
    while (!iter._At_end())
    {
        std::filesystem::directory_entry entry = *iter;
        name = entry.path().string();

        tokenize(name, dictionary, trigram_dictionary, file_id(Path, name), straight_index, straight_index_ID);

        if (dictionary.size() > MAXPOCKETSIZE) {
            OPENMACROS(fopen_s(&file_with_tokens_c, (SORTINGPATH + std::to_string(COUNT_OF_OUT_FILES) + ".data").c_str(), "wb, ccs=UNICODE"));
            for (auto& token : dictionary) {
                fwrite(&token, sizeof(TOKEN), 1, file_with_tokens_c);
            }
            fclose(file_with_tokens_c);
            dictionary.clear();
            COUNT_OF_OUT_FILES++;
        }

        if (trigram_dictionary.size() > MAXPOCKETSIZE) {
            OPENMACROS(fopen_s(&file_with_trigram_c, (SORTINGPATH2 + std::to_string(COUNT_OF_OUT_FILES2) + ".data").c_str(), "wb, ccs=UNICODE"));
            for (auto& trigram : trigram_dictionary) {
                fwrite(&trigram, sizeof(TRIGRAM), 1, file_with_trigram_c);
            }
            fclose(file_with_trigram_c);
            trigram_dictionary.clear();
            COUNT_OF_OUT_FILES2++;
        }
        
        COUNT_OF_FILES++;
        iter++;

        if (COUNT_OF_FILES % 1000 == 0) {
            std::cout << "[" << COUNT_OF_FILES << "] done...\n";
        }
    }

    fclose(straight_index_ID);
    fclose(straight_index);

    if (dictionary.size()) {
        OPENMACROS(fopen_s(&file_with_tokens_c, (SORTINGPATH + std::to_string(COUNT_OF_OUT_FILES) + ".data").c_str(), "wb, ccs=UNICODE"));
        for (auto& token : dictionary) {
            fwrite(&token, sizeof(TOKEN), 1, file_with_tokens_c);
        }
        fclose(file_with_tokens_c);
        dictionary.clear();
        COUNT_OF_OUT_FILES++;
    }

    if (trigram_dictionary.size()) {
        OPENMACROS(fopen_s(&file_with_trigram_c, (SORTINGPATH2 + std::to_string(COUNT_OF_OUT_FILES2) + ".data").c_str(), "wb, ccs=UNICODE"));
        for (auto& trigram : trigram_dictionary) {
            fwrite(&trigram, sizeof(TRIGRAM), 1, file_with_trigram_c);
        }
        fclose(file_with_trigram_c);
        trigram_dictionary.clear();
        COUNT_OF_OUT_FILES2++;
    }
}

template<class T>
void merge_files(const std::string& path, size_t l, size_t r, size_t step = 0) {
    if (r == l) {
        if (std::rename((path + std::to_string(l) + ".data").c_str(),
            (path + "merge" + std::to_string(step) + ".data").c_str()))
        {
            std::perror("Error renaming");
            exit(-1);
        }
        return;
    }

    merge_files<T>(path, l, (l + r) / 2, step * 2 + 1);
    merge_files<T>(path, (l + r) / 2 + 1, r, step * 2 + 2);

    std::cout << "File " << l << " to " << r << " merging... on step " << step << std::endl;

    FILE* input1, * input2, * output;
    OPENMACROS(fopen_s(&input1, (path + "merge" + std::to_string(step * 2 + 1) + ".data").c_str(), "rb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&input2, (path + "merge" + std::to_string(step * 2 + 2) + ".data").c_str(), "rb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&output, (path + "merge" + std::to_string(step) + ".data").c_str(), "wb, ccs=UNICODE"));

    T TI1, TI2;
    size_t n1, n2;

    n1 = fread(&TI1, sizeof(T), 1, input1);
    n2 = fread(&TI2, sizeof(T), 1, input2);

    do {
        if (n1 <= 0 and n2 <= 0) {
            break;
        }
        else if (n1 <= 0) {
            fwrite(&TI2, sizeof(T), 1, output);
            n2 = fread(&TI2, sizeof(T), 1, input2);
        }
        else if (n2 <= 0) {
            fwrite(&TI1, sizeof(T), 1, output);
            n1 = fread(&TI1, sizeof(T), 1, input1);
        }
        else {

            if (TI1 < TI2) {
                fwrite(&TI1, sizeof(T), 1, output);
                n1 = fread(&TI1, sizeof(T), 1, input1);
            }
            else {
                fwrite(&TI2, sizeof(T), 1, output);
                n2 = fread(&TI2, sizeof(T), 1, input2);
            }
        }

    } while (1);


    fclose(input1);
    fclose(input2);
    fclose(output);

    try {
        std::filesystem::remove((path + "merge" + std::to_string(step * 2 + 1) + ".data"));
        std::filesystem::remove((path + "merge" + std::to_string(step * 2 + 2) + ".data"));
    }
    catch (const std::filesystem::filesystem_error& err) {
        std::cout << "filesystem error: " << err.what() << '\n';
    }
}

void concatenate(const std::string& path, const std::string& out, const std::string& coords_out) {
    FILE *input, * output, *output_ID, * coords_output, * coords_output_ID, * coords_output_ID_POS;

    OPENMACROS(fopen_s(&input, path.c_str(), "rb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&output, out.c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&output_ID, (out + "_ID").c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&coords_output, coords_out.c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&coords_output_ID, (coords_out + "_ID").c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&coords_output_ID_POS, (coords_out + "_ID_POS").c_str(), "wb, ccs=UNICODE"));

    TOKEN last_TI, TI;
    size_t sum_freq = 0;
    std::vector<std::pair<size_t, long>> fileIdes;
    std::vector<size_t> filePos;

    long prev_hash = -1;

    while (fread(&TI, sizeof(TOKEN), 1, input) > 0) {
        if (sum_freq == 0) {
            last_TI = TI;
            sum_freq++;
            fileIdes.push_back({ TI.docID, ftell(coords_output_ID_POS) });
            filePos.push_back(TI.pos);

            prev_hash = ftell(coords_output_ID);

            fwrite(&TI.hash, sizeof(size_t), 1, coords_output);
            fwrite(&prev_hash, sizeof(long), 1, coords_output);
        }
        else if (last_TI.hash == TI.hash and last_TI.docID == TI.docID) {
            sum_freq++;
            filePos.push_back(TI.pos);
        }
        else if (last_TI.hash == TI.hash) {
            sum_freq++;

            size_t filePos_size = filePos.size();
            
            fwrite(&filePos_size, sizeof(size_t), 1, coords_output_ID_POS);
            Compressor::compress(coords_output_ID_POS, filePos);
            
            last_TI = TI;
            filePos.clear();
            fileIdes.push_back({ TI.docID, ftell(coords_output_ID_POS) });
            filePos.push_back(TI.pos);
        }
        else {
            size_t fileIdes_size = fileIdes.size();
            size_t filePos_size = filePos.size();
            long pointer_output_ID = ftell(output_ID);
            
            // INDEX FILE
            fwrite(&last_TI.hash, sizeof(size_t), 1, output);
            fwrite(&sum_freq, sizeof(size_t), 1, output);
            fwrite(&pointer_output_ID, sizeof(long), 1, output);

            fwrite(&fileIdes_size, sizeof(size_t), 1, output_ID);
            Compressor::compress(output_ID, fileIdes);

            // COORDS FILE
            fwrite(&fileIdes_size, sizeof(size_t), 1, coords_output_ID);
            for (int i = 0; i < fileIdes.size(); i++) {
                fwrite(&fileIdes[i].first, sizeof(size_t), 1, coords_output_ID);
                fwrite(&fileIdes[i].second, sizeof(long), 1, coords_output_ID);
            }

            fwrite(&filePos_size, sizeof(size_t), 1, coords_output_ID_POS);
            Compressor::compress(coords_output_ID_POS, filePos);
            
            prev_hash = ftell(coords_output_ID);

            fwrite(&TI.hash, sizeof(size_t), 1, coords_output);
            fwrite(&prev_hash, sizeof(long), 1, coords_output);
            
            last_TI = TI;
            sum_freq = 1;
            fileIdes.clear();
            filePos.clear();
            fileIdes.push_back({ TI.docID, ftell(coords_output_ID_POS) });
            filePos.push_back(TI.pos);
        }
    }

    if (fileIdes.size()) {
        size_t fileIdes_size = fileIdes.size();
        size_t filePos_size = filePos.size();
        long pointer_output_ID = ftell(output_ID);
        
        // INDEX FILE
        fwrite(&last_TI.hash, sizeof(size_t), 1, output);
        fwrite(&sum_freq, sizeof(size_t), 1, output);
        fwrite(&pointer_output_ID, sizeof(long), 1, output);

        fwrite(&fileIdes_size, sizeof(size_t), 1, output_ID);
        Compressor::compress(output_ID, fileIdes);

        // COORDS FILE
        fwrite(&fileIdes_size, sizeof(size_t), 1, coords_output_ID);
        for (int i = 0; i < fileIdes.size(); i++) {
            fwrite(&fileIdes[i].first, sizeof(size_t), 1, coords_output_ID);
            fwrite(&fileIdes[i].second, sizeof(long), 1, coords_output_ID);
        }

        fwrite(&filePos_size, sizeof(size_t), 1, coords_output_ID_POS);
        Compressor::compress(coords_output_ID_POS, filePos);
        
        prev_hash = ftell(coords_output_ID);

        fwrite(&TI.hash, sizeof(size_t), 1, coords_output);
        fwrite(&prev_hash, sizeof(long), 1, coords_output);
        
        fileIdes.clear();
        filePos.clear();
    }

    fclose(coords_output_ID_POS);
    fclose(coords_output_ID);
    fclose(coords_output);
    fclose(output_ID);
    fclose(output);
    fclose(input);
}

void concatenate_trigrams(const std::string& path, const std::string& out) {
    FILE* input, * output, * output_ID;

    OPENMACROS(fopen_s(&input, path.c_str(), "rb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&output, out.c_str(), "wb, ccs=UNICODE"));
    OPENMACROS(fopen_s(&output_ID, (out + "_ID").c_str(), "wb, ccs=UNICODE"));

    TRIGRAM last_TG, TG;
    size_t sum_freq = 0;
    std::vector<size_t> fileHashes;

    while (fread(&TG, sizeof(TRIGRAM), 1, input) > 0) {
        if (sum_freq == 0) {
            last_TG = TG;
            sum_freq++;
            fileHashes.push_back(TG.hash);
            
        }
        else if (last_TG.trig == TG.trig and last_TG.hash == TG.hash) {
            sum_freq++;
        }
        else if (last_TG.trig == TG.trig) {
            sum_freq++;
            last_TG = TG;
            fileHashes.push_back(TG.hash);
        }
        else {
            size_t fileHashes_size = fileHashes.size();
            long pointer_output_ID = ftell(output_ID);

            // TRIGRAM INDEX FILE
            fwrite(&last_TG.trig, sizeof(size_t), 1, output);
            fwrite(&sum_freq, sizeof(size_t), 1, output);
            fwrite(&pointer_output_ID, sizeof(long), 1, output);

            fwrite(&fileHashes_size, sizeof(size_t), 1, output_ID);
            Compressor::compress(output_ID, fileHashes);

            last_TG = TG;
            sum_freq = 1;
            fileHashes.clear();
            fileHashes.push_back(TG.hash);
        }
    }

    if (fileHashes.size()) {
        size_t fileHashes_size = fileHashes.size();
        long pointer_output_ID = ftell(output_ID);

        // TRIGRAM INDEX FILE
        fwrite(&last_TG.trig, sizeof(size_t), 1, output);
        fwrite(&sum_freq, sizeof(size_t), 1, output);
        fwrite(&pointer_output_ID, sizeof(long), 1, output);

        fwrite(&fileHashes_size, sizeof(size_t), 1, output_ID);
        Compressor::compress(output_ID, fileHashes);
    }

    fclose(output_ID);
    fclose(output);
    fclose(input);
}

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::string Corpus, Path, Out1, Out2, Out3, Out4, argv1, argv2, argv3, argv4, argv5;

    switch (argc) {
    case 7:
        Corpus = std::string(argv[1]); // C:\Users\Igor\Desktop\инфопоиск\corpus_very_light\ 
        Path = std::string(argv[2]);   // C:\Users\Igor\Desktop\инфопоиск\ 
        Out1 = std::string(argv[3]);    // inverted_index_very_light
        Out2 = std::string(argv[4]);    // straight_index_very_light
        Out3 = std::string(argv[5]);    // coords_index_very_light
        Out4 = std::string(argv[6]);    // trigram_index_very_light
        std::cout << "Corpus path: " << Corpus << std::endl;
        std::cout << "Directory for working: " << Path << std::endl;
        std::cout << "Output filename: " << Out1 << std::endl;
        std::cout << "Output filename: " << Out2 << std::endl;
        std::cout << "Output filename: " << Out3 << std::endl;
        std::cout << "Output filename: " << Out4 << std::endl;
        break;
    default:
        std::cout << "Enter the path to files for indexing." << std::endl;
        return 0;
    }

    double SUMMARY_TIME = 0.0;

    /* TOKENIZATION */
    argv1 = Corpus;                 // C:\Users\Igor\Desktop\инфопоиск\corpus_very_light\ 
    argv2 = Path + Out2 + ".data";   // C:\Users\Igor\Desktop\инфопоиск\straight_index_very_light.data
    argv3 = Path + Out4 + ".data";   // C:\Users\Igor\Desktop\инфопоиск\trigram_index_very_light.data

    auto start1 = std::chrono::high_resolution_clock::now();
    size_t COUNT_OF_FILES, COUNT_OF_OUT_FILES, COUNT_OF_OUT_FILES2;
    process_files(argv1, argv2, COUNT_OF_FILES, COUNT_OF_OUT_FILES, COUNT_OF_OUT_FILES2);
    auto end1 = std::chrono::high_resolution_clock::now();

    SUMMARY_TIME += std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count();
    std::cout << "\n================\n" << 
        "Summary work time with " << COUNT_OF_FILES << " files: "
        << std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count() << "sec\n"
        << "Out files for tokens: " << COUNT_OF_OUT_FILES << "\nOut files for trigrams: " << COUNT_OF_OUT_FILES2
        << "\n================\n\n";

    /* SORTING */
    argv1 = Path + "sorted_tokens.data";    // C:\Users\Igor\Desktop\инфопоиск\sorted_tokens.data
    argv2 = Path + "sorted_trigrams.data";    // C:\Users\Igor\Desktop\инфопоиск\sorted_trigrams.data

    // Start sort tokens
    start1 = std::chrono::high_resolution_clock::now();
    merge_files<TOKEN>(SORTINGPATH, 0llu, COUNT_OF_OUT_FILES - 1llu);
    end1 = std::chrono::high_resolution_clock::now();

    SUMMARY_TIME += std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count();
    std::cout << "\n================\n" <<
        "Summary sorting tokens time with " << COUNT_OF_OUT_FILES << " pockets: "
        << std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count()
        << "sec\n================\n";

    // Start sort trigrams
    start1 = std::chrono::high_resolution_clock::now();
    merge_files<TRIGRAM>(SORTINGPATH2, 0llu, COUNT_OF_OUT_FILES2 - 1llu);
    end1 = std::chrono::high_resolution_clock::now();

    SUMMARY_TIME += std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count();
    std::cout << "\n================\n" <<
        "Summary sorting trigrams time with " << COUNT_OF_OUT_FILES2 << " pockets: "
        << std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count()
        << "sec\n================\n";

    try { 
        std::filesystem::copy(std::string(SORTINGPATH) + "merge0.data", argv1, std::filesystem::copy_options::update_existing);
        std::filesystem::copy(std::string(SORTINGPATH2) + "merge0.data", argv2, std::filesystem::copy_options::update_existing);
    }
    catch (const std::filesystem::filesystem_error& e) { std::cout << e.what() << std::endl; }

    /* MAKE INVERTED, TRIGRAM AND COORDINATE INDEX */
    argv1 = argv1;                    // C:\Users\Igor\Desktop\инфопоиск\sorted_tokens.data 
    argv4 = argv2;                    // C:\Users\Igor\Desktop\инфопоиск\sorted_trigrams.data
    argv2 = Path + Out1 + ".data";    // C:\Users\Igor\Desktop\инфопоиск\inverted_index_very_light.data
    argv3 = Path + Out3 + ".data";    // C:\Users\Igor\Desktop\инфопоиск\coords_index_very_light.data 
    argv5 = Path + Out4 + ".data";    // C:\Users\Igor\Desktop\инфопоиск\trigram_index_very_light.data

    start1 = std::chrono::high_resolution_clock::now();
    concatenate(argv1, argv2, argv3);
    end1 = std::chrono::high_resolution_clock::now();

    SUMMARY_TIME += std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count();
    std::cout << "\n================\n" <<
        "Summary merging tokens time: " << std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count()
        << "sec\n================\n";

    start1 = std::chrono::high_resolution_clock::now();
    concatenate_trigrams(argv4, argv5);
    end1 = std::chrono::high_resolution_clock::now();

    SUMMARY_TIME += std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count();
    std::cout << "\n================\n" <<
        "Summary merging trigrams time: " << std::chrono::duration<double, std::ratio<1, 1>>(end1 - start1).count()
        << "sec\n================\n";

    std::cout << "\n================\n" << "Summary time: " << SUMMARY_TIME << "sec\n================\n";

    return 0;
}