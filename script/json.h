#ifndef __LIB_H__
#define __LIB_H__

#include <sys/types.h>

using namespace std;

class json {
	private:

	public:
		json(char[]);
		~json();
};

#include "lib.cpp"

#endif