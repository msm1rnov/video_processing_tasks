#include <stdlib.h>
#include <stdio.h>

#include "ari.h"


struct buff {
    short bits_quantity;
    long long current_value;

};
long long get_range (long long Tree[], short index) {
    long long sum = Tree[0];
    while (index > 0) {
        sum += Tree[index];
        index = index&(index - 1);
    }
    return sum;
}

void expand_range (long long Tree[], long long value, short index) {

    while (index < 258) {
        Tree[index] += value;
        index += (index & (-index));

    }
}

long long get_prob (long long Tree[], short index) {
    long long value;
    short parent;
    value = Tree[index];
    if (index > 0) {
        parent = index&(index - 1);
        index -= 1;
        while (parent != index) {
            value -= Tree[index];
            index = index&(index - 1);

        }
    }
    return value;
}

void  scail(long long Tree[]) {

    for (short i = 1; i < 258; i++) {
        long long temp = (get_prob(Tree, i) - 1) / 2;
        expand_range(Tree, -temp, i);

    }
}


short get_index_of_range (long long Tree[], long long prob) {
    short mask = 128;
    long long safe_prob = prob;
    short index = 0, test_index;

    if (prob > Tree[0]) {
        while (mask != 0) {

            test_index = index+mask;

            if(prob >= Tree[test_index]) {
                index = test_index;
                prob -= Tree[index];
            }
            mask /= 2;
        }

    }
    while (index < 257 && get_range(Tree, index) <= safe_prob) {

                index ++;
            }
    return index;
}

long long ReadBit (FILE* in, struct buff* buffer) {
    long long given_bit;
    if (!buffer->bits_quantity) {
      buffer->current_value = getc (in);
      buffer->bits_quantity = 8;
    }
    given_bit=buffer->current_value & 1;
    buffer->current_value >>= 1;
    buffer->bits_quantity -= 1;
    return given_bit;
}

void WriteBit (FILE* out, long long bit, struct buff* buffer) {
    buffer->current_value >>= 1;
  if (bit) buffer->current_value |= 128;
  buffer->bits_quantity --;
  if (!buffer->bits_quantity) {
    putc(buffer->current_value, out);
    buffer->bits_quantity = 8;
    buffer->current_value = 0;
  }
}

void bits_plus_follow (FILE* out,long long bit,long long* bits_to_follow, struct buff* buffer) {
    WriteBit(out,bit,buffer);
    while (*bits_to_follow > 0) {
        WriteBit(out,!bit,buffer);
        *bits_to_follow -= 1;
    }
}

void table_change (short* mod, short current, long long prob1[], long long prob2[], short index[], short short_[], const long long Max_Of_Prob) {

    short cur_ind = index[current];
       
    if (get_range(prob1, 257) >= Max_Of_Prob) {
         scail(prob1);
      }
    if (get_range(prob2, 257) >= Max_Of_Prob) {
         scail(prob2);
      }

    *mod = 0;
    expand_range(prob1, 1048575, cur_ind);

}

void code_current (short c,long long* l, long long* h, FILE* out, long long prob[], short index[],
                long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr,
                        struct buff* buffer) {

    short j = index[c];

    long long interval = *h - *l + 1;
    long long delitel = get_range(prob, 257);

    *h = *l + get_range(prob, j) * interval / delitel - 1;
    *l = *l + get_range(prob, j - 1) * interval / delitel;

    for(;;) {
        if(*h < Half) {
            bits_plus_follow(out, 0, bits_to_follow, buffer);
        }
        else {

            if(*l >= Half) {

                bits_plus_follow(out, 1, bits_to_follow, buffer);

                *l -= Half; *h -= Half;

            }
            else if((*l >= First_qtr) && (*h < Third_qtr)){
                    *bits_to_follow += 1;
                    *l -= First_qtr; *h -= First_qtr;
                }
                else {
                    break;
                }
        }
        *l <<= 1;
        *h <<= 1;
        *h |= 1;
    }
}

void code_eof(long long* l, long long* h, FILE* out, long long prob[],
                        short index[], long long* bits_to_follow, const long long Half,
                            const long long First_qtr, const long long Third_qtr,struct buff* buffer) {
    short j = 257;
    long long interval = *h - *l + 1;
    long long delitel = get_range(prob, 257);
    *h = *l + get_range(prob, j) * interval / delitel - 1;
    *l = *l + get_range(prob, j - 1) * interval / delitel;

    for(;;) {
        if(*h < Half) {
            bits_plus_follow(out, 0, bits_to_follow, buffer);
        }
            else {
                if(*l >= Half) {
                    bits_plus_follow(out, 1, bits_to_follow, buffer);
                    *l -= Half; *h -= Half;
                }
                else if((*l >= First_qtr)&&(*h < Third_qtr)){
                        *bits_to_follow += 1;
                        *l -= First_qtr; *h -= First_qtr;
                    }
                    else break;
            }
        *l <<= 1;
        *h <<= 1;
        *h |= 1;
    }
}


