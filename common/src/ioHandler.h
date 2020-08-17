#ifndef IOHANDLER_H
#define IOHANDLER_H

#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include <istream>
#include <fstream>
#include <memory>
#include <utility>

#include "read.h"
#include <boost/iostreams/concepts.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <string>
#include <cstdio>

#include "counters.h"
#include "hts_exception.h"

namespace bf = boost::filesystem;
namespace bi = boost::iostreams;

int check_open_r(const std::string& filename);
std::string string2fasta(std::string seqstring, std::string prefix, const char delim=',');
Read fasta_to_read(std::string fasta_file);

class HtsOfstream {
private:
    std::string filename;
    bool force;
    bool gzip;
    bool std_out;
    std::shared_ptr<std::ostream> out = nullptr;
    FILE* gzfile = 0;

    int check_exists(const std::string& filename, bool force, bool gzip, bool std_out) ;

    void create_out() {
        out.reset(new bi::stream<bi::file_descriptor_sink> (
                check_exists(filename, force, gzip, std_out), bi::close_handle));
    }

public:
    ~HtsOfstream() {
        if (out) {
            out->flush();
            if (gzfile) {
                pclose(gzfile);
            }
            out.reset();
        }
    }

    HtsOfstream(std::string filename_, bool force_, bool gzip_, bool stdout_) :
        filename(filename_), force(force_), gzip(gzip_), std_out(stdout_)  { }

    HtsOfstream(std::shared_ptr<std::ostream> out_) : out(out_) { }

    template<class T>
    HtsOfstream& operator<< (T s) {
        if (!out) {
            create_out();
        }
        *out << s;
        return *this;
    }
};

// ### input ###
template <class T, class Impl>
class InputReader : Impl {
public:
    typedef std::unique_ptr<T> value_type;
    using Impl::Impl;

    bool has_next();
    value_type next();
};

class InputFastq {
protected:
    ReadPtr load_read(std::istream *input);

    std::string id, seq, id2, qual;
};

class InputFasta {
protected:
    ReadPtr load_read(std::istream *input);
    std::string id, seq;
    std::string tmpSeq;
};

class FastaReadImpl : public InputFasta {
public:
    FastaReadImpl(std::istream& input_) : input(&input_) {}
protected:
    std::istream* input = 0;
};

class PairedEndReadFastqImpl : public InputFastq {
public:
    PairedEndReadFastqImpl(std::istream& in1_, std::istream& in2_) : in1(&in1_), in2(&in2_) {}
    PairedEndReadFastqImpl(std::vector<std::string> in1_, std::vector<std::string> in2_) : fin1(in1_), fin2(in2_) {}
protected:
    std::istream* in1, * in2 = 0;
    std::vector<std::string> fin1, fin2;
    bi::stream<bi::file_descriptor_source> fs1;
    bi::stream<bi::file_descriptor_source> fs2;
};

class InterReadImpl : public InputFastq {
public:
    InterReadImpl(std::istream& in1_) : in1(&in1_) {}
    InterReadImpl(std::vector<std::string> in_) : fin(in_) {}
protected:
    std::istream *in1;
    std::vector<std::string> fin;
    bi::stream<bi::file_descriptor_source> inter;
};

class SingleEndReadFastqImpl : public InputFastq {
public:
    SingleEndReadFastqImpl(std::istream& in_) : input(&in_) {}
    SingleEndReadFastqImpl(std::vector<std::string> in_) : finput(in_) {}
protected:
    std::istream* input = 0;
    std::vector<std::string> finput;
    bi::stream<bi::file_descriptor_source> fs;
};

class TabReadImpl : public InputFastq {
public:
    TabReadImpl(std::istream& in1_) : in1(&in1_) {}
    TabReadImpl(std::vector<std::string> in_) : fin(in_) {}
    std::vector<ReadPtr> load_read(std::istream *input);
protected:
    std::istream* in1;
    std::vector<std::string> fin;
    bi::stream<bi::file_descriptor_source> tabin;
    //to read the line
    std::string tabLine;
};


class inputReaders {
private:
  std::vector<std::string> r1_input;
  std::vector<std::string> r2_input;
  std::vector<std::string> se_input;
  std::vector<std::string> interleaved_input;
  std::vector<std::string> tab_input;

public:
    inputReaders(std::vector<std::string> r1_input_, std::vector<std::string> r2_input_,
                 std::vector<std::string> se_input_,
                 std::vector<std::string> interleaved_input_,
                 std::vector<std::string> tab_input_) :
        r1_input(r1_input_), r2_input(r2_input_), se_input(se_input_), interleaved_input(interleaved_input_), tab_input(tab_input_) { }

};

