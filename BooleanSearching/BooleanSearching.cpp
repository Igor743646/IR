#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <assert.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <set>

#include "Compressor.h"
#include "Constants.h"
#include "Lematization.h"
#include "Joker.h"

#pragma comment (lib, "Ws2_32.lib")

template <class T>
std::vector<T> op_intersect(const std::vector<T>& l, const std::vector<T>& r) {
	std::vector<T> result;

	size_t i = 0, j = 0;

	while (i < l.size() && j < r.size()) {
		if (l[i] == r[j]) {
			result.push_back(l[i] + r[j]);
			i++;
			j++;
		}
		else if (l[i] < r[j]) {
			i++;
		}
		else if (l[i] > r[j]) {
			j++;
		}
	}

	return result;
}

template <class T>
std::vector<T> op_union(const std::vector<T>& l, const std::vector<T>& r) {
	std::vector<T> result;

	size_t i = 0, j = 0;

	while (i < l.size() or j < r.size()) {

		if (i >= l.size() and j < r.size()) {
			result.push_back(r[j]);
			j++;
		}
		else if (j >= r.size() and i < l.size()) {
			result.push_back(l[i]);
			i++;
		}
		else {
			if (l[i] == r[j]) {
				result.push_back(l[i] + r[j]);
				i++;
				j++;
			}
			else if (l[i] < r[j]) {
				result.push_back(l[i]);
				i++;
			}
			else if (l[i] > r[j]) {
				result.push_back(r[j]);
				j++;
			}
		}
	}

	return result;
}

std::vector<size_t> op_intersect_hashes(const std::vector<size_t>& l, const std::vector<size_t>& r) {
	std::vector<size_t> result;

	size_t i = 0, j = 0;

	while (i < l.size() && j < r.size()) {
		if (l[i] == r[j]) {
			result.push_back(l[i]);
			i++;
			j++;
		}
		else if (l[i] < r[j]) {
			i++;
		}
		else if (l[i] > r[j]) {
			j++;
		}
	}

	return result;
}

std::size_t token_to_term(std::wstring s) {
	std::hash<std::wstring> hash_wstr;

	for (size_t i = 0; i < s.size(); i++) {
		s[i] = towlower(s[i]);
	}

	return hash_wstr(s);
}

std::wstring token_to_terms(std::wstring s) {
	
	for (size_t i = 0; i < s.size(); i++) {
		s[i] = towlower(s[i]);
	}

	return s;
}

std::vector<CompareType> get_indexes(const CORPUS& Corpus, size_t hash, long pointer, double xw = 1.0) {
	FILE* coords_index_ID, * coords_index_ID_POS;
	OPENMACROS(fopen_s(&coords_index_ID, (Corpus.coords_path + "_ID").c_str(), "rb, ccs=UNICODE"));
	OPENMACROS(fopen_s(&coords_index_ID_POS, (Corpus.coords_path + "_ID_POS").c_str(), "rb, ccs=UNICODE"));
	fseek(coords_index_ID, pointer, 0);

	size_t N = Corpus.straight_dictionary.size(); // N
	size_t size; // dj
	fread(&size, sizeof(size_t), 1, coords_index_ID);

	std::vector<CompareType> result(size);

	for (size_t i = 0; i < size; i++) {
		size_t docID;
		long pointer_pos;
		fread(&docID, sizeof(size_t), 1, coords_index_ID);
		fread(&pointer_pos, sizeof(long), 1, coords_index_ID);

		size_t Ni = Corpus.straight_dictionary[docID].count_of_words; // Ni
		size_t nij; // nij

		fseek(coords_index_ID_POS, pointer_pos, 0);
		fread(&nij, sizeof(size_t), 1, coords_index_ID_POS);
		
		double rank = static_cast<double>(nij) / static_cast<double>(Ni) * 
			log10(static_cast<double>(N) / static_cast<double>(size));
		result[i] = {docID, xw * rank};
	}
	
	fclose(coords_index_ID);
	fclose(coords_index_ID_POS);

	return result;
}

