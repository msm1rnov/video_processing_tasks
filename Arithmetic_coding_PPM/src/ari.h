#pragma once
struct buff;
long long get_range (long long Tree[],short index);
void expand_range (long long Tree[], long long value, short index);
long long get_prob (long long Tree[],short index);
void  scail(long long Tree[]);
short get_index_of_range (long long Tree[],long long prob);
long long ReadBit (FILE* in, struct buff* buffer);
void WriteBit (FILE* out,long long bit,struct buff* buffer);
void bits_plus_follow (FILE* out,long long bit,long long* bits_to_follow, struct buff* buffer);
void table_change (short* mod, short current,long long prob1[],long long prob2[],short index[],short short_[],const long long Max_Of_Prob);
void code_current (short c,long long* l, long long* h, FILE* out, long long prob[],short index[],
                long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr,
                   struct buff* buffer);
void code_eof(long long* l, long long* h, FILE* out, long long prob[],
              short index[], long long* bits_to_follow, const long long Half,
              const long long First_qtr, const long long Third_qtr,struct buff* buffer);
long long decode_current(long long* value,long long* l, long long* h,
                FILE* in, long long prob[], const long long Half,
                const long long First_qtr, const long long Third_qtr,
                         struct buff* buffer) ;
void compress_ari(char *ifile, char *ofile);
void decompress_ari(char *ifile, char *ofile);