void compress_ari(char *ifile, char *ofile) {

    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    short index[256];
    short short_[257];
    long long prob1[258];
    long long prob2[258];


    for (short i = 0; i < 256; i++) {
          index[i] = i + 1;
          short_[i+1] = i;
    }

  
    for (short i = 0; i < 258; i++) prob1[i] = 0;
    for (short i = 1; i < 258; i++){
        expand_range(prob1, 30, i);
     }
    for (short i = 0; i < 258; i++) prob2[i] = 0;
    for (short i = 1; i < 258; i++){
          expand_range(prob2, 4000, i);
     }

    long long l = 0;
    long long bits_to_follow = 0;
    long long h = 4294967296;
    long long Max_Of_Prob = 1073741823;


    long long First_qtr = (h+1)/4;
    long long Half = First_qtr*2;
    long long Third_qtr = First_qtr*3;

    struct buff buffer;
    buffer.current_value = 0;
    buffer.bits_quantity = 8;

    
    short c;
    c = getc(ifp);
    short mod = 1;
    short k = 0;
    while(!feof(ifp)) {
        k ++;
        if (mod == 0) code_current(c, &l, &h, ofp, prob1, index, &bits_to_follow, Half, First_qtr, Third_qtr,&buffer);
        if (mod == 1) code_current(c, &l, &h, ofp, prob2, index, &bits_to_follow, Half, First_qtr, Third_qtr,&buffer);
        table_change(&mod, c, prob1, prob2, index, short_, Max_Of_Prob);
        c = getc (ifp);

    }



    // выбор двоичного числа из полученного интервала
    if (mod == 0)  code_eof(&l, &h, ofp, prob1, index, &bits_to_follow, Half, First_qtr, Third_qtr,&buffer);

    if (mod == 1)  code_eof(&l, &h, ofp, prob2, index, &bits_to_follow, Half, First_qtr, Third_qtr,&buffer);

    short bit_temp;
    if (l < First_qtr) {
        bit_temp = 0;
    } else {
        bit_temp = 1;
    }
    bits_to_follow ++;
    bits_plus_follow (ofp, bit_temp, &bits_to_follow, &buffer);

    buffer.current_value >>= (buffer.bits_quantity);
    putc(buffer.current_value, ofp);

    fclose(ifp);
    fclose(ofp);

}


long long decode_current(long long* value,long long* l, long long* h,
                FILE* in, long long prob[], const long long Half,
                const long long First_qtr, const long long Third_qtr,
                struct buff* buffer) {
   short j = 0;
    long long delitel = get_range(prob, 257);


    long long freq = ((*value - *l + 1) * delitel - 1) / (*h - *l + 1);

    j = get_index_of_range(prob, freq);
    
    long long length = *h - *l + 1;
    *h=*l + get_range(prob, j)*length / delitel - 1;
    *l=*l + get_range(prob, j - 1) * length / delitel;

    while(1){
        if(*h < Half) {

        }
        else if(*l >= Half) {

                *value -= Half;
                *l -= Half;

                *h -= Half;
            }
            else
                if ((*l >= First_qtr) && (*h < Third_qtr)) {

                    *value -= First_qtr;
                    *l -= First_qtr;
                    *h -= First_qtr;

                } else break;
        *l <<= 1;
        *h <<= 1;
        *h |= 1;
        long long bit_t = ReadBit(in, buffer);
        *value=((*value) << 1) + bit_t;
    }

    return j;
}

void decompress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    short index[256];
    short short_[257];
    long long prob1[258];
    long long prob2[258];



    for (short i = 0; i < 256; i++) {
          index[i] = i + 1;
          short_[i + 1] = i;
    }


    for (short i = 0; i < 258; i++) prob1[i] = 0;
    for (short i = 1; i < 258; i++){
        expand_range(prob1, 30, i);
     }
    for (short i = 0; i < 258; i++) prob2[i] = 0;
    for (short i = 1; i < 258; i++){
        expand_range(prob2, 4000, i);
     }


    long long l = 0;
    long long h = 4294967296;
    long long Max_Of_Prob = 1073741823;

    struct buff buffer;
    buffer.bits_quantity = 0;
    buffer.current_value = 0;

    long long value = 0;
    for (short i = 0; i < 32; i++) {
        long long temp_bit = ReadBit(ifp, &buffer);
        value = value << 1;
        value += temp_bit;
    }


    long long First_qtr = (h + 1) / 4;
    long long Half = First_qtr * 2;
    long long Third_qtr = First_qtr * 3;
    short k = 0;
        short mod = 1;
    while(1) {
        long long current;
        if (mod == 0) current = decode_current(&value, &l, &h, ifp, prob1,Half, First_qtr, Third_qtr, &buffer);
        if (mod == 1) current = decode_current(&value, &l, &h, ifp, prob2,Half, First_qtr, Third_qtr, &buffer);
        if (current == 257) {
            break;
        }
        k ++;
        short current_short = short_[current];

        putc(current_short, ofp);
        table_change(&mod, current_short, prob1, prob2, index, short_, Max_Of_Prob);
    }



    fclose(ifp);
    fclose(ofp);
}
