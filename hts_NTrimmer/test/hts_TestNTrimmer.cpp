#include "gtest/gtest.h"
#include <sstream>
#include <iostream>
#include "hts_NTrimmer.h"

class TrimN : public ::testing::Test {
public:
    po::variables_map vm;
    const std::string readData_1 = "@Read1\nTTTTTNGAAAAAAAAAGNTTTTT\n+\n#######################\n";
    const std::string readData_2 = "@Read1\nAAAAAAAAAAAAAAAAAAAAAAAA\n+\n########################\n";
    const std::string readData_3 = "@Read1\nTTTTTNGGNTTTTTTTTTTTTTN\n+\n#######################\n";
    const std::string readData_4 = "@Read1\nNTTTTAGGATTTTTTTTTTTTTN\n+\n#######################\n";
    const std::string readData_5 = "@Read1\nATTTTAGGATTTTTTTTTTTTTN\n+\n#######################\n";
    const std::string readData_6 = "@Read1\nNTTTTAGGATTTTTTTTTTTTTA\n+\n#######################\n";
    const std::string readData_7 = "@Read1\nGTTTTAGGATTNTTTTTTTTTTA\n+\n#######################\n";
    const std::string readData_8 = "@Read1\nNNNNNNNNNNNNNNNNNNNNNNN\n+\n#######################\n";
    const std::string readData_9a = "@Read1\nCTGACTGACTGANNNACTGACTGACTGNCTGACTG\n+\n###################################\n";
    const std::string readData_9b = "@Read1\nCTGACTGACTGANNNACTGACTGACTGANTGACTG\n+\n###################################\n";
    const std::string readData_10a = "@Read1\nN\n+\n#\n";
    const std::string readData_10b = "@Read1\nACTNNNTGCT\n+\n##########\n";
    const std::string readData_empty_tab = "D00689:146:C9B2EANXX:8:2303:19367:3733#GATCAC		";
    NTrimmer nt;
};

TEST_F(TrimN, Exclude) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_5);

    NTrimCounters counter("hts_NTrimmer", vm);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);
    bool test;
    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        test = nt.trim_n(per->non_const_read_one(), true);
        if (!test) counter.increment_discard_pe();
        ASSERT_EQ(false, test);
        ASSERT_EQ(1u, counter.PE_Discarded);
    }
};

TEST_F(TrimN, EdgeRightN) {
    std::istringstream in1(readData_5);
    std::istringstream in2(readData_5);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("ATTTTAGGATTTTTTTTTTTTT", (per->non_const_read_one()).get_sub_seq());
    }
};


TEST_F(TrimN, EdgeLeftN) {
    std::istringstream in1(readData_6);
    std::istringstream in2(readData_6);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("TTTTAGGATTTTTTTTTTTTTA", (per->non_const_read_one()).get_sub_seq());
    }
};


TEST_F(TrimN, EdgeCaseNonEnds) {
    std::istringstream in1(readData_4);
    std::istringstream in2(readData_4);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("TTTTAGGATTTTTTTTTTTTT", (per->non_const_read_one()).get_sub_seq());
    }
};

TEST_F(TrimN, BasicTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_2);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("GAAAAAAAAAG", (per->non_const_read_one()).get_sub_seq());
    }
};

TEST_F(TrimN, NoTrim) {
    std::istringstream in1(readData_1);
    std::istringstream in2(readData_2);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_two(), false);
        ASSERT_EQ("AAAAAAAAAAAAAAAAAAAAAAAA", (per->non_const_read_two()).get_sub_seq());
    }
};

TEST_F(TrimN, TwoBasicTrim) {
    std::istringstream in1(readData_3);
    std::istringstream in2(readData_2);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("TTTTTTTTTTTTT", (per->non_const_read_one()).get_sub_seq());
    }
};

TEST_F(TrimN, equalTrim) {
    std::istringstream in1(readData_7);

    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifp(in1);

    while(ifp.has_next()) {
        auto i = ifp.next();
        SingleEndRead *per = dynamic_cast<SingleEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("GTTTTAGGATT", (per->non_const_read_one()).get_sub_seq());
    }
};

TEST_F(TrimN, allN) {
    std::istringstream in1(readData_8);

    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifp(in1);

    while(ifp.has_next()) {
        auto i = ifp.next();
        SingleEndRead *per = dynamic_cast<SingleEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ("N", (per->non_const_read_one()).get_sub_seq());
    }
};

TEST_F(TrimN, longN) {
    std::istringstream in1(readData_9a);
    std::istringstream in2(readData_9b);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        nt.trim_n(per->non_const_read_two(), false);
        ASSERT_EQ("CTGACTGACTGA", (per->non_const_read_one()).get_sub_seq());
        ASSERT_EQ("ACTGACTGACTGA", (per->non_const_read_two()).get_sub_seq());
    }
};

TEST_F(TrimN, shortN) {
    std::istringstream in1(readData_10a);
    std::istringstream in2(readData_10b);

    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);

    while(ifp.has_next()) {
        auto i = ifp.next();
        PairedEndRead *per = dynamic_cast<PairedEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        nt.trim_n(per->non_const_read_two(), false);
        ASSERT_EQ("N", (per->non_const_read_one()).get_sub_seq());
        ASSERT_EQ(0u, (per->get_read_one().getRTrim()));
        ASSERT_EQ(1u, (per->get_read_one().getLengthTrue()));
        ASSERT_EQ(per->get_read_one().getLength(),per->get_read_one().getLengthTrue());

        ASSERT_EQ("TGCT", (per->non_const_read_two()).get_sub_seq());
        ASSERT_EQ(0u, (per->get_read_two().getRTrim()));
        ASSERT_EQ(6u, (per->get_read_two().getLTrim()));
        ASSERT_EQ(10u, per->get_read_two().getLength());
        ASSERT_EQ(4u, per->get_read_two().getLengthTrue());
    }
};

TEST_F(TrimN, emptyTab) {
    std::istringstream in1(readData_empty_tab);

    InputReader<ReadBase, TabReadImpl> ifp(in1);

    while(ifp.has_next()) {
        auto i = ifp.next();
        SingleEndRead *per = dynamic_cast<SingleEndRead*>(i.get());
        nt.trim_n(per->non_const_read_one(), false);
        ASSERT_EQ(0u, per->get_read().getLength());
        ASSERT_EQ(1u, per->get_read().getLengthTrue());
        ASSERT_EQ(0u, per->get_read().getRTrim());
    }
};