std::vector<size_t> getHashes(const CORPUS& Corpus, size_t trig) {
	FILE* trigram_index_ID;
	OPENMACROS(fopen_s(&trigram_index_ID, (Corpus.trigrams_path + "_ID").c_str(), "rb, ccs=UNICODE"));
	
	long pointer = Corpus.trigrams_dictionary[trig];
	fseek(trigram_index_ID, pointer, 0);

	size_t size;
	fread(&size, sizeof(size_t), 1, trigram_index_ID);
	
	std::vector<size_t> result;
	//fread(result.data(), sizeof(size_t), size, trigram_index_ID);
	Compressor::decompress(trigram_index_ID, result, size);

	fclose(trigram_index_ID);
	return result;
}

std::vector<CompareType> getDocsByPattern(const CORPUS& Corpus, std::wstring& buff) {
	std::vector<CompareType> result;
	
	std::vector<size_t> trigrams = Joker::trigram(token_to_terms(buff));

	if (trigrams.size() == 0) return result;

	std::vector<size_t> hashes = getHashes(Corpus, trigrams[0]);

	for (size_t i = 1; i < trigrams.size(); i++) {
		std::vector<size_t> hashesi = getHashes(Corpus, trigrams[i]);
		hashes = op_intersect_hashes(hashes, hashesi);
	}

	if (hashes.size() == 0) return result;
	
	for (size_t i = 0; i < hashes.size(); i++) {
		size_t hash = hashes[i];
		index_dictionary_type::iterator iter = Corpus.coords_dictionary.find(hash);
		if (iter != Corpus.coords_dictionary.end()) {
			result = op_union(result, get_indexes(Corpus, hash, iter->second));
		}
	}

	return result;
}

std::vector<CompareType> getDocs(const CORPUS& Corpus, std::wstring& buff, int mode = 1)
{
	using  namespace std::string_literals;
	if (mode == 1 and buff.find_first_of('*') != std::string::npos) {
		return getDocsByPattern(Corpus, buff);
	}

	std::vector<CompareType> result;

	size_t hash = token_to_term(buff);
	index_dictionary_type::iterator iter = Corpus.coords_dictionary.find(hash);
	if (iter != Corpus.coords_dictionary.end()) {
		result = get_indexes(Corpus, hash, iter->second);
	}

	if (mode == 1) {
		size_t normal_hash = token_to_term(Lematization::normalization(buff));
		index_dictionary_type::iterator normal_iter = Corpus.coords_dictionary.find(normal_hash);
		if (normal_iter != Corpus.coords_dictionary.end()) {
			result = op_union(result, get_indexes(Corpus, normal_hash, normal_iter->second, 0.1));
		}
	}

	return result;
}

void getPos(const CORPUS& Corpus, FILE* pp, term_docs_type& docs, size_t doc_ID, size_t& size)
{
	term_docs_type::iterator iter = docs.find(doc_ID);
	if (iter == docs.end()) {
		size = 0;
	}

	fseek(pp, iter->second, 0);
	fread(&size, sizeof(size_t), 1, pp);
}

void getDocPos(const CORPUS& Corpus, size_t word_hash, term_docs_type& docs)
{
	long pointer = Corpus.coords_dictionary[word_hash];

	FILE* file;
	OPENMACROS(fopen_s(&file, (Corpus.coords_path + "_ID").c_str(), "rb, ccs=UNICODE"));
	size_t doc_ID_size;
	fseek(file, pointer, 0);
	fread(&doc_ID_size, sizeof(size_t), 1, file);

	for (size_t i = 0; i < doc_ID_size; i++) {
		size_t doc_ID;
		long pointer1;

		fread(&doc_ID, sizeof(size_t), 1, file);
		fread(&pointer1, sizeof(long), 1, file);

		docs[doc_ID] = pointer1;
	}

	fclose(file);
}

