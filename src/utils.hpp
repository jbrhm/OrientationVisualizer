#pragma once
#include <iostream>

// Gets the sign of a number
inline int getSign(double num){
	if (num < 0) return -1;
	return 1;
}
// Gets the type of on object and prints it
template <class T> void printType(const T&)
{
    std::cout << __PRETTY_FUNCTION__ << "\n";
}