class OutputWriter {
public:
    virtual ~OutputWriter() {  }
    virtual void write(const PairedEndRead& ) { throw HtsRuntimeException("No PE implementation of write (Probably a SE read)"); }
    virtual void write(const SingleEndRead& ) { throw HtsRuntimeException("No SE implementaiton of write (Probably a PE read)"); }
    virtual void write_read(const Read &, bool ) { throw HtsRuntimeException("No write_read class, only accessable with SE"); } //only SE
};

class SingleEndReadOutFastq : public OutputWriter {
public:
    SingleEndReadOutFastq(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    virtual ~SingleEndReadOutFastq() {};
    virtual void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    virtual void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }
protected:
    std::shared_ptr<HtsOfstream> output = nullptr;

    void format_writer_rc(const Read &read) {
       *output << "@" << read.get_id_fastq("1") << '\n' << read.get_seq_rc() << "\n+\n" << read.get_qual_rc() << '\n';
    }
    void format_writer(const Read &read) {
       *output << "@" << read.get_id_fastq("1") << '\n' << read.get_sub_seq() << "\n+\n" << read.get_sub_qual() << '\n';
    }

};

class PairedEndReadOutFastq : public OutputWriter {
public:
    PairedEndReadOutFastq(std::shared_ptr<HtsOfstream> &out1_, std::shared_ptr<HtsOfstream> &out2_) : out1(out1_), out2(out2_) { }
    virtual ~PairedEndReadOutFastq() {};
    virtual void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
protected:
    std::shared_ptr<HtsOfstream> out1 = nullptr;
    std::shared_ptr<HtsOfstream> out2 = nullptr;
    void format_writer(const Read &read1, const Read &read2) {
        *out1 << "@" << read1.get_id_fastq("1") << '\n' << read1.get_sub_seq() << "\n+\n" << read1.get_sub_qual() << '\n';
        *out2 << "@" << read2.get_id_fastq("2") << '\n' << read2.get_sub_seq() << "\n+\n" << read2.get_sub_qual() << '\n';
    }
};

class PairedEndReadOutInter : public OutputWriter {
public:
    PairedEndReadOutInter(std::shared_ptr<HtsOfstream> &out_) : out1(out_) { }
    virtual ~PairedEndReadOutInter() {};
    virtual void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
protected:
    std::shared_ptr<HtsOfstream> out1 = nullptr;
    void format_writer(const Read &read1, const Read &read2) {
        *out1 << "@" << read1.get_id_fastq("1") << '\n' << read1.get_sub_seq() << "\n+\n" << read1.get_sub_qual() << '\n';
        *out1 << "@" << read2.get_id_fastq("2") << '\n' << read2.get_sub_seq() << "\n+\n" << read2.get_sub_qual() << '\n';
    }
};

/*Unmapped reads*/
class ReadBaseOutUnmapped : public OutputWriter {
public:
    ReadBaseOutUnmapped(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    virtual ~ReadBaseOutUnmapped() {};
    virtual void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
    virtual void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    virtual void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }

protected:
    std::shared_ptr<HtsOfstream> output = nullptr;

    /*sam format specs spaces are for readability
     * id \t bitwise flag \t rname \t pos \t mapQ \t CIGAR \t RNEXT \t PNEXT \t TLEN \t SEQ \t QUAL\n
     *
     * id = id
     * bitwas flag
     * SE - 4
     * PE R1 - 77
     * PE R2 - 141
     * RNAME - *
     * POS - 0
     * MAPQ - 0
     * CIGAR - *
     * RNEXT - *
     * PNEXT - 0
     * TLEN - 0
     * SEQ - seq
     * QUAL - qual */
    const size_t se_bitwise = 4;
    const size_t pe1_bitwise = 77;
    const size_t pe2_bitwise = 141;

    void samout(const Read &read, size_t bitwiseflag) {
        std::string sam_comment = "";
        for (auto const& s : read.get_comment()) { sam_comment = sam_comment + '\t' + s; }

        *output << read.get_id_first() << '\t'
            << bitwiseflag << '\t'
            << "*\t" /*RNAME*/
            << "0\t" /*POS*/
            << "0\t" /*MAPQ*/
            << "*\t" /*CIGAR*/
            << "*\t" /*RNEXT*/
            << "0\t" /*PNEXT*/
            << "0\t" /*TLEN*/
            << read.get_sub_seq() << "\t"
            << read.get_sub_qual() << sam_comment << "\n";
    }

