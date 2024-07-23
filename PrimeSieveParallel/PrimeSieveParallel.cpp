#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>

#include <agents.h>

using namespace concurrency;
using namespace std;

#include "optionparser.h"

typedef unsigned long long int itype;

struct Arg : public option::Arg
{
	static option::ArgStatus Required(const option::Option& option, bool)
	{
		return option.arg == 0 ? option::ARG_ILLEGAL : option::ARG_OK;
	}
};

enum  optionIndex { UNKNOWN, SIEVES, MEMORY, PRIME, HELP, LOGFILE };

const option::Descriptor usage[] = {
{ UNKNOWN,  0, "", "",			Arg::None, "USAGE: primesieveparallel [options]\n\n"
										  "Options:" },
{ HELP,     0, "h", "help",		Arg::None,    "  \t--help  \tPrint usage and exit." },
{ SIEVES,   0, "s","sieves",	Arg::Required,"  -s <arg>, \t--sieves=<arg>"
										  "  \tMaximum number of parallel sieves." },
{ MEMORY,   0, "m","memory",	Arg::Required,"  -m<arg>, \t--memory=<arg>  \tMaximum amount of memory to use" },
{ PRIME,    0, "p","prime",		Arg::Required,"  -p <arg>, \t--prime=<arg>"
										  "  \tFind primes up to this number" },
{ LOGFILE,  0, "l","logfile",	Arg::Required,"  -l <arg>, \t--logfile=<arg>"
										  "  \tWrite summary of run to this file" },
{ UNKNOWN,  0, "", "",			Arg::None,
 "\nExamples:\n"
 "  primesieveparallel -s4 -m1e9 -p 1000000\n"
},
{ 0, 0, 0, 0, 0, 0 } }; // End of table

class sieve : public agent
{
public:
	sieve(itype startRange, itype endRange, const itype* initPrimes, itype numInitPrimes, ITarget<sieve*>& endChannel)
		: 
		startRange(startRange), endRange(endRange), 
		initPrimes(initPrimes), numInitPrimes(numInitPrimes), 
		resultChannel(endChannel)
	{
	};

	~sieve()
	{
		delete[] numbers;
	}

	itype primeCount = 0;

private:
	itype startRange, endRange;
	const itype* initPrimes;
	itype numInitPrimes;

	char* numbers = nullptr;

	concurrency::ITarget<sieve*>& resultChannel;

	void run()
	{
		itype rangeSize = endRange - startRange + 1; // Include both ends
		numbers = new char[rangeSize] { 0 };

		itype sqrtEnd = sqrt((double)endRange);
		itype p = 2;
		itype pindex = 0;

		while (p <= sqrtEnd)
		{
			itype i;

			if (p * p > startRange)
				i = p * p - startRange;
			else
			{
				itype offset = (startRange % p);
				i = (offset == 0 && p != startRange) ? 0 : (p - offset);
			}

			for (; i < rangeSize; i += p)
			{
				numbers[i] = 1;
			}

			pindex++;
			p = (pindex < numInitPrimes) ? initPrimes[pindex] : sqrtEnd + 1;
		}

		// Count my primes
		for (itype i = 0; i < rangeSize; i++)
		{
			if (numbers[i] == 0)
			{
				primeCount++;
			}
		}

		done();

		asend(resultChannel, this);
	};
};

class starter : public concurrency::agent
{
public:
	starter(
		itype maxPrime,
		int parallelSieves,
		itype maxMemory
	)
		:
		maxPrime(maxPrime),
		parallelSieves(parallelSieves),
		maxMemory(maxMemory)
	{
	};

	itype numPrimes = 0;
	int totalSieves;

private:
	itype maxPrime;
	int parallelSieves;
	itype maxMemory;

	unbounded_buffer<sieve*> resultChannel;

	itype nextSieveStart;
	itype rangeSize;

	itype initialNumPrimes = 0;
	itype* initialPrimes = nullptr;

	bool CreateSieve()
	{
		if (nextSieveStart > maxPrime)
			return false;

		itype end = nextSieveStart + rangeSize - 1;

		// Make sure the last thread is not too long
		// or takes the last chunk
		if (end > maxPrime)
			end = maxPrime;

		sieve* newSieve =
			new sieve(
				nextSieveStart,
				end,
				initialPrimes,
				initialNumPrimes,
				resultChannel);

		//cout << "Making sieve with start " << nextSieveStart
		//     << " and end " << end << endl;

		nextSieveStart = end+1;

		newSieve->start();

		return true;
	}

