#pragma once

struct buff;
long long get_range_ppm (long long Tree[],short index);
void expand_range_ppm (long long Tree[], long long value, short index);
long long get_prob_ppm (long long Tree[],short index);
void  rescaling_ppm(long long Tree[]);
short get_index_of_range_ppm (long long Tree[],long long prob);
long long ReadBit_ppm (FILE* in, struct buff* buffer);
void WriteBit_ppm (FILE* out,long long bit,struct buff* buffer);
void bits_plus_follow_ppm (FILE* out,long long bit,long long* bits_to_follow, struct buff* buffer);
void code_current_ppm (short j,long long* l, long long* h, FILE* out, long long prob[],
                long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr,
                       struct buff* buffer);
void code_context_ppm(short* status, short cur_char, long long* l, long long* h, FILE* out, long long prob[],short index[],
        long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr, const long long Max_Of_Prob,
                      struct buff* buffer);
long long decode_current_ppm(long long* value,long long* l, long long* h,
                FILE* in, long long prob[], const long long Half,
                const long long First_qtr, const long long Third_qtr,
                             struct buff* buffer);
long long decode_context_ppm(short* status, long long* value, long long* l, long long* h, FILE* in , long long prob[],short index[],
       const long long Half, const long long First_qtr, const long long Third_qtr, const long long Max_Of_Prob,
                             struct buff* buffer);
void compress_ppm(char *ifile, char *ofile);
void decompress_ppm(char *ifile, char *ofile);
