// RunPrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

int main()
{
    constexpr unsigned long long int K1 = 1000;
    constexpr unsigned long long int M1 = K1 * 1000;
    constexpr unsigned long long int G1 = M1 * 1000;
    constexpr unsigned long long int T1 = G1 * 1000;

    constexpr unsigned long long int Ki1 = 1024;
    constexpr unsigned long long int Mi1 = 1024 * Ki1;
    constexpr unsigned long long int Gi1 = 1024 * Mi1;

    std::vector<unsigned long long int> maxPrimeValues{ 20*T1 };
    
    std::vector<unsigned long long> memValues {
        20*G1,
    };

    std::vector<int> sieveValues{
        32
    };

    std::vector<unsigned long long> rangeValues{
        512* Ki1, (512 + 256)* Ki1
    };

    std::ostringstream commandLine;

    for (auto p : maxPrimeValues)
    {
        for (auto s : sieveValues)
        {
            for (auto m : memValues)
            {
                for (auto r : rangeValues)
                {
                    std::ostringstream commandLine;

                    commandLine << "..\\..\\PrimeSieveParallel\\x64\\Release\\PrimeSieveParallel.exe ";
                    commandLine << " -p " << static_cast<long double>(p);
                    commandLine << " -s " << s;
                    commandLine << " -m " << m;
                    commandLine << " -r " << r;

                    commandLine << " -l " << "p10M-10000Gs32mMaxrMedium.csv";
                    commandLine << std::endl;

                    std::cout << commandLine.str();

                    system(commandLine.str().c_str());
                }
            }
        }
    }
}