std::vector<CompareType> op_intersect_pro(const CORPUS& Corpus, size_t word1_hash, size_t word2_hash,
	std::vector<CompareType>& l, std::vector<CompareType>& r, size_t k)
{
	std::set<CompareType> result;

	size_t i = 0, j = 0;

	term_docs_type docs1;
	term_docs_type docs2;

	getDocPos(Corpus, word1_hash, docs1);
	getDocPos(Corpus, word2_hash, docs2);

	FILE* pp1, *pp2;
	OPENMACROS(fopen_s(&pp1, (Corpus.coords_path + "_ID_POS").c_str(), "rb, ccs=UNICODE"));
	OPENMACROS(fopen_s(&pp2, (Corpus.coords_path + "_ID_POS").c_str(), "rb, ccs=UNICODE"));

	while (i < l.size() && j < r.size()) {
		if (l[i] == r[j]) {

			size_t size1, size2;
			
			getPos(Corpus, pp1, docs1, l[i].doc, size1);
			getPos(Corpus, pp2, docs2, r[j].doc, size2);

			size_t ii = 0, jj = 0;
			size_t pos_ii, pos_jj;
			
			pos_ii = Compressor::read_pos(pp1);
			pos_jj = Compressor::read_pos(pp2);

			while (ii < size1) {
				while (jj < size2) {
					if (0 < (pos_jj - pos_ii) and (pos_jj - pos_ii) <= k) {
						result.insert(l[i] + r[j]);
					}
					else {
						if (pos_jj > pos_ii)
							break;
					}

					pos_jj += Compressor::read_pos(pp2);
					jj++;
				}

				pos_ii += Compressor::read_pos(pp1);
				ii++;
			}

			i++;
			j++;
		}
		else if (l[i] < r[j]) {
			i++;
		}
		else {
			j++;
		}
	}

	fclose(pp1);
	fclose(pp2);

	return std::vector<CompareType>(result.begin(), result.end());
}

void range(std::wstring& query, std::vector<CompareType>& res, const CORPUS& Corpus) {
	if (res.empty()) { return; }

	std::sort(res.begin(), res.end(), [](const CompareType& a, const CompareType& b) {return a.rank > b.rank; });
}

std::vector<CompareType> parseCitation(std::wstring& citation, const CORPUS& Corpus)
{
	std::wstring last_word;
	std::stack<std::vector<CompareType>> result;
	std::wstring buff = L"";

	int i = 0;
	size_t k = 1;
	while (i <= citation.size()) {
		if ((i == citation.size()) or (citation[i] == ' ')) {
			if ((buff.size() != 0) and (result.size() == 0)) {
				last_word = buff;
				result.push(getDocs(Corpus, buff, 2));
				buff = L"";
			}
			if (buff.size()) {
				std::vector<CompareType> a1 = result.top();
				result.pop();
				std::vector<CompareType> a2 = getDocs(Corpus, buff, 2);
				size_t word1_hash = token_to_term(last_word);
				size_t word2_hash = token_to_term(buff);

				result.push(op_intersect_pro(Corpus, word1_hash, word2_hash, a1, a2, k));
				k = 1;
				last_word = buff;
				buff = L"";
			}
		}
		else if (citation[i] == '|') {
			k = 0;
			i++;
			while ('0' <= citation[i] and citation[i] <= '9') {
				k *= 10;
				k += (size_t)(citation[i] - '0');
				i++;
			}
			i--;
		}
		else {
			buff += citation[i];
		}
		i++;
	}

	return result.size() == 1 ? result.top() : std::vector<CompareType>();
}