    void samout_rc(const Read &read, size_t bitwiseflag) {
        std::string sam_comment = "";
        for (auto const& s : read.get_comment()) { sam_comment = sam_comment + '\t' + s; }

        *output << read.get_id_first() << '\t'
            << bitwiseflag << '\t'
            << "*\t" /*RNAME*/
            << "0\t" /*POS*/
            << "0\t" /*MAPQ*/
            << "*\t" /*CIGAR*/
            << "*\t" /*RNEXT*/
            << "0\t" /*PNEXT*/
            << "0\t" /*TLEN*/
            << read.get_seq_rc() << "\t"
            << read.get_qual_rc() << sam_comment << "\n";
    }

    /*Unmapped specs for SE reads*/
    void format_writer(const Read &read) {
        samout(read, se_bitwise);
    }

    void format_writer(const Read &read1, const Read &read2) {
        samout(read1, pe1_bitwise);
        samout(read2, pe2_bitwise);
    }

    void format_writer_rc(const Read &read) {
       samout_rc(read, se_bitwise);
    }
};


class ReadBaseOutTab : public OutputWriter {
public:
    ReadBaseOutTab(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    virtual ~ReadBaseOutTab() {};
    virtual void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
    virtual void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    virtual void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }

protected:
    std::shared_ptr<HtsOfstream> output = nullptr;

    void format_writer(const Read &read) {
        *output << read.get_id_tab("1") << '\t' << read.get_sub_seq() << '\t' << read.get_sub_qual();
        if (read.get_comment().size() > 0){
            *output << '\t' << strjoin(read.get_comment(), "|");
        }
        *output << '\n';
    }

    void format_writer(const Read &read1, const Read &read2) {
        *output << read1.get_id_tab("1") << '\t' << read1.get_sub_seq() << '\t' << read1.get_sub_qual() << '\t' << read2.get_id_tab("2") << '\t' << read2.get_sub_seq() << '\t' << read2.get_sub_qual();
        if (read1.get_comment().size() > 0 || read2.get_comment().size() > 0){
            std::vector <std::string> comment1 = read1.get_comment();
            std::vector <std::string> comment2 = read2.get_comment();
            std::string strComment = "";
            if (comment1.size() > 0){
                strComment += strjoin(comment1, "|");
            }
            strComment += '\t';
            if (comment2.size() > 0){
                strComment += strjoin(comment2, "|");
            }
            *output << '\t' << strComment;
        }
        *output << '\n';
    }

    void format_writer_rc(const Read &read) {
        *output <<  read.get_id_tab("1") << '\t' << read.get_seq_rc() << "\t" << read.get_qual_rc();
        if (read.get_comment().size() > 0){
            *output << '\t' << strjoin(read.get_comment(), "|");
        }
        *output << '\n';
    }
};

class WriterHelper : public ReadVisitor {
public:
    virtual ~WriterHelper() {}
    WriterHelper(std::shared_ptr<OutputWriter> pe_, std::shared_ptr<OutputWriter> se_,
                 bool stranded_ = false, bool no_orphans_ = false) :
        stranded(stranded_), no_orphans(no_orphans_), pe(pe_), se(se_) {}

    void operator() (ReadBase &read) {
        read.accept(*this);
    }

    void operator() (SingleEndRead &read) {
        visit(&read);
    }

    void operator() (PairedEndRead &read) {
        visit(&read);
    }

    virtual void visit(PairedEndRead *per) {
        Read &one = per->non_const_read_one();
        Read &two = per->non_const_read_two();

        if (!one.getDiscard() && !two.getDiscard()) {
            pe->write(*per);
        } else if (!one.getDiscard() && !no_orphans) { // Will never be RC
            one.join_comment(r2.get_comment());
            se->write_read(one, false);
        } else if (!two.getDiscard() && !no_orphans) { // if stranded RC
            two.join_comment(r1.get_comment());
            se->write_read(two, stranded);
        }

    }

    virtual void visit(SingleEndRead *ser) {
        if (! (ser->non_const_read_one()).getDiscard() ) {
            se->write(*ser);
        }
    }

private:
    bool stranded;
    bool no_orphans;
    std::shared_ptr<OutputWriter> pe;
    std::shared_ptr<OutputWriter> se;
};

#endif
