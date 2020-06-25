#pragma once
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>
#include <iostream>

//#define UNICODE_SUPPORT
#include "Helpers.h"


// SymSpell: 1 million times faster through Symmetric Delete spelling correction algorithm
//
// The Symmetric Delete spelling correction algorithm reduces the complexity of edit candidate generation and dictionary lookup
// for a given Damerau-Levenshtein distance. It is six orders of magnitude faster and language independent.
// Opposite to other algorithms only deletes are required, no transposes + replaces + inserts.
// Transposes + replaces + inserts of the input term are transformed into deletes of the dictionary term.
// Replaces and inserts are expensive and language dependent: e.g. Chinese has 70,000 Unicode Han characters!
//
// SymSpell supports compound splitting / decompounding of multi-word input strings with three cases:
// 1. mistakenly inserted space into a correct word led to two incorrect terms 
// 2. mistakenly omitted space between two correct words led to one incorrect combined term
// 3. multiple independent input terms with/without spelling errors

// Copyright (C) 2019 Wolf Garbe
// Version: 6.5
// Author: Wolf Garbe wolf.garbe@faroo.com
// Maintainer: Wolf Garbe wolf.garbe@faroo.com
// URL: https://github.com/wolfgarbe/symspell
// Description: https://medium.com/@wolfgarbe/1000x-faster-spelling-correction-algorithm-2012-8701fcd87a5f
//
// MIT License
// Copyright (c) 2019 Wolf Garbe
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// https://opensource.org/licenses/MIT

#define DEFAULT_SEPARATOR_CHAR XL('\t')
#define DEFAULT_MAX_EDIT_DISTANCE 2
#define DEFAULT_PREFIX_LENGTH 7
#define DEFAULT_COUNT_THRESHOLD 1
#define DEFAULT_INITIAL_CAPACITY 82765
#define DEFAULT_COMPACT_LEVEL 5
#define min3(a, b, c) (min(a, min(b, c)))
#define MAXINT LLONG_MAX
#define M
#define MAXLONG MAXINT
#define uint unsigned int
static inline void ltrim(xstring& s)
{
	
	s.erase(s.begin(),find_if(s.begin(), s.end(), [](xchar ch){
		return !isxspace(ch);
	}));
}

static inline void rtrim(xstring& s)
{
	s.erase(find_if(s.rbegin(), s.rend(), [](xchar ch){
		return !isxspace(ch);
	}).base(), s.end());
}

static inline void trim(xstring& s)
{
	ltrim(s);
	rtrim(s);
}

class Info
{
private:

	xstring segmentedstring;
	xstring correctedstring;
	int distanceSum;
	double probabilityLogSum;
public:
	void set(xstring& seg, xstring& cor, int d, double prob)
	{
		segmentedstring = seg;
		correctedstring = cor;
		distanceSum = d;
		probabilityLogSum = prob;
	};
	xstring getSegmented() 
	{
		return segmentedstring;
	};
	xstring getCorrected() 
	{
		return correctedstring;
	};
	int getDistance()
	{
		return distanceSum;
	};
	double getProbability()
	{
		return probabilityLogSum;
	};
};

/// <summary>Controls the closeness/quantity of returned spelling suggestions.</summary>
enum Verbosity
{
	/// <summary>Top suggestion with the highest term frequency of the suggestions of smallest edit distance found.</summary>
	Top,
	/// <summary>All suggestions of smallest edit distance found, suggestions ordered by term frequency.</summary>
	Closest,
	/// <summary>All suggestions within maxEditDistance, suggestions ordered by edit distance
	/// , then by term frequency (slower, no early termination).</summary>
	All
};