std::vector<CompareType> parseBooling(std::wstring& query, const CORPUS& Corpus)
{
	std::stack<ELEM> stack;
	std::vector<std::vector<CompareType>> result;
	std::wstring buff = L"";

	bool last_token = false;
	bool citation = false;
	for (size_t i = 0; i <= query.size(); i++) {
		if (i == query.size()) { // Если достигли конца запроса
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
			while (!stack.empty()) {
				ELEM temp = stack.top();
				stack.pop();

				if (result.size() >= 2) {
					std::vector<CompareType> l, r;
					r = result[result.size() - 1];
					l = result[result.size() - 2];
					result.pop_back();
					result.pop_back();
					result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
				}
			}
		}
		else if (query[i] == L' ') { // Если встретился пробел
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
		}
		else if (query[i] == L'(') { // Если встретилась открывающая скобка
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
			if (last_token) { // & algo
				while (!stack.empty() and (stack.top().type == TYPE::OPAND or stack.top().type == TYPE::OPOR)) {
					ELEM temp = stack.top();
					stack.pop();

					if (result.size() >= 2) {
						std::vector<CompareType> l, r;
						r = result[result.size() - 1];
						l = result[result.size() - 2];
						result.pop_back();
						result.pop_back();
						result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
					}
				}

				stack.push({ TYPE::OPAND });
			}

			last_token = false;
			stack.push({ TYPE::OPENBRACKET });
		}
		else if (query[i] == L'&' or query[i] == L'|') { // Если встретился оператор и/или
			last_token = false;
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}

			while (!stack.empty() and (stack.top().type == TYPE::OPAND or stack.top().type == TYPE::OPOR)) {
				ELEM temp = stack.top();
				stack.pop();

				if (result.size() >= 2) {
					std::vector<CompareType> l, r;
					r = result[result.size() - 1];
					l = result[result.size() - 2];
					result.pop_back();
					result.pop_back();
					result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
				}
			}

			stack.push({ query[i] == L'&' ? TYPE::OPAND : TYPE::OPOR });

		}
		else if (query[i] == L')') { // Если встретилась закрывающая скобка
			last_token = true;
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
			while (stack.top().type != TYPE::OPENBRACKET) {
				ELEM temp = stack.top();
				stack.pop();

				if (result.size() >= 2) {
					std::vector<CompareType> l, r;
					r = result[result.size() - 1];
					l = result[result.size() - 2];
					result.pop_back();
					result.pop_back();
					result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
				}
			}
			stack.pop();
		}
		else if (query[i] == L'\"') {
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
			if (last_token) { // & algo
				while (!stack.empty() and (stack.top().type == TYPE::OPAND or stack.top().type == TYPE::OPOR)) {
					ELEM temp = stack.top();
					stack.pop();

					if (result.size() >= 2) {
						std::vector<CompareType> l, r;
						r = result[result.size() - 1];
						l = result[result.size() - 2];
						result.pop_back();
						result.pop_back();
						result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
					}
				}

				stack.push({ TYPE::OPAND });
			}

			i++;
			while (query[i] != L'\"') {
				buff += query[i];
				i++;
			}

			if (buff.size()) {
				result.push_back(parseCitation(buff, Corpus));
				buff = L"";
			}

			last_token = true;
		}
		else { // Если встретилась буква/символ
			if (last_token and (buff.size() == 0)) { // & algo
				while (!stack.empty() and (stack.top().type == TYPE::OPAND or stack.top().type == TYPE::OPOR)) {
					ELEM temp = stack.top();
					stack.pop();

					if (result.size() >= 2) {
						std::vector<CompareType> l, r;
						r = result[result.size() - 1];
						l = result[result.size() - 2];
						result.pop_back();
						result.pop_back();
						result.push_back(temp.type == TYPE::OPAND ? op_intersect(l, r) : op_union(l, r));
					}
				}

				stack.push({ TYPE::OPAND });
			}

			last_token = true;
			buff += query[i];
		}
	}

	return result.size() == 1 ? result[0] : std::vector<CompareType>();
}

