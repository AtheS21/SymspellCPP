#pragma once

#ifdef _WIN32
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API __declspec(dllexport)
#    else
#        define LIBRARY_API __declspec(dllimport)
#    endif
#elif
#    define LIBRARY_API
#endif

#define Dictionary unordered_map
#define HashSet unordered_set
#ifdef UNICODE_SUPPORT
#	define xstring wstring
#	define xchar wchar_t
#	define xifstream wifstream
#	define xstringstream wstringstream
#	define isxspace iswspace
#	define xregex wregex
#	define xsmatch wsmatch
#	define to_xstring to_wstring
#	define XL(x) L##x
#	define xcout wcout
#else
#	define xstring string
#	define xchar char
#	define xifstream ifstream
#	define xstringstream stringstream
#	define isxspace isspace
#	define xregex regex
#	define xsmatch smatch
#	define to_xstring to_string
#	define XL(x) x
#	define xcout cout
#endif

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <math.h>
#include <limits.h>
#include <functional>
using namespace std;


// A growable list of elements that's optimized to support adds, but not deletes,
// of large numbers of elements, storing data in a way that's friendly to the garbage
// collector (not backed by a monolithic array object), and can grow without needing
// to copy the entire backing array contents from the old backing array to the new.
template <class T>
class ChunkArray
{
private:
	const int ChunkSize = 4096; //this must be a power of 2, otherwise can't optimize Row and Col functions
	const int DivShift = 12; // number of bits to shift right to do division by ChunkSize (the bit position of ChunkSize)
	int Row(unsigned int index) { return index >> DivShift; } // same as index / ChunkSize
	int Col(unsigned int  index) { return index & (ChunkSize - 1); } //same as index % ChunkSize
	int Capacity() { return Values.size() * ChunkSize; }

public:
	vector<vector<T>> Values;
	int Count;

	ChunkArray()
	{
		Count = 0;
	}

	void Reserve(int initialCapacity)
	{
		int chunks = (initialCapacity + ChunkSize - 1) / ChunkSize;
		Values.resize(chunks);
		for (int i = 0; i < chunks; ++i)
		{
			Values[i].resize(ChunkSize);
		}
	}

	int Add(T &value)
	{
		if (Count == Capacity())
		{
			Values.push_back(vector<T>());
			Values[Values.size() - 1].resize(ChunkSize);
		}

		int row = Row(Count);
		int col = Col(Count);

		Values[row][col] = value;
		return Count++;
	}

	void Clear()
	{
		Count = 0;
	}

	T& At(unsigned int index)
	{
		return Values[Row(index)][Col(index)];
	}

	void Set(unsigned int index, T &value)
	{
		Values[Row(index)][Col(index)] = value;
	}
};

class Helpers {
public:
	/// <summary>Determines the proper return value of an edit distance function when one or
	/// both strings are null.</summary>
	static int NullDistanceResults(xstring string1, xstring string2, double maxDistance) {
		if (string1.empty())
			return (string2.empty()) ? 0 : (string2.size() <= maxDistance) ? string2.size() : -1;
		return (string1.size() <= maxDistance) ? string1.size() : -1;
	}

	/// <summary>Determines the proper return value of an similarity function when one or
	/// both strings are null.</summary>
	static int NullSimilarityResults(xstring string1, xstring string2, double minSimilarity) {
		return (string1.empty() && string2.empty()) ? 1 : (0 <= minSimilarity) ? 0 : -1;
	}

	/// <summary>Calculates starting position and lengths of two strings such that common
	/// prefix and suffix substrings are excluded.</summary>
	/// <remarks>Expects string1.size() to be less than or equal to string2.size()</remarks>
	static void PrefixSuffixPrep(xstring string1, xstring string2, int &len1, int &len2, int &start) {
		len2 = string2.size();
		len1 = string1.size(); // this is also the minimum length of the two strings
		// suffix common to both strings can be ignored
		while (len1 != 0 && string1[len1 - 1] == string2[len2 - 1]) {
			len1 = len1 - 1; len2 = len2 - 1;
		}
		// prefix common to both strings can be ignored
		start = 0;
		while (start != len1 && string1[start] == string2[start]) start++;
		if (start != 0) {
			len2 -= start; // length of the part excluding common prefix and suffix
			len1 -= start;
		}
	}

	/// <summary>Calculate a similarity measure from an edit distance.</summary>
	/// <param name="length">The length of the longer of the two strings the edit distance is from.</param>
	/// <param name="distance">The edit distance between two strings.</param>
	/// <returns>A similarity value from 0 to 1.0 (1 - (length / distance)).</returns>
	static double ToSimilarity(int distance, int length) {
		return (distance < 0) ? -1 : 1 - (distance / (double)length);
	}