class SymSpell
{
private:
	int initialCapacity;
	int maxDictionaryEditDistance;
	int prefixLength; //prefix length  5..7
	long countThreshold; //a threshold might be specified, when a term occurs so frequently in the corpus that it is considered a valid word for spelling correction
	int compactMask;
	DistanceAlgorithm distanceAlgorithm = DistanceAlgorithm::DamerauOSADistance;
	int maxDictionaryWordLength; //maximum dictionary term length
	// Dictionary that contains a mapping of lists of suggested correction words to the hashCodes
	// of the original words and the deletes derived from them. Collisions of hashCodes is tolerated,
	// because suggestions are ultimately verified via an edit distance function.
	// A list of suggestions might have a single suggestion, or multiple suggestions. 
	Dictionary<int, vector<xstring>> *deletes = NULL;
	// Dictionary of unique correct spelling words, and the frequency count for each word.
	Dictionary<xstring, int64_t> words;
	// Dictionary of unique words that are below the count threshold for being considered correct spellings.
	Dictionary<xstring, int64_t> belowThresholdWords;

public:
	/// <summary>Maximum edit distance for dictionary precalculation.</summary>
	int MaxDictionaryEditDistance();

		/// <summary>Length of prefix, from which deletes are generated.</summary>
	int PrefixLength();

		/// <summary>Length of longest word in the dictionary.</summary>
	int MaxLength();

		/// <summary>Count threshold for a word to be considered a valid word for spelling correction.</summary>
	long CountThreshold();

		/// <summary>Number of unique words in the dictionary.</summary>
	int WordCount();

		/// <summary>Number of word prefixes and intermediate word deletes encoded in the dictionary.</summary>
	int EntryCount();

		/// <summary>Create a new instanc of SymSpell.</summary>
		/// <remarks>Specifying ann accurate initialCapacity is not essential, 
		/// but it can help speed up processing by alleviating the need for 
		/// data restructuring as the size grows.</remarks>
		/// <param name="initialCapacity">The expected number of words in dictionary.</param>
		/// <param name="maxDictionaryEditDistance">Maximum edit distance for doing lookups.</param>
		/// <param name="prefixLength">The length of word prefixes used for spell checking..</param>
		/// <param name="countThreshold">The minimum frequency count for dictionary words to be considered correct spellings.</param>
		/// <param name="compactLevel">Degree of favoring lower memory use over speed (0=fastest,most memory, 16=slowest,least memory).</param>
	
	SymSpell(int initialCapacity = DEFAULT_INITIAL_CAPACITY, int maxDictionaryEditDistance = DEFAULT_MAX_EDIT_DISTANCE
		, int prefixLength = DEFAULT_PREFIX_LENGTH, int countThreshold = DEFAULT_COUNT_THRESHOLD
		, unsigned char compactLevel = DEFAULT_COMPACT_LEVEL);

	/// <summary>Create/Update an entry in the dictionary.</summary>
	/// <remarks>For every word there are deletes with an edit distance of 1..maxEditDistance created and added to the
	/// dictionary. Every delete entry has a suggestions list, which points to the original term(s) it was created from.
	/// The dictionary may be dynamically updated (word frequency and new words) at any time by calling CreateDictionaryEntry</remarks>
	/// <param name="key">The word to add to dictionary.</param>
	/// <param name="count">The frequency count for word.</param>
	/// <param name="staging">Optional staging object to speed up adding many entries by staging them to a temporary structure.</param>
	/// <returns>True if the word was added as a new correctly spelled word,
	/// or false if the word is added as a below threshold word, or updates an
	/// existing correctly spelled word.</returns>
	bool CreateDictionaryEntry(xstring key, int64_t count, SuggestionStage* staging);

	Dictionary<xstring, long> bigrams;
	int64_t bigramCountMin = MAXLONG;