std::vector<CompareType> parseFuzzy(std::wstring& query, const CORPUS& Corpus)
{
	std::stack<ELEM> stack;
	std::vector<std::vector<CompareType>> result;
	std::wstring buff = L"";

	bool last_token = false;
	for (size_t i = 0; i <= query.size(); i++) {
		if (i == query.size()) { // Если достигли конца запроса
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
			while (!stack.empty()) {
				ELEM temp = stack.top();
				stack.pop();

				if (result.size() >= 2) {
					std::vector<CompareType> l, r;
					r = result[result.size() - 1];
					l = result[result.size() - 2];
					result.pop_back();
					result.pop_back();
					result.push_back(op_union(l, r));
				}
			}
		}
		else if (query[i] == L' ') { // Если встретился пробел
			if (buff.size()) {
				result.push_back(getDocs(Corpus, buff));
				buff = L"";
			}
		}
		else { // Если встретилась буква/символ
			if (last_token and (buff.size() == 0)) { // & algo
				while (!stack.empty() and (stack.top().type == TYPE::OPOR)) {
					ELEM temp = stack.top();
					stack.pop();

					if (result.size() >= 2) {
						std::vector<CompareType> l, r;
						r = result[result.size() - 1];
						l = result[result.size() - 2];
						result.pop_back();
						result.pop_back();
						result.push_back(op_union(l, r));
					}
				}

				stack.push({ TYPE::OPOR });
			}

			last_token = true;
			buff += query[i];
		}
	}

	return result.size() == 1 ? result[0] : std::vector<CompareType>();
}

std::vector<size_t> parse(std::wstring& query, const CORPUS& Corpus)
{
	if (query.empty()) {
		return std::vector<size_t>();
	}

	bool fuzzy = true;
	for (size_t i = 0; i < query.size(); i++) {
		if (query[i] == L'|' or query[i] == L')' or query[i] == L'(' or query[i] == L'&' or query[i] == L'\"') {
			fuzzy = false;
			break;
		}
	}

	std::vector<CompareType> res;

	if (fuzzy) {
		res = parseFuzzy(query, Corpus);
	} else {
		res = parseBooling(query, Corpus);
	}

	range(query, res, Corpus);

	std::vector<size_t> result(res.size());
	for (size_t i = 0; i < res.size(); i++) {
		result[i] = res[i].doc;
	}

	return result;
}

int SERVER_RUN(const CORPUS& Corpus)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	CSC_SOCKET_ERROR((iResult != 0), true, "WSAStartup failed", 0);
		
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	CSC_SOCKET_ERROR((iResult != 0), true, "getaddrinfo failed", 8);

	// Create a SOCKET for the server to listen for client connections.
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	CSC_SOCKET_ERROR(ListenSocket, INVALID_SOCKET, "socket failed", 9);

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	CSC_SOCKET_ERROR(iResult, SOCKET_ERROR, "bind failed", 11);

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	CSC_SOCKET_ERROR(iResult, SOCKET_ERROR, "listen failed", 10);

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	CSC_SOCKET_ERROR(ClientSocket, INVALID_SOCKET, "accept failed", 10);

	// No longer need server socket
	closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {
		// read message length
		size_t message_length;
		iResult = recv(ClientSocket, (char*)(&message_length), 8, 0);

		if (iResult == 0) { printf("Connection closing...\n"); continue; }
		else if (iResult < 0) { CSC_SOCKET_ERROR(1, 1, "recv failed", 12); }

		std::cout << "Message length: " << message_length << std::endl;

		// read message
		size_t recvbuflen = sizeof(wchar_t) * (message_length + 1);
		wchar_t* recvbuf = new wchar_t[recvbuflen] {L'\0'}; //(wchar_t*)malloc(recvbuflen);
		CSC_SOCKET_ERROR(recvbuf, nullptr, "recvbuf malloc failed", 12);
		iResult = recv(ClientSocket, (char*)recvbuf, static_cast<int>(recvbuflen), 0);

		if (iResult == 0) { printf("Connection closing...\n"); continue; }
		else if (iResult < 0) { CSC_SOCKET_ERROR(1, 1, "recv failed", 12); }

		printf("Bytes received: %d\n", iResult);
		std::wstring message(message_length, '\0');
		for (size_t i = 1; i < message_length + 1; i++) message[i - 1] = recvbuf[i];
		delete[] recvbuf;

		std::wcout << "Message: " << message << std::endl;

		// searching documents
		auto start1 = std::chrono::high_resolution_clock::now();
		std::vector<size_t> answer_result = parse(message, Corpus);
		auto end1 = std::chrono::high_resolution_clock::now();
		double sendresult_time = std::chrono::duration<double, std::milli>(end1 - start1).count(); // millisec

		// log of first 20 docs
		std::cout << "Time: " << sendresult_time << std::endl;
		for (size_t i = 0; i < answer_result.size() and i < 20; i++) {
			std::cout << answer_result[i] << " ";
		} std::cout << std::endl;

		char tom1;
		size_t* sendresult = answer_result.data();
		size_t sendresult_size = answer_result.size() * sizeof(size_t);

		if (answer_result.size() == 0) {
			tom1 = 0;

			// founded or not
			iSendResult = send(ClientSocket, &tom1, 1, 0);
			CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
			printf("Bytes sent: %d\n", iSendResult);

			// time
			iSendResult = send(ClientSocket, (char*)(&sendresult_time), sizeof(double), 0);
			CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
			printf("Bytes sent: %d\n", iSendResult);
		}
		else {
			tom1 = 1;

			// founded or not
			iSendResult = send(ClientSocket, &tom1, 1, 0);
			CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
			printf("Bytes sent: %d\n", iSendResult);

			// time
			iSendResult = send(ClientSocket, (char*)(&sendresult_time), sizeof(double), 0);
			CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
			//printf("Bytes sent: %d\n", iSendResult);

			// count of bytes for docs
			iSendResult = send(ClientSocket, (char*)(&sendresult_size), sizeof(size_t), 0);
			CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
			//printf("Bytes sent: %d\n", iSendResult);

			// docs
			for (size_t i = 0; i < answer_result.size(); i++) {
				size_t docID = answer_result[i];
				std::wstring title = Corpus.straight_dictionary[docID].title;
				const wchar_t* buff = title.data();
				size_t title_size = title.size();

				iSendResult = send(ClientSocket, (char*)(&docID), sizeof(size_t), 0);
				CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
				//printf("Bytes sent: %d\n", iSendResult);

				iSendResult = send(ClientSocket, (char*)(&title_size), sizeof(size_t), 0);
				CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
				//printf("Bytes sent: %d\n", iSendResult);

				iSendResult = send(ClientSocket, (char*)buff, sizeof(wchar_t) * title_size, 0);
				CSC_SOCKET_ERROR(iSendResult, SOCKET_ERROR, "send failed", 12);
				//printf("Bytes sent: %d\n", iSendResult);
			}
		}
		
	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	CSC_SOCKET_ERROR(iResult, SOCKET_ERROR, "shutdown failed", 12);

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}

