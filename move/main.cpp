
#include <iostream>
#include <utility>
#include <memory>
#include <vector>
#include <sstream>

#include <string.h>

using namespace std;

char* gbuf;

/*

// this one sucks
vector<int>&& make_vector() {
    size_t size = 10;
    vector<int>* v = new vector<int>();
    for (int i = 0; i < size; ++i) {
        v->push_back(i);
    }
    return move(*v);
}

unique_ptr< vector<int> > make_vector() {
    size_t size = 10;
    vector<int>* v = new vector<int>();
    for (int i = 0; i < size; ++i) {
        v->push_back(i);
    }
    return unique_ptr< vector<int> >(v);
}

int main() {
    auto v = make_vector();
    for (vector<int>::iterator it = v->begin(); it != v->end(); ++it) {
        cout << *it << endl;
    }
    return 0;
}

*/

vector<char> make_vector() {
    size_t size = 256;
    vector<char> v(size);
    strcpy(v.data(), "This is the contents of the vector buffer");
    return v;
}

unique_ptr<istringstream>
make_stream() {
    char* raw = new char[256];
    strcpy(raw, "This is the contents of the buffer");
    istringstream* is = new istringstream();
    is->rdbuf()->pubsetbuf(raw, 256);
    return unique_ptr<istringstream>(is);
}

istringstream
make_stream_move() {
    gbuf = new char[256];
    strcpy(gbuf, "This is the contents of the global buffer");
    istringstream is;
    is.rdbuf()->pubsetbuf(gbuf, 256);
    return is;
}

int main() {
    string tok;
    auto is = make_stream_move();
    gbuf[0] = 't';  // verify it was a move
    while (is.good()) {
        is >> tok;
        cout << tok << endl;
    }
    auto v = make_vector();
    istringstream iv;
    iv.rdbuf()->pubsetbuf(v.data(), v.capacity());
    while (iv.good()) {
        iv >> tok;
        cout << tok << endl;
    }
}