	/// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpus">The path+filename of the file.</param>
	/// <param name="termIndex">The column position of the word.</param>
	/// <param name="countIndex">The column position of the frequency count.</param>
	/// <param name="separatorChars">Separator characters between term(s) and count.</param>
	/// <returns>True if file loaded, or false if file not found.</returns>
	bool LoadBigramDictionary(string corpus, int termIndex, int countIndex, xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

	/// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpus">The path+filename of the file.</param>
	/// <param name="termIndex">The column position of the word.</param>
	/// <param name="countIndex">The column position of the frequency count.</param>
	/// <param name="separatorChars">Separator characters between term(s) and count.</param>
	/// <returns>True if file loaded, or false if file not found.</returns>
	bool LoadBigramDictionary(xifstream& corpusStream, int termIndex, int countIndex, xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

	/// <summary>Load multiple dictionary entries from a file of word/frequency count pairs</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpus">The path+filename of the file.</param>
	/// <param name="termIndex">The column position of the word.</param>
	/// <param name="countIndex">The column position of the frequency count.</param>
	/// <param name="separatorChars">Separator characters between term(s) and count.</param>
	/// <returns>True if file loaded, or false if file not found.</returns>
	bool LoadDictionary(string corpus, int termIndex, int countIndex, xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

	/// <summary>Load multiple dictionary entries from a stream of word/frequency count pairs</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpusStream">The stream containing the word/frequency count pairs.</param>
	/// <param name="termIndex">The column position of the word.</param>
	/// <param name="countIndex">The column position of the frequency count.</param>
	/// <param name="separatorChars">Separator characters between term(s) and count.</param>
	/// <returns>True if stream loads.</returns>
	bool LoadDictionary(xifstream& corpusStream, int termIndex, int countIndex, xchar separatorChars = DEFAULT_SEPARATOR_CHAR);

	/// <summary>Load multiple dictionary words from a file containing plain text.</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpus">The path+filename of the file.</param>
	/// <returns>True if file loaded, or false if file not found.</returns>
	bool CreateDictionary(string corpus);

	/// <summary>Load multiple dictionary words from a stream containing plain text.</summary>
	/// <remarks>Merges with any dictionary data already loaded.</remarks>
	/// <param name="corpusStream">The stream containing the plain text.</param>
	/// <returns>True if stream loads.</returns>
	bool CreateDictionary(xifstream& corpusStream);

	/// <summary>Remove all below threshold words from the dictionary.</summary>
	/// <remarks>This can be used to reduce memory consumption after populating the dictionary from
	/// a corpus using CreateDictionary.</remarks>
	void PurgeBelowThresholdWords();

	/// <summary>Commit staged dictionary additions.</summary>
	/// <remarks>Used when you write your own process to load multiple words into the
	/// dictionary, and as part of that process, you first created a SuggestionsStage 
	/// object, and passed that to CreateDictionaryEntry calls.</remarks>
	/// <param name="staging">The SuggestionStage object storing the staged data.</param>
	void CommitStaged(SuggestionStage* staging);

	/// <summary>Find suggested spellings for a given input word, using the maximum
	/// edit distance specified during construction of the SymSpell dictionary.</summary>
	/// <param name="input">The word being spell checked.</param>
	/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
	/// <returns>A List of SuggestItem object representing suggested correct spellings for the input word, 
	/// sorted by edit distance, and secondarily by count frequency.</returns>
	vector<SuggestItem> Lookup(xstring input, Verbosity verbosity);

	/// <summary>Find suggested spellings for a given input word, using the maximum
	/// edit distance specified during construction of the SymSpell dictionary.</summary>
	/// <param name="input">The word being spell checked.</param>
	/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
	/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
	/// <returns>A List of SuggestItem object representing suggested correct spellings for the input word, 
	/// sorted by edit distance, and secondarily by count frequency.</returns>
	vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance);

	/// <summary>Find suggested spellings for a given input word.</summary>
	/// <param name="input">The word being spell checked.</param>
	/// <param name="verbosity">The value controlling the quantity/closeness of the retuned suggestions.</param>
	/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>
	/// <param name="includeUnknown">Include input word in suggestions, if no words within edit distance found.</param>																													   
	/// <returns>A List of SuggestItem object representing suggested correct spellings for the input word, 
	/// sorted by edit distance, and secondarily by count frequency.</returns>
	vector<SuggestItem> Lookup(xstring input, Verbosity verbosity, int maxEditDistance, bool includeUnknown);

private:
	//check whether all delete chars are present in the suggestion prefix in correct order, otherwise this is just a hash collision
	bool DeleteInSuggestionPrefix(xstring deleteSugg, int deleteLen, xstring suggestion, int suggestionLen);

	//create a non-unique wordlist from sample text
	//language independent (e.g. works with Chinese characters)
	vector<xstring> ParseWords(xstring text);

	//inexpensive and language independent: only deletes, no transposes + replaces + inserts
	//replaces and inserts are expensive and language dependent (Chinese has 70,000 Unicode Han characters)
	HashSet<xstring>* Edits(xstring word, int editDistance, HashSet<xstring>* deleteWords);

	HashSet<xstring> EditsPrefix(xstring key);

	int GetstringHash(xstring s);

public:
	//######################

	//LookupCompound supports compound aware automatic spelling correction of multi-word input strings with three cases:
	//1. mistakenly inserted space into a correct word led to two incorrect terms 
	//2. mistakenly omitted space between two correct words led to one incorrect combined term
	//3. multiple independent input terms with/without spelling errors

	/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
	/// <param name="input">The string being spell checked.</param>																										   
	/// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns> 
	vector<SuggestItem> LookupCompound(xstring input);

	/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
	/// <param name="input">The string being spell checked.</param>
	/// <param name="maxEditDistance">The maximum edit distance between input and suggested words.</param>																											   
	/// <returns>A List of SuggestItem object representing suggested correct spellings for the input string.</returns> 
	vector<SuggestItem> LookupCompound(xstring input, int editDistanceMax);

	//######

	//WordSegmentation divides a string into words by inserting missing spaces at the appropriate positions
	//misspelled words are corrected and do not affect segmentation
	//existing spaces are allowed and considered for optimum segmentation

	//SymSpell.WordSegmentation uses a novel approach *without* recursion.
	//https://medium.com/@wolfgarbe/fast-word-segmentation-for-noisy-text-2c2c41f9e8da
	//While each string of length n can be segmentend in 2^n−1 possible compositions https://en.wikipedia.org/wiki/Composition_(combinatorics)
	//SymSpell.WordSegmentation has a linear runtime O(n) to find the optimum composition

	//number of all words in the corpus used to generate the frequency dictionary
	//this is used to calculate the word occurrence probability p from word counts c : p=c/N
	//N equals the sum of all counts c in the dictionary only if the dictionary is complete, but not if the dictionary is truncated or filtered
	static const int64_t N = 1024908267229L;

	/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
	/// <param name="input">The string being spell checked.</param>
	/// <returns>The word segmented string, 
	/// the word segmented and spelling corrected string, 
	/// the Edit distance sum between input string and corrected string, 
	/// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns> 
	Info WordSegmentation(xstring input);

	/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
	/// <param name="input">The string being spell checked.</param>
	/// <param name="maxEditDistance">The maximum edit distance between input and corrected words 
	/// (0=no correction/segmentation only).</param>	
	/// <returns>The word segmented string, 
	/// the word segmented and spelling corrected string, 
	/// the Edit distance sum between input string and corrected string, 
	/// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns> 
	Info WordSegmentation(xstring input, int maxEditDistance);

	/// <summary>Find suggested spellings for a multi-word input string (supports word splitting/merging).</summary>
	/// <param name="input">The string being spell checked.</param>
	/// <param name="maxSegmentationWordLength">The maximum word length that should be considered.</param>	
	/// <param name="maxEditDistance">The maximum edit distance between input and corrected words 
	/// (0=no correction/segmentation only).</param>	
	/// <returns>The word segmented string, 
	/// the word segmented and spelling corrected string, 
	/// the Edit distance sum between input string and corrected string, 
	/// the Sum of word occurrence probabilities in log scale (a measure of how common and probable the corrected segmentation is).</returns> 
	Info WordSegmentation(xstring input, int maxEditDistance, int maxSegmentationWordLength);
};