void CONSOLE_RUN(const CORPUS& Corpus)
{
	while (true) {
		std::wcout << L"Enter search query: ";
		std::wstring wenter;
		std::getline(std::wcin, wenter);

		if (wenter == L".quit") {
			break;
		}
		else {
			auto start1 = std::chrono::high_resolution_clock::now();
			std::vector<size_t> answer = parse(wenter, Corpus);
			auto end1 = std::chrono::high_resolution_clock::now();

			//std::cout << std::chrono::duration<double, std::milli>(end1 - start1).count() << " " << answer.size() << "\n";
			std::cout << "\n================\n" << "Summary searching time: " << std::chrono::duration<double, std::milli>(end1 - start1).count()
				<< "ms\nFiles founded: " << answer.size() << "\n================\n\n";

			if (answer.size()) {
				//vector_print(answer);

				for (size_t i = 0; i < answer.size() and i < 10; i++) {
					std::wcout << Corpus.straight_dictionary[answer[i]].title << " " << answer[i] << std::endl;
				}
			}
			else {
				std::cout << "There is no files for this query\n";
			}
		}
	}
}

enum MODE {
	CONSOLE = 0,
	WEB = 1,
};

int main(int argc, char* argv[]) 
{
	setlocale(LC_ALL, "Russian");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	std::string PathII, PathSI, PathCI, PathTG;
	MODE mode;

	switch (argc) {
	case 5:
		PathII = std::string(argv[1]);
		PathSI = std::string(argv[2]);
		PathCI = std::string(argv[3]);
		PathTG = std::string(argv[4]);
		mode = MODE::CONSOLE;
		std::cout << "Inverted index: " << PathII << std::endl;
		std::cout << "Straight index: " << PathSI << std::endl;
		std::cout << "Coordinate index: " << PathCI << std::endl;
		std::cout << "Trigram index: " << PathTG << std::endl;
		break;
	case 6:
		if (strcmp(argv[1], "--console") == 0) {
			mode = MODE::CONSOLE;
		}
		else if (strcmp(argv[1], "--web") == 0) {
			mode = MODE::WEB;
		}
		else {
			mode = MODE::CONSOLE;
		}

		PathII = std::string(argv[2]);
		PathSI = std::string(argv[3]);
		PathCI = std::string(argv[4]);
		PathTG = std::string(argv[5]);

		std::cout << "Inverted index: " << PathII << std::endl;
		std::cout << "Straight index: " << PathSI << std::endl;
		std::cout << "Coordinate index: " << PathCI << std::endl;
		std::cout << "Trigram index: " << PathTG << std::endl;
		break;
	default:
		std::cout << "Enter the path to files for indexing." << std::endl;
		return 0;
	}

	index_dictionary_type index_dictionary;
	trigrams_dictionary_type trigrams_dictionary;
	straight_dictionary_type straight_dictionary;
	coords_dictionary_type coords_dictionary;

	FILE* II, *SI, *CI, *TG;
	OPENMACROS(fopen_s(&II, PathII.c_str(), "rb, ccs=UNICODE")); //C:\Users\Igor\Desktop\инфопоиск\inverted_index_very_light.data
	OPENMACROS(fopen_s(&SI, PathSI.c_str(), "rb, ccs=UNICODE")); //C:\Users\Igor\Desktop\инфопоиск\straight_index_very_light.data
	OPENMACROS(fopen_s(&CI, PathCI.c_str(), "rb, ccs=UNICODE")); //C:\Users\Igor\Desktop\инфопоиск\coords_index_very_light.data
	OPENMACROS(fopen_s(&TG, PathTG.c_str(), "rb, ccs=UNICODE")); //C:\Users\Igor\Desktop\инфопоиск\trigram_index_very_light.data

	termInfo TI;

	while (fread(&TI, sizeof(termInfo), 1, II) > 0) {
		index_dictionary[TI.hash] = TI.pointer;
	}

	printf("Количество термов: %llu\n", index_dictionary.size());

	trigInfo TGI;

	while (fread(&TGI, sizeof(trigInfo), 1, TG) > 0) {
		trigrams_dictionary[TGI.trig] = TGI.pointer;
	}

	while (!feof(CI)) {
		size_t hash = 0;
		long pointer = 0;

		if (fread(&hash, sizeof(size_t), 1, CI) == 0) break;
		fread(&pointer, sizeof(long), 1, CI);

		coords_dictionary[hash] = {pointer};
	}

	while (!feof(SI)) {
		size_t docID;
		size_t titleSize;
		wchar_t title[MAXSTRINGSIZE] = { L'\0' };
		size_t count_of_words;
		long p;

		if (fread(&docID, sizeof(size_t), 1, SI) == 0) break;
		fread(&titleSize, sizeof(size_t), 1, SI);
		if (sizeof(wchar_t) * titleSize > sizeof(title)) {
			std::cout << "Title more than 300 symbols\n";
			exit(0);
		}
		fread(title, sizeof(wchar_t), titleSize, SI);
		fread(&count_of_words, sizeof(size_t), 1, SI);
		fread(&p, sizeof(long), 1, SI);

		straight_dictionary[docID] = { std::wstring(title, titleSize), count_of_words };
	}

	fclose(II);
	fclose(SI);
	fclose(CI);
	fclose(TG);

	CORPUS Corpus = { PathII, PathTG, PathSI, PathCI, index_dictionary, trigrams_dictionary, straight_dictionary, coords_dictionary };

	std::cout << ((mode == MODE::CONSOLE) ? "Console mode\n" : "Web-service mode\n");

	if (mode == MODE::WEB) {
		SERVER_RUN(Corpus);
	}
	else if (mode == MODE::CONSOLE) {
		CONSOLE_RUN(Corpus);
	}
	
	return 0;
}