#include "processors/BedDeduplicator.hpp"
#include "fileformats/Bed.hpp"
#include "io/InputStream.hpp"
#include "fileformats/DefaultPrinter.hpp"

#include <gtest/gtest.h>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

TEST(BedDeduplicator, deduplicate) {
    string inputText[] = {
        "1\t2\t2\t*/ATTAT",
        "1\t2\t2\t*/ATTCT",
        "1\t2\t2\t*/ATTAT", //dup
        "1\t2\t2\t*/ATTCTT",
        "2\t3\t4\tC/G",
        "2\t3\t4\tC/G", //dup
        "2\t3\t4\tC/T",
        "3\t3\t6\tCGC/*",
        "3\t3\t6\tCGC/*", // dup
        "3\t3\t7\tCGCT/*"
    };
    size_t nBeds = sizeof(inputText)/sizeof(inputText[0]);

    string expected =
        "1\t2\t2\t*/ATTAT\n"
        "1\t2\t2\t*/ATTCT\n"
        "1\t2\t2\t*/ATTCTT\n"
        "2\t3\t4\tC/G\n"
        "2\t3\t4\tC/T\n"
        "3\t3\t6\tCGC/*\n"
        "3\t3\t7\tCGCT/*\n"
        ;

    stringstream result;
    DefaultPrinter out(result);
    BedDeduplicator<DefaultPrinter> bdd(out);
    BedHeader hdr;
    for (size_t i = 0; i < nBeds; ++i) {
        Bed b;
        Bed::parseLine(&hdr, inputText[i], b, 1); // 1 extra field: allele
        bdd(b);
    }
    bdd.flush();

    ASSERT_EQ(expected, result.str());
}
