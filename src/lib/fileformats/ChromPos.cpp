#include "ChromPos.hpp"
#include "common/Tokenizer.hpp"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <iostream>
#include <utility>

using boost::format;
using namespace std;

std::ostream& operator<<(std::ostream& s, const ChromPosHeader& h) {
    return s;
}

ChromPos::ChromPos()
    : _start(0)
{}

ChromPos::ChromPos(const ChromPos& b)
    : _chrom(b._chrom)
    , _start(b._start)
    , _line(b._line)
{
}

ChromPos::ChromPos(ChromPos&& b)
    : _chrom(std::move(b._chrom))
    , _start(b._start)
    , _line(std::move(b._line))
{
}

ChromPos& ChromPos::operator=(ChromPos const& b) {
    _chrom = b._chrom;
    _start = b._start;
    _line = b._line;
    return *this;
}

ChromPos& ChromPos::operator=(ChromPos&& b) {
    _chrom = std::move(b._chrom);
    _start = std::move(b._start);
    _line = std::move(b._line);
    return *this;
}



void ChromPos::parseLine(const ChromPosHeader*, std::string& line, ChromPos& cp) {
    Tokenizer<char> tokenizer(line);
    if (!tokenizer.extract(cp._chrom))
        throw runtime_error(str(format("Failed to extract chromosome from ChromPos line '%1%'") %line));

    if (!tokenizer.extract(cp._start))
        throw runtime_error(str(format("Failed to extract start position from ChromPos line '%1%'") %line));

    cp._line.swap(line);
}

void ChromPos::swap(ChromPos& rhs) {
    _chrom.swap(rhs._chrom);
    std::swap(_start, rhs._start);
    _line.swap(rhs._line);
}

const std::string& ChromPos::toString() const {
    return _line;
}

std::ostream& operator<<(std::ostream& s, const ChromPos& cp) {
    s << cp.toString();
    return s;
}
