// RunPrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

int main()
{
#define G1 (1000000000ULL)
#define M1 (1000000ULL)
#define K1 (1000ULL)

    const unsigned long long int maxPrime = 10 * G1;
    std::ostringstream commandLine;
    std::vector<unsigned long long> memValues {
        23*G1,
    };

    std::vector<int> sieveValues { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32 };

    for (int s = 0; s < sieveValues.size(); s++)
    {
        for (int m = 0; m < memValues.size(); m++)
        {
            std::ostringstream commandLine;

            commandLine << "..\\..\\PrimeSieveParallel\\x64\\Release\\PrimeSieveParallel.exe ";
            commandLine << " -p " << maxPrime;
            commandLine << " -s " << sieveValues[s];
            commandLine << " -m " << memValues[m];
            commandLine << " -l " << "TestOutput.csv";
            commandLine << std::endl;

            std::cout << commandLine.str();

            system(commandLine.str().c_str());
        }
    }
}
