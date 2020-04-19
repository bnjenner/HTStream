#include "gtest/gtest.h"
#include <sstream>
#include <iostream>
#include "hts_QWindowTrim.h"

class QTrimTest : public ::testing::Test {
public:
    const std::string readData_1 = "@Read1\nTTTTTGGAAAAAAAAAGTCTTTGTTG\n+\n#####AAAAAAAAAAAAAAAA#####\n";
    const std::string readData_2 = "@Read1\nTTTTTGGAAAAAAAAAGTCTTTGTTG\n+\n##########################\n";
    const std::string readData_Perf="@Read1\nTTTTTGGAAAAAAAAAGTCTTTGTTG\n+\nAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    size_t window_size = 5;
    //size_t sum_qual = (20 + 33) * window_size;
    size_t sum_qual = (20 + 33) ;

    QWindowTrim qt;
};

TEST_F(QTrimTest, LeftTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_1);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        qt.trim_left(per->non_const_read_one(), sum_qual, window_size);
        ASSERT_EQ("AAAAAAAAAAAAAA#####", (per->non_const_read_one()).get_sub_qual());
    }
};

TEST_F(QTrimTest, RightTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_1);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        qt.trim_right(per->non_const_read_one(), sum_qual, window_size);
        ASSERT_EQ("#####AAAAAAAAAAAAAA", (per->non_const_read_one()).get_sub_qual());
    }
};

TEST_F(QTrimTest, NoTrim) {
    std::istringstream in1(readData_Perf);
    std::istringstream in2(readData_Perf);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        qt.trim_left(per->non_const_read_one(), sum_qual, window_size);
        qt.trim_right(per->non_const_read_one(), sum_qual, window_size);
        ASSERT_EQ("AAAAAAAAAAAAAAAAAAAAAAAAAA", (per->non_const_read_one()).get_sub_qual());
        ASSERT_EQ("AAAAAAAAAAAAAAAAAAAAAAAAAA", (per->non_const_read_two()).get_sub_qual());
    }
};

TEST_F(QTrimTest, BasicTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_2);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        qt.trim_left(per->non_const_read_one(), sum_qual, window_size);
        qt.trim_right(per->non_const_read_one(), sum_qual, window_size);
        ASSERT_EQ("AAAAAAAAAAAA", (per->non_const_read_one()).get_sub_qual());
    }
};

TEST_F(QTrimTest, AllTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_2);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        qt.trim_left(per->non_const_read_two(), sum_qual, window_size);
        qt.trim_right(per->non_const_read_two(), sum_qual, window_size);
        ASSERT_EQ("#", (per->non_const_read_two()).get_sub_qual());
    }
};