	/// <summary>Calculate an edit distance from a similarity measure.</summary>
	/// <param name="length">The length of the longer of the two strings the edit distance is from.</param>
	/// <param name="similarity">The similarity measure between two strings.</param>
	/// <returns>An edit distance from 0 to length (length * (1 - similarity)).</returns>
	static int ToDistance(double similarity, int length) {
		return (int)((length * (1 - similarity)) + .0000000001);
	}

	/// <summary>
	/// CompareTo of two intergers
	/// </summary>
	static int CompareTo(int64_t mainValue, int64_t compareValue)
	{
		if (mainValue == compareValue)
			return 0;
		else if (mainValue > compareValue)
			return 1;
		else
			return -1;
	}
};


/// <summary>Types implementing the IDistance interface provide methods
/// for computing a relative distance between two strings.</summary>
class IDistance {
public:
	/// <summary>Return a measure of the distance between two strings.</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>0 if the strings are equivalent, otherwise a positive number whose
	/// magnitude increases as difference between the strings increases.</returns>
	virtual double Distance(xstring string1, xstring string2) = 0;

	/// <summary>Return a measure of the distance between two strings.</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="maxDistance">The maximum distance that is of interest.</param>
	/// <returns>-1 if the distance is greater than the maxDistance, 0 if the strings
	/// are equivalent, otherwise a positive number whose magnitude increases as
	/// difference between the strings increases.</returns>
	virtual double Distance(xstring string1, xstring string2, double maxDistance) = 0;
};


/// <summary>Types implementing the ISimilarity interface provide methods
/// for computing a normalized value of similarity between two strings.</summary>
class ISimilarity {
public:
	/// <summary>Return a measure of the similarity between two strings.</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>The degree of similarity 0 to 1.0, where 0 represents a lack of any
	/// notable similarity, and 1 represents equivalent strings.</returns>
	virtual double Similarity(xstring string1, xstring string2) = 0;

	/// <summary>Return a measure of the similarity between two strings.</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="minSimilarity">The minimum similarity that is of interest.</param>
	/// <returns>The degree of similarity 0 to 1.0, where -1 represents a similarity
	/// lower than minSimilarity, otherwise, a number between 0 and 1.0 where 0
	/// represents a lack of any notable similarity, and 1 represents equivalent
	/// strings.</returns>
	virtual double Similarity(xstring string1, xstring string2, double minSimilarity) = 0;
};

/// <summary>
/// Class providing optimized methods for computing Damerau-Levenshtein Optimal string
/// Alignment (OSA) comparisons between two strings.
/// </summary>
/// <remarks>
/// Copyright ©2015-2018 SoftWx, Inc.
/// The inspiration for creating highly optimized edit distance functions was 
/// from Sten Hjelmqvist's "Fast, memory efficient" algorithm, described at
/// http://www.codeproject.com/Articles/13525/Fast-memory-efficient-Levenshtein-algorithm
/// The Damerau-Levenshtein algorithm is basically the Levenshtein algorithm with a 
/// modification that considers transposition of two adjacent characters as a single edit.
/// The optimized algorithm was described in detail in my post at 
/// http://blog.softwx.net/2015/01/optimizing-damerau-levenshtein_15.html
/// Also see http://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance
/// Note that this implementation of Damerau-Levenshtein is the simpler and faster optimal
/// string alignment (aka restricted edit) distance that difers slightly from the classic
/// algorithm by imposing the restriction that no substring is edited more than once. So,
/// for example, "CA" to "ABC" has an edit distance of 2 by a complete application of
/// Damerau-Levenshtein, but has a distance of 3 by the method implemented here, that uses
/// the optimal string alignment algorithm. This means that this algorithm is not a true
/// metric since it does not uphold the triangle inequality. In real use though, this OSA
/// version may be desired. Besides being faster, it does not give the lower distance score
/// for transpositions that occur across long distances. Actual human error transpositions
/// are most likely for adjacent characters. For example, the classic Damerau algorithm 
/// gives a distance of 1 for these two strings: "sated" and "dates" (it counts the 's' and
/// 'd' as a single transposition. The optimal string alignment version of Damerau in this
/// class gives a distance of 2 for these two strings (2 substitutions), as it only counts
/// transpositions for adjacent characters.
/// The methods in this class are not threadsafe. Use the static versions in the Distance
/// class if that is required.</remarks>
class DamerauOSA : public IDistance {
private:
	vector<int> baseChar1Costs;
	vector<int> basePrevChar1Costs;

public:
	/// <summary>Create a new instance of DamerauOSA.</summary>
	DamerauOSA() {
	}

