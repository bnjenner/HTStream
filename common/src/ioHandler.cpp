#include "ioHandler.h"
#include <exception>
#include <cerrno>
#include <sstream>

void skip_lr(std::istream *input) {
    while(input and input->good() and (input->peek() == '\n' || input->peek() == '\r')) {
        input->get();
    }
}

void  __attribute__ ((noreturn)) throw_error(const std::string& filename) {
    throw HtsIOException(filename + ": " +  std::strerror( errno ));
}

/*
takes a delimited string and converts to fasta format for reading in
*/
std::string string2fasta(std::string seqstring,  std::string prefix, const char delim) {
  std::string newfa;
  int index=1;
  std::stringstream ss( seqstring );

  while( ss.good() )
  {
      std::string substr;
      getline( ss, substr, delim );
      newfa.append(">");
      newfa.append(prefix);
      newfa.append( std::to_string(index) );
      newfa.append( "\n" );
      newfa.append( substr );
      newfa.append( "\n" );
      index++;
  }
  return(newfa);
}

int check_open_r(const std::string& filename) {
    FILE* f = NULL;

    bf::path p(filename);
    if (!bf::exists(p)) {
        throw HtsIOException("File " + filename + " was not found.");
    }

    if (p.extension() == ".gz") {
        f = popen(("gunzip -c '" + filename + "'").c_str(), "r");
    } else {
        f = fopen(filename.c_str(), "r");
    }

    if (!f) {
        throw_error(filename);
    }
    return fileno(f);
}

int HtsOfstream::check_exists(const std::string& filename, bool force, bool gzip, bool std_out) {
    FILE* f = NULL;

    if (std_out) {
        return fileno(stdout);
    }
    std::string fname = gzip ? filename + ".gz" : filename ;

    bf::path p(fname);

    if (force || !bf::exists(p)) {
        if (gzip) {
            f = popen(("gzip > '" + fname + "'").c_str(), "w");
            gzfile = f;
        } else {
            f = fopen(fname.c_str(), "w");
        }
        if (!f) {
            throw_error(fname);
        }
        return fileno(f);
    } else {
        throw HtsIOException("File " + fname + " all ready exists. Please use -F or delete it\n");
    }
}

ReadPtr InputFastq::load_read(std::istream *input) {
    while(std::getline(*input, id) && id.size() < 1) {
    }
    if (id.size() < 1) {
        throw HtsIOException("invalid id line empty");
    }
    if (id[0] != '@') {
        throw HtsIOException("id line did not begin with @");
    }
    std::getline(*input, seq);
    if (seq.size() < 1) {
        throw HtsIOException("invalid seq line empty");
    }
    std::getline(*input, id2);
    if (id2.size() < 1) {
        throw HtsIOException("invalid id2 line empty");
    }
    if (id2[0] != '+') {
        throw HtsIOException("invalid id2 line did not begin with +");
    }
    std::getline(*input, qual);
    if (qual.size() != seq.size()) {
        throw HtsIOException("qual string not the same length as sequence");
    }

    // ignore extra lines at end of file
    while(input->good() and (input->peek() == '\n' || input->peek() == '\r')) {
        input->get();
    }
    return std::make_shared<Read>(seq, qual, id.substr(1));
}

ReadPtr InputFasta::load_read(std::istream *input) {
    while(std::getline(*input, id) && id.size() < 1) {
    }
    if (id.size() < 1) {
        throw HtsIOException("invalid id - line empty");
    }
    if (id[0] != '>') {
        throw HtsIOException("id line did not begin with >");
    }
    seq = "";
    while (std::getline(*input, tmpSeq)) {
        seq += tmpSeq;
        if (input->peek() == '>') {
            break;
        }
    }
    if (seq.size() < 1) {
        throw HtsIOException("no sequence");
    }
    while(input->good() and (input->peek() == '\n' || input->peek() == '\r')) {
        input->get();
    }
    return std::make_shared<Read>(seq, "", id.substr(1));

}

