#include "main.h"
#include "TestSortAlgo.hpp"

#include <windows.h>
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    TestSortAlgo::RunTest();
    std::cin >> std::ws;
    return 0;
}