	/// <summary>Create a new instance of DamerauOSA using the specified expected
	/// maximum string length that will be encountered.</summary>
	/// <remarks>By specifying the max expected string length, better memory efficiency
	/// can be achieved.</remarks>
	/// <param name="expectedMaxstringLength">The expected maximum length of strings that will
	/// be passed to the edit distance functions.</param>
	DamerauOSA(int expectedMaxstringLength) {
		if (expectedMaxstringLength <= 0) throw "expectedMaxstringLength must be larger than 0";
		this->baseChar1Costs = vector<int>(expectedMaxstringLength, 0);
		this->basePrevChar1Costs = vector<int>(expectedMaxstringLength, 0);
	}

	/// <summary>Compute and return the Damerau-Levenshtein optimal string
	/// alignment edit distance between two strings.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match
	/// This method is not threadsafe.</remarks>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>0 if the strings are equivalent, otherwise a positive number whose
	/// magnitude increases as difference between the strings increases.</returns>
	double Distance(xstring string1, xstring string2) {
		if (string1.empty()) return string2.size();
		if (string2.empty()) return string1.size();

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return len2;

		if (len2 > this->baseChar1Costs.size()) {
			this->baseChar1Costs = vector<int>(len2, 0);
			this->basePrevChar1Costs = vector<int>(len2, 0);
		}
		return Distance(string1, string2, len1, len2, start, this->baseChar1Costs, this->basePrevChar1Costs);
	}

	/// <summary>Compute and return the Damerau-Levenshtein optimal string
	/// alignment edit distance between two strings.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match
	/// This method is not threadsafe.</remarks>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="maxDistance">The maximum distance that is of interest.</param>
	/// <returns>-1 if the distance is greater than the maxDistance, 0 if the strings
	/// are equivalent, otherwise a positive number whose magnitude increases as
	/// difference between the strings increases.</returns>
	double Distance(xstring string1, xstring string2, double maxDistance) {
		if (string1.empty() || string2.empty()) return Helpers::NullDistanceResults(string1, string2, maxDistance);
		if (maxDistance <= 0) return (string1 == string2) ? 0 : -1;
		maxDistance = ceil(maxDistance);
		int iMaxDistance = (maxDistance <= INT_MAX) ? (int)maxDistance : INT_MAX;

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }
		if (string2.size() - string1.size() > iMaxDistance) return -1;

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return (len2 <= iMaxDistance) ? len2 : -1;