	void run()
	{
		itype maxRangeSize;

		// Calculate how many primes the starter must calculate on its own
		itype sqrtMaxNum = sqrt(double(maxPrime));
		itype sqrtSqrtMaxNum = sqrt((double)sqrtMaxNum);

		// Prepare the primary sieve in the starter
		char* numbers = new char[sqrtMaxNum + 1] {0};  // Starts at 0, must include sqrtMaxNum
		numbers[0] = numbers[1] = 1;

		cout << "Initial sieve size: " << sqrtMaxNum << endl;
		cout << "Initial sieve seeding prime size: " << sqrtSqrtMaxNum << endl;

		// Calculate the first sqrtMaxNum primes
		itype p = 2;
		do
		{
			for (itype i = p * p; i <= sqrtMaxNum; i += p)
			{
				numbers[i] = 1;
			}

			// Find next prime
			p++;
			while (p <= sqrtSqrtMaxNum && (numbers[p] != 0))
				p++;

		} while (p <= sqrtSqrtMaxNum);

		// Count my own primes
		for (int i = 2; i <= sqrtMaxNum; i++)
		{
			if (numbers[i] == 0)
			{
				numPrimes++;
			}
		}

		cout << "Initial sieve number of primes: " << numPrimes << endl;

		// Convert initial sieve to primes
		initialNumPrimes = numPrimes;
		initialPrimes = new itype[numPrimes];
		itype index = 0;
		for (int i = 2; i <= sqrtMaxNum; i++)
		{
			if (numbers[i] == 0)
			{
				initialPrimes[index++] = i;
			}
		}

		// Release the initial sieve
		delete[] numbers;
		numbers = nullptr;

		// Calculate rangesize on the basis of used and max memory
		unsigned long long int startUse = sizeof(starter) + sizeof(itype) * numPrimes;
		maxRangeSize = (maxMemory - startUse + parallelSieves - 1) / parallelSieves;

		// Adjust rangesize if less than maxMemory needed
		rangeSize = std::min<itype>(maxRangeSize, (maxPrime - sqrtMaxNum + parallelSieves - 1) / parallelSieves);

		cout << endl;
		cout << "Range for each sieve: " << rangeSize << endl;
		totalSieves = (maxPrime - sqrtMaxNum + rangeSize - 1) / rangeSize;
		cout << "Using " << totalSieves << " sieves" << endl;

		cout << endl;

		// Start the sieves
		nextSieveStart = sqrtMaxNum+1;

		// Create numsieves new sieves
		int outstandingSieves = 0;
		for (int i = 0; i < parallelSieves; i++)
		{
			if (CreateSieve())
				outstandingSieves++;
		}

		int finishedSieves = 0;
		do
		{
			// Wait for a finished sieve
			sieve* finished = receive(resultChannel);

			finishedSieves++;
			// Harvest results from the finished sieve
			itype sievePrimes = finished->primeCount;
			numPrimes += sievePrimes;

			// cout << "Sieve " << finishedSieves << " has " << sievePrimes << " primes" << endl;

			// delete it
			delete finished;
			finished = nullptr;

			// Replace it with a new one
			if (CreateSieve())
			{

			}
			else
			{
				outstandingSieves--;
			}


		} while (outstandingSieves > 0);

		cout << endl << numPrimes << " primtal" << endl;
		// Output summary

		done();
	};
};



int main(int argc, char* argv[])
{
	int parallelSieves = 8;
	unsigned long long int maxMemory = 0x80000000ULL - 1;
	itype maxPrime = 1000000;

	// Imbue with thousand separators because we have big numbers
	// Because of trouble with locales on MINGW64, the facet is implemented with raw force
	// Example here: https://en.cppreference.com/w/cpp/locale/numpunct/grouping

	struct specialNumpunct : public std::numpunct<char>
	{
		char do_thousands_sep() const override { return ','; }
		string do_grouping() const override { return "\3"; }
	};

	std::cout.imbue(std::locale(std::cout.getloc(), new specialNumpunct));

	argc -= 1;
	argv += 1;
	option::Stats  stats(usage, argc, argv);
	std::vector<option::Option> options(stats.options_max);
	std::vector<option::Option> buffer(stats.buffer_max);
	option::Parser parse(true, usage, argc, argv, &options[0], &buffer[0]);

	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
		std::cout << "Unknown option: " << std::string(opt->name, opt->namelen) << "\n";

	for (int i = 0; i < parse.nonOptionsCount(); ++i)
		std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << std::endl;

	if (parse.error() || options[HELP] || options[UNKNOWN] || parse.nonOptionsCount()) {
		option::printUsage(std::cout, usage, 40);
		return 1;
	}

	if (options[SIEVES])
	{
		int sieves = std::atol(options[SIEVES].arg);

		if (sieves > 0)
			parallelSieves = sieves;
	}
	cout << "Specified parallel sieves: " << parallelSieves << endl;

	if (options[MEMORY])
	{
		unsigned long long int memory;

		const char* pMem = options[MEMORY].arg;
		
		// Allow parameter to be in float format, eg. 1E8
		double dMem;
		std::from_chars(pMem, pMem + strlen(pMem), dMem);
		memory = (long long int) dMem;

		maxMemory = memory;
	}
	cout << "Specified max memory: " << maxMemory << endl;

	if (options[PRIME])
	{
		// Allow parameter to be in float format, eg. 1E8
		double dPrime;
		const char* parg = options[PRIME].arg;
		std::from_chars(parg, parg + strlen(parg), dPrime);

		itype prime = (itype) dPrime;

		if (prime > 0)
			maxPrime = prime;
	}
	cout << "Searching for primes up to " << maxPrime << endl;

	auto startT = std::chrono::high_resolution_clock::now();

	starter first(maxPrime, parallelSieves, maxMemory);

	first.start();

	agent::wait(&first);

	auto elapsed = std::chrono::high_resolution_clock::now() - startT;
	auto elapsedSeconds = (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0);

	cout << "Total time: " <<  elapsedSeconds << " seconds" << endl;

	// Log results
	if (options[LOGFILE])
	{
		ofstream log(options[LOGFILE].arg, std::ios::app);

		log << maxPrime << ";";
		log << parallelSieves << ";";
		log << first.totalSieves << ";";
		log << maxMemory << ";";
		log << first.numPrimes << ";";
		log << elapsedSeconds << ";";
		log << endl;
	}
}