//Overrides load_read for tab delimited reads
std::vector<ReadPtr> TabReadImpl::load_read(std::istream *input) {

    std::vector <ReadPtr> reads(1);
    while(std::getline(*input, tabLine) && tabLine.size() < 1) {
    }

    std::vector <std::string> parsedRead;
    boost::split(parsedRead, tabLine, boost::is_any_of("\t"));


    if (parsedRead.size() != 3 && parsedRead.size() != 4 && parsedRead.size() != 5 && parsedRead.size() != 6 && parsedRead.size() != 8) {
        throw HtsIOException("There are not either 3 (SE, itab3), 4 (SE, itab with tags) 5 (PE, itab5), or 6 (PE, itab6), or 8 (PE, itab6 with tags) elements within a tab delimited file line");
    }

    if (parsedRead[1].size() != parsedRead[2].size()) {
        throw HtsIOException("sequence and qualities are not the same length 1");
    }

    reads[0] = std::make_shared<Read>(parsedRead[1], parsedRead[2], parsedRead[0]);

    if (parsedRead.size() == 4) {
        std::vector <std::string> comment;
        boost::split(comment, parsedRead[3], boost::is_any_of("|"));
        reads[0]->join_comment(comment);
    }
    if (parsedRead.size() == 5) {

        if (parsedRead[3].size() != parsedRead[4].size()) {
            throw HtsIOException("sequence and qualities are not the same length 2");
        }

        reads.push_back(std::make_shared<Read>(parsedRead[3], parsedRead[4], parsedRead[0]));
    }

    if (parsedRead.size() == 6) {

        if (parsedRead[4].size() != parsedRead[5].size()) {
            throw HtsIOException("sequence and qualities are not the same length 2");
        }

        reads.push_back(std::make_shared<Read>(parsedRead[4], parsedRead[5], parsedRead[3]));
    }

    if (parsedRead.size() == 8) {

        if (parsedRead[4].size() != parsedRead[5].size()) {
            throw HtsIOException("sequence and qualities are not the same length 2");
        }

        reads.push_back(std::make_shared<Read>(parsedRead[4], parsedRead[5], parsedRead[3]));
        std::vector <std::string> comment1;
        std::vector <std::string> comment2;
        boost::split(comment1, parsedRead[6], boost::is_any_of("|"));
        reads[0]->join_comment(comment1);
        boost::split(comment2, parsedRead[7], boost::is_any_of("|"));
        reads[1]->join_comment(comment2);
    }
    // ignore extra lines at end of file
    while(input->good() and (input->peek() == '\n' || input->peek() == '\r')) {
        input->get();
    }

    return reads;
}

template <>
InputReader<SingleEndRead, SingleEndReadFastqImpl>::value_type InputReader<SingleEndRead, SingleEndReadFastqImpl>::next() {
    return InputReader<SingleEndRead, SingleEndReadFastqImpl>::value_type(new SingleEndRead(load_read(input)));
}

template <>
bool InputReader<SingleEndRead, SingleEndReadFastqImpl>::has_next() {
    // ignore extra lines at end of file
    if (!(input and input->good()) and !finput.empty()){
        fs.open(check_open_r(finput.back()), bi::close_handle);
        input = &fs;
        finput.pop_back();
    }
    skip_lr(input);
    return (input and input->good());
};

template<>
InputReader<SingleEndRead, FastaReadImpl>::value_type InputReader<SingleEndRead, FastaReadImpl>::next() {
    return InputReader<SingleEndRead, FastaReadImpl>::value_type(new SingleEndRead(load_read(input)));
}

template <>
bool InputReader<SingleEndRead, FastaReadImpl>::has_next() {
    // ignore extra lines at end of file
    skip_lr(input);
    return (input and input->good());
};

template <>
InputReader<PairedEndRead, PairedEndReadFastqImpl>::value_type InputReader<PairedEndRead, PairedEndReadFastqImpl>::next() {
    ReadPtr r1 = load_read(in1);
    ReadPtr r2 = load_read(in2);
    return InputReader<PairedEndRead, PairedEndReadFastqImpl>::value_type(new PairedEndRead(r1, r2));
}

template <>
bool InputReader<PairedEndRead, PairedEndReadFastqImpl>::has_next() {
    // ignore extra lines at end of file
    if (!(in1 and in1->good() and in2 and in2->good()) and !fin1.empty()) {
        fs1.open(check_open_r(fin1.back()), bi::never_close_handle);
        fs2.open(check_open_r(fin2.back()), bi::never_close_handle);
        in1=&fs1;
        in2=&fs2;
        fin1.pop_back();
        fin2.pop_back();
    }
    skip_lr(in1);
    skip_lr(in2);
    return (in1 and in1->good() and in2 and in2->good());
};

template<>
InputReader<PairedEndRead, InterReadImpl>::value_type InputReader<PairedEndRead, InterReadImpl>::next() {
    ReadPtr r1 = load_read(in1);
    ReadPtr r2;
    try {
        r2 = load_read(in1);
    } catch (const std::exception&) {
        throw HtsIOException("odd number of sequences in interleaved file");
    }
    return InputReader<PairedEndRead, InterReadImpl>::value_type(new PairedEndRead(r1, r2));
}

template<>
bool InputReader<PairedEndRead, InterReadImpl>::has_next() {
    if (!(in1 and in1->good()) and !fin.empty()){
        inter.open(check_open_r(fin.back()), bi::close_handle);
        in1 = &inter;
        fin.pop_back();
    }
    skip_lr(in1);
    return(in1 && in1->good());
}

template <>
InputReader<ReadBase, TabReadImpl>::value_type InputReader<ReadBase, TabReadImpl>::next() {
    std::vector<ReadPtr> rs = load_read(in1);
    if (rs.size() == 1) {
        return InputReader<SingleEndRead, TabReadImpl>::value_type(new SingleEndRead(rs));
    }
    return InputReader<PairedEndRead, TabReadImpl>::value_type(new PairedEndRead(rs));
}

template <>
bool InputReader<ReadBase, TabReadImpl>::has_next() {
    // ignore extra lines at end of file
    if (!(in1 and in1->good()) and !fin.empty()){
        tabin.open(check_open_r(fin.back()), bi::close_handle);
        in1 = &tabin;
        fin.pop_back();
    }
    skip_lr(in1);
    return (in1 and in1->good());
}