		if (len2 > this->baseChar1Costs.size()) {
			this->baseChar1Costs = vector<int>(len2, 0);
			this->basePrevChar1Costs = vector<int>(len2, 0);
		}
		if (iMaxDistance < len2) {
			return Distance(string1, string2, len1, len2, start, iMaxDistance, this->baseChar1Costs, this->basePrevChar1Costs);
		}
		return Distance(string1, string2, len1, len2, start, this->baseChar1Costs, this->basePrevChar1Costs);
	}

	/// <summary>Return Damerau-Levenshtein optimal string alignment similarity
	/// between two strings (1 - (damerau distance / len of longer string)).</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>The degree of similarity 0 to 1.0, where 0 represents a lack of any
	/// noteable similarity, and 1 represents equivalent strings.</returns>
	double Similarity(xstring string1, xstring string2) {
		if (string1.empty()) return (string2.empty()) ? 1 : 0;
		if (string2.empty()) return 0;

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return 1.0;

		if (len2 > this->baseChar1Costs.size()) {
			this->baseChar1Costs = vector<int>(len2, 0);
			this->basePrevChar1Costs = vector<int>(len2, 0);
		}
		return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start, this->baseChar1Costs, this->basePrevChar1Costs)
			, string2.size());
	}

	/// <summary>Return Damerau-Levenshtein optimal string alignment similarity
	/// between two strings (1 - (damerau distance / len of longer string)).</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="minSimilarity">The minimum similarity that is of interest.</param>
	/// <returns>The degree of similarity 0 to 1.0, where -1 represents a similarity
	/// lower than minSimilarity, otherwise, a number between 0 and 1.0 where 0
	/// represents a lack of any noteable similarity, and 1 represents equivalent
	/// strings.</returns>
	double Similarity(xstring string1, xstring string2, double minSimilarity) {
		if (minSimilarity < 0 || minSimilarity > 1) throw "minSimilarity must be in range 0 to 1.0";
		if (string1.empty() || string2.empty()) return Helpers::NullSimilarityResults(string1, string2, minSimilarity);

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		int iMaxDistance = Helpers::ToDistance(minSimilarity, string2.size());
		if (string2.size() - string1.size() > iMaxDistance) return -1;
		if (iMaxDistance <= 0) return (string1 == string2) ? 1 : -1;

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return 1.0;

		if (len2 > this->baseChar1Costs.size()) {
			this->baseChar1Costs = vector<int>(len2, 0);
			this->basePrevChar1Costs = vector<int>(len2, 0);
		}
		if (iMaxDistance < len2) {
			return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start, iMaxDistance, this->baseChar1Costs, this->basePrevChar1Costs)
				, string2.size());
		}
		return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start, this->baseChar1Costs, this->basePrevChar1Costs)
			, string2.size());
	}

	/// <summary>Internal implementation of the core Damerau-Levenshtein, optimal string alignment algorithm.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match</remarks>
	static int Distance(xstring string1, xstring string2, int len1, int len2, int start, vector<int> char1Costs, vector<int> prevChar1Costs) {
		int j;
		for (j = 0; j < len2; j++) char1Costs[j] = j + 1;
		xchar char1 = XL(' ');
		int currentCost = 0;
		for (int i = 0; i < len1; ++i) {
			xchar prevChar1 = char1;
			char1 = string1[start + i];
			xchar char2 = XL(' ');
			int leftCharCost, aboveCharCost;
			leftCharCost = aboveCharCost = i;
			int nextTransCost = 0;
			for (j = 0; j < len2; ++j) {
				int thisTransCost = nextTransCost;
				nextTransCost = prevChar1Costs[j];
				prevChar1Costs[j] = currentCost = leftCharCost; // cost of diagonal (substitution)
				leftCharCost = char1Costs[j];    // left now equals current cost (which will be diagonal at next iteration)
				xchar prevChar2 = char2;
				char2 = string2[start + j];
				if (char1 != char2) {
					//substitution if neither of two conditions below
					if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
					if (leftCharCost < currentCost) currentCost = leftCharCost;   // insertion
					++currentCost;
					if ((i != 0) && (j != 0)
						&& (char1 == prevChar2)
						&& (prevChar1 == char2)
						&& (thisTransCost + 1 < currentCost)) {
						currentCost = thisTransCost + 1; // transposition
					}
				}
				char1Costs[j] = aboveCharCost = currentCost;
			}
		}
		return currentCost;
	}

	/// <summary>Internal implementation of the core Damerau-Levenshtein, optimal string alignment algorithm
	/// that accepts a maxDistance.</summary>
	static int Distance(xstring string1, xstring string2, int len1, int len2, int start, int maxDistance, vector<int> char1Costs, vector<int> prevChar1Costs) {
		int i, j;
		//for (j = 0; j < maxDistance;) char1Costs[j] = ++j;
		for (j = 0; j < maxDistance; j++)
			char1Costs[j] = j + 1;
		for (; j < len2;) char1Costs[j++] = maxDistance + 1;
		int lenDiff = len2 - len1;
		int jStartOffset = maxDistance - lenDiff;
		int jStart = 0;
		int jEnd = maxDistance;
		xchar char1 = XL(' ');
		int currentCost = 0;
		for (i = 0; i < len1; ++i) {
			xchar prevChar1 = char1;
			char1 = string1[start + i];
			xchar char2 = XL(' ');
			int leftCharCost, aboveCharCost;
			leftCharCost = aboveCharCost = i;
			int nextTransCost = 0;
			// no need to look beyond window of lower right diagonal - maxDistance cells (lower right diag is i - lenDiff)
			// and the upper left diagonal + maxDistance cells (upper left is i)
			jStart += (i > jStartOffset) ? 1 : 0;
			jEnd += (jEnd < len2) ? 1 : 0;
			for (j = jStart; j < jEnd; ++j) {
				int thisTransCost = nextTransCost;
				nextTransCost = prevChar1Costs[j];
				prevChar1Costs[j] = currentCost = leftCharCost; // cost on diagonal (substitution)
				leftCharCost = char1Costs[j];     // left now equals current cost (which will be diagonal at next iteration)
				xchar prevChar2 = char2;
				char2 = string2[start + j];
				if (char1 != char2) {
					// substitution if neither of two conditions below
					if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
					if (leftCharCost < currentCost) currentCost = leftCharCost;   // insertion
					++currentCost;
					if ((i != 0) && (j != 0)
						&& (char1 == prevChar2)
						&& (prevChar1 == char2)
						&& (thisTransCost + 1 < currentCost)) {
						currentCost = thisTransCost + 1; // transposition
					}
				}
				char1Costs[j] = aboveCharCost = currentCost;
			}
			if (char1Costs[i + lenDiff] > maxDistance) return -1;
		}
		return (currentCost <= maxDistance) ? currentCost : -1;
	}
};


/// <summary>
	/// Class providing optimized methods for computing Levenshtein comparisons between two strings.
	/// </summary>
	/// <remarks>
	/// Copyright ©2015-2018 SoftWx, Inc.
	/// The inspiration for creating highly optimized edit distance functions was 
	/// from Sten Hjelmqvist's "Fast, memory efficient" algorithm, described at
	/// http://www.codeproject.com/Articles/13525/Fast-memory-efficient-Levenshtein-algorithm
	/// The Levenshtein algorithm computes the edit distance metric between two strings, i.e.
	/// the number of insertion, deletion, and substitution edits required to transform one
	/// string to the other. This value will be >= 0, where 0 indicates identical strings.
	/// Comparisons are case sensitive, so for example, "Fred" and "fred" will have a 
	/// distance of 1. The optimized algorithm was described in detail in my post at
	/// http://blog.softwx.net/2014/12/optimizing-levenshtein-algorithm-in-c.html
	/// Also see http://en.wikipedia.org/wiki/Levenshtein_distance for general information.
	/// The methods in this class are not threadsafe. Use the static versions in the Distance
	/// class if that is required.</remarks>
class Levenshtein : public IDistance, ISimilarity {
private:
	vector<int> baseChar1Costs;

public:

	/// <summary>Create a new instance of DamerauOSA.</summary>
	Levenshtein() {

	}

	/// <summary>Create a new instance of Levenshtein using the specified expected
	/// maximum string length that will be encountered.</summary>
	/// <remarks>By specifying the max expected string length, better memory efficiency
	/// can be achieved.</remarks>
	/// <param name="expectedMaxstringLength">The expected maximum length of strings that will
	/// be passed to the Levenshtein methods.</param>
	Levenshtein(int expectedMaxstringLength) {
		if (expectedMaxstringLength <= 0) throw "expectedMaxstringLength must be larger than 0";
		this->baseChar1Costs = vector<int>(expectedMaxstringLength, 0);
	}

