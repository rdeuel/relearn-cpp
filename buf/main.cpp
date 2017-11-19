
#include <iostream>
#include <streambuf>
#include <sstream>

#include <string.h>

using namespace std;


int main() {
    char* raw = new char[256];
    strcpy(raw, "This is the contents of the buffer");
    istringstream is;
    is.rdbuf()->pubsetbuf(raw, 256);
    string tok;
    while (is.good()) {
        is >> tok;
        cout << tok << endl;
    }
    delete [] raw;
}