	/// <summary>Compute and return the Levenshtein edit distance between two strings.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match
	/// This method is not threadsafe.</remarks>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>0 if the strings are equivalent, otherwise a positive number whose
	/// magnitude increases as difference between the strings increases.</returns>
	double Distance(xstring string1, xstring string2) {
		if (string1.empty()) return string2.size();
		if (string2.empty()) return string1.size();

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return len2;

		return Distance(string1, string2, len1, len2, start,
			(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)));
	}

	/// <summary>Compute and return the Levenshtein edit distance between two strings.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match
	/// This method is not threadsafe.</remarks>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="maxDistance">The maximum distance that is of interest.</param>
	/// <returns>-1 if the distance is greater than the maxDistance, 0 if the strings
	/// are equivalent, otherwise a positive number whose magnitude increases as
	/// difference between the strings increases.</returns>
	double Distance(xstring string1, xstring string2, double maxDistance) {
		if (string1.empty() || string2.empty()) return Helpers::NullDistanceResults(string1, string2, maxDistance);
		if (maxDistance <= 0) return (string1 == string2) ? 0 : -1;
		maxDistance = ceil(maxDistance);
		int iMaxDistance = (maxDistance <= INT_MAX) ? (int)maxDistance : INT_MAX;

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }
		if (string2.size() - string1.size() > iMaxDistance) return -1;

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return (len2 <= iMaxDistance) ? len2 : -1;

		if (iMaxDistance < len2) {
			return Distance(string1, string2, len1, len2, start, iMaxDistance,
				(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)));
		}
		return Distance(string1, string2, len1, len2, start,
			(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)));
	}

	/// <summary>Return Levenshtein similarity between two strings
	/// (1 - (levenshtein distance / len of longer string)).</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <returns>The degree of similarity 0 to 1.0, where 0 represents a lack of any
	/// noteable similarity, and 1 represents equivalent strings.</returns>
	double Similarity(xstring string1, xstring string2) {
		if (string1.empty()) return (string2.empty()) ? 1 : 0;
		if (string2.empty()) return 0;

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return 1.0;

		return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start,
			(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)))
			, string2.size());
	}


	/// <summary>Return Levenshtein similarity between two strings 
	/// (1 - (levenshtein distance / len of longer string)).</summary>
	/// <param name="string1">One of the strings to compare.</param>
	/// <param name="string2">The other string to compare.</param>
	/// <param name="minSimilarity">The minimum similarity that is of interest.</param>
	/// <returns>The degree of similarity 0 to 1.0, where -1 represents a similarity
	/// lower than minSimilarity, otherwise, a number between 0 and 1.0 where 0
	/// represents a lack of any noteable similarity, and 1 represents equivalent
	/// strings.</returns>
	double Similarity(xstring string1, xstring string2, double minSimilarity) {
		if (minSimilarity < 0 || minSimilarity > 1) throw "minSimilarity must be in range 0 to 1.0";
		if (string1.empty() || string2.empty()) return Helpers::NullSimilarityResults(string1, string2, minSimilarity);

		// if strings of different lengths, ensure shorter string is in string1. This can result in a little
		// faster speed by spending more time spinning just the inner loop during the main processing.
		if (string1.size() > string2.size()) { xstring t = string1; string1 = string2; string2 = t; }

		int iMaxDistance = Helpers::ToDistance(minSimilarity, string2.size());
		if (string2.size() - string1.size() > iMaxDistance) return -1;
		if (iMaxDistance == 0) return (string1 == string2) ? 1 : -1;

		// identify common suffix and/or prefix that can be ignored
		int len1, len2, start;
		Helpers::PrefixSuffixPrep(string1, string2, len1, len2, start);
		if (len1 == 0) return 1.0;

		if (iMaxDistance < len2) {
			return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start, iMaxDistance,
				(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)))
				, string2.size());
		}
		return Helpers::ToSimilarity(Distance(string1, string2, len1, len2, start,
			(this->baseChar1Costs = (len2 <= this->baseChar1Costs.size()) ? this->baseChar1Costs : vector<int>(len2, 0)))
			, string2.size());
	}

	/// <summary>Internal implementation of the core Levenshtein algorithm.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match</remarks>
	static int Distance(xstring string1, xstring string2, int len1, int len2, int start, vector<int> &char1Costs) {
		for (int j = 0; j < len2;) char1Costs[j] = ++j;
		int currentCharCost = 0;
		if (start == 0) {
			for (int i = 0; i < len1; ++i) {
				int leftCharCost, aboveCharCost;
				leftCharCost = aboveCharCost = i;
				xchar char1 = string1[i];
				for (int j = 0; j < len2; ++j) {
					currentCharCost = leftCharCost; // cost on diagonal (substitution)
					leftCharCost = char1Costs[j];
					if (string2[j] != char1) {
						// substitution if neither of two conditions below
						if (aboveCharCost < currentCharCost) currentCharCost = aboveCharCost; // deletion
						if (leftCharCost < currentCharCost) currentCharCost = leftCharCost; // insertion
						++currentCharCost;
					}
					char1Costs[j] = aboveCharCost = currentCharCost;
				}
			}
		}
		else {
			for (int i = 0; i < len1; ++i) {
				int leftCharCost, aboveCharCost;
				leftCharCost = aboveCharCost = i;
				xchar char1 = string1[start + i];
				for (int j = 0; j < len2; ++j) {
					currentCharCost = leftCharCost; // cost on diagonal (substitution)
					leftCharCost = char1Costs[j];
					if (string2[start + j] != char1) {
						// substitution if neither of two conditions below
						if (aboveCharCost < currentCharCost) currentCharCost = aboveCharCost; // deletion
						if (leftCharCost < currentCharCost) currentCharCost = leftCharCost; // insertion
						++currentCharCost;
					}
					char1Costs[j] = aboveCharCost = currentCharCost;
				}
			}
		}
		return currentCharCost;
	}

	/// <summary>Internal implementation of the core Levenshtein algorithm that accepts a maxDistance.</summary>
	/// <remarks>https://github.com/softwx/SoftWx.Match</remarks>
	static int Distance(xstring string1, xstring string2, int len1, int len2, int start, int maxDistance, vector<int> &char1Costs) {
		//            if (maxDistance >= len2) return Distance(string1, string2, len1, len2, start, char1Costs);
		int i, j;
		for (j = 0; j < maxDistance;) char1Costs[j] = ++j;
		for (; j < len2;) char1Costs[j++] = maxDistance + 1;
		int lenDiff = len2 - len1;
		int jStartOffset = maxDistance - lenDiff;
		int jStart = 0;
		int jEnd = maxDistance;
		int currentCost = 0;
		if (start == 0) {
			for (i = 0; i < len1; ++i) {
				xchar char1 = string1[i];
				int prevChar1Cost, aboveCharCost;
				prevChar1Cost = aboveCharCost = i;
				// no need to look beyond window of lower right diagonal - maxDistance cells (lower right diag is i - lenDiff)
				// and the upper left diagonal + maxDistance cells (upper left is i)
				jStart += (i > jStartOffset) ? 1 : 0;
				jEnd += (jEnd < len2) ? 1 : 0;
				for (j = jStart; j < jEnd; ++j) {
					currentCost = prevChar1Cost; // cost on diagonal (substitution)
					prevChar1Cost = char1Costs[j];
					if (string2[j] != char1) {
						// substitution if neither of two conditions below
						if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
						if (prevChar1Cost < currentCost) currentCost = prevChar1Cost;   // insertion
						++currentCost;
					}
					char1Costs[j] = aboveCharCost = currentCost;
				}
				if (char1Costs[i + lenDiff] > maxDistance) return -1;
			}
		}
		else {
			for (i = 0; i < len1; ++i) {
				xchar char1 = string1[start + i];
				int prevChar1Cost, aboveCharCost;
				prevChar1Cost = aboveCharCost = i;
				// no need to look beyond window of lower right diagonal - maxDistance cells (lower right diag is i - lenDiff)
				// and the upper left diagonal + maxDistance cells (upper left is i)
				jStart += (i > jStartOffset) ? 1 : 0;
				jEnd += (jEnd < len2) ? 1 : 0;
				for (j = jStart; j < jEnd; ++j) {
					currentCost = prevChar1Cost; // cost on diagonal (substitution)
					prevChar1Cost = char1Costs[j];
					if (string2[start + j] != char1) {
						// substitution if neither of two conditions below
						if (aboveCharCost < currentCost) currentCost = aboveCharCost; // deletion
						if (prevChar1Cost < currentCost) currentCost = prevChar1Cost;   // insertion
						++currentCost;
					}
					char1Costs[j] = aboveCharCost = currentCost;
				}
				if (char1Costs[i + lenDiff] > maxDistance) return -1;
			}
		}
		return (currentCost <= maxDistance) ? currentCost : -1;
	}
};


/// <summary>Supported edit distance algorithms.</summary>
enum DistanceAlgorithm {
	/// <summary>Levenshtein algorithm.</summary>
	LevenshteinDistance,
	/// <summary>Damerau optimal string alignment algorithm.</summary>
	DamerauOSADistance
};

/// <summary>Wrapper for third party edit distance algorithms.</summary>
class EditDistance {
private:
	DistanceAlgorithm algorithm;
	IDistance* distanceComparer;
	DamerauOSA damerauOSADistance;
	Levenshtein levenshteinDistance;

public:
	/// <summary>Create a new EditDistance object.</summary>
	/// <param name="algorithm">The desired edit distance algorithm.</param>
	EditDistance(DistanceAlgorithm algorithm) {
		this->algorithm = algorithm;
		switch (algorithm) {
		case DistanceAlgorithm::DamerauOSADistance:
			this->distanceComparer = &damerauOSADistance;
			break;
		case DistanceAlgorithm::LevenshteinDistance:
			this->distanceComparer = &levenshteinDistance;
			break;
		default:
			throw "Unknown distance algorithm.";
		}
	}

	/// <summary>Compare a string to the base string to determine the edit distance,
	/// using the previously selected algorithm.</summary>
	/// <param name="string2">The string to compare.</param>
	/// <param name="maxDistance">The maximum distance allowed.</param>
	/// <returns>The edit distance (or -1 if maxDistance exceeded).</returns>
	int Compare(xstring string1, xstring string2, int maxDistance) {
		return (int)this->distanceComparer->Distance(string1, string2, maxDistance);
	}
};

class Node
{
public:
	xstring suggestion;
	int next;
};

class Entry
{
public:
	int count;
	int first;
};

/// <summary>An intentionally opacque class used to temporarily stage
/// dictionary data during the adding of many words. By staging the
/// data during the building of the dictionary data, significant savings
/// of time can be achieved, as well as a reduction in final memory usage.</summary>
class SuggestionStage
{
private:
	Dictionary<int, Entry> Deletes;
	ChunkArray<Node> Nodes;

public:
	/// <summary>Create a new instance of SuggestionStage.</summary>
	/// <remarks>Specifying ann accurate initialCapacity is not essential, 
	/// but it can help speed up processing by alleviating the need for 
	/// data restructuring as the size grows.</remarks>
	/// <param name="initialCapacity">The expected number of words that will be added.</param>
	SuggestionStage(int initialCapacity)
	{
		Deletes.reserve(initialCapacity);
		Nodes.Reserve(initialCapacity * 2);
	}

	/// <summary>Gets the count of unique delete words.</summary>
	int DeleteCount() { return Deletes.size(); }

	/// <summary>Gets the total count of all suggestions for all deletes.</summary>
	int NodeCount() { return Nodes.Count; }

	/// <summary>Clears all the data from the SuggestionStaging.</summary>
	void Clear()
	{
		Deletes.clear();
		Nodes.Clear();
	}

	void Add(int deleteHash, xstring suggestion)
	{
		auto deletesFinded = Deletes.find(deleteHash);
		Entry newEntry;
		newEntry.count = 0;
		newEntry.first = -1;
		Entry entry = (deletesFinded == Deletes.end()) ? newEntry : deletesFinded->second;
		int next = entry.first;
		entry.count++;
		entry.first = Nodes.Count;
		Deletes[deleteHash] = entry;
		Node item;
		item.suggestion = suggestion;
		item.next = next; // 1st semantic errors, this should not be Nodes.Count
		Nodes.Add(item);
	}

	void CommitTo(Dictionary<int, vector<xstring>>* permanentDeletes)
	{
		auto permanentDeletesEnd = permanentDeletes->end();
		for (auto it = Deletes.begin(); it != Deletes.end(); ++it)
		{
			auto permanentDeletesFinded = permanentDeletes->find(it->first);
			vector<xstring> suggestions;
			int i;
			if (permanentDeletesFinded != permanentDeletesEnd)
			{
				suggestions = permanentDeletesFinded->second;
				i = suggestions.size();

				vector<xstring> newSuggestions;
				newSuggestions.reserve(suggestions.size() + it->second.count);
				std::copy(suggestions.begin(), suggestions.end(), back_inserter(newSuggestions));
				//permanentDeletes[it->first] = newSuggestions;
			}
			else
			{
				i = 0;
				int32_t count = it->second.count;
				suggestions.reserve(count);
				//permanentDeletes[it->first] = suggestions;
			}

			int next = it->second.first;
			while (next >= 0)
			{
				auto node = Nodes.At(next);
				suggestions.push_back(node.suggestion);
				next = node.next;
				++i;
			}
			(*permanentDeletes)[it->first] = suggestions;
		}
	}
};


/// <summary>Spelling suggestion returned from Lookup.</summary>
class SuggestItem
{
public:
	/// <summary>The suggested correctly spelled word.</summary>
	xstring term = XL("");
	/// <summary>Edit distance between searched for word and suggestion.</summary>
	int distance = 0;
	/// <summary>Frequency of suggestion in the dictionary (a measure of how common the word is).</summary>
	int64_t count = 0;

	/// <summary>Create a new instance of SuggestItem.</summary>
	/// <param name="term">The suggested word.</param>
	/// <param name="distance">Edit distance from search word.</param>
	/// <param name="count">Frequency of suggestion in dictionary.</param>
	SuggestItem()
	{
	}


	SuggestItem(xstring term, int distance, int64_t count)
	{
		this->term = term;
		this->distance = distance;
		this->count = count;
	}

	int CompareTo(SuggestItem other)
	{
		// order by distance ascending, then by frequency count descending, and then by alphabet descending
		int disCom = Helpers::CompareTo(this->distance, other.distance);
		if (disCom != 0)
			return disCom;
		int cntCom = Helpers::CompareTo(other.count, this->count);
		if (cntCom != 0)
			return cntCom;
		return this->term.compare(other.term);
	}

	bool Equals(const SuggestItem obj)
	{
		return this->term == obj.term && this->distance == obj.distance && this->count == obj.count;
	}

	int GetHashCode()
	{
		return hash<xstring>{}(this->term);
	}

	xstring Tostring()
	{
		return XL("{") + term + XL(", ") + to_xstring(distance) + XL(", ") + to_xstring(count) + XL("}");
	}
	static bool compare(SuggestItem s1, SuggestItem s2)
	{
		return s1.CompareTo(s2) < 0 ? 1 : 0;
	}

	void set(SuggestItem exam)
	{
		this->term = exam.term;
		this->distance = exam.distance;
		this->count = exam.count;
	}
};
