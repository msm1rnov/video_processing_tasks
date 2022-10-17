
#include <stdlib.h>
#include <stdio.h>

#include "ppm.h"

struct buff {
    short bits_quantity;
    long long current_value;

};
long long get_range_ppm (long long* Tree, short index) {
    long long sum = Tree[0];
    while (index > 0) {
        sum += Tree[index];
        index = index & (index - 1);
    }
    return sum;
}

void expand_range_ppm (long long* Tree, long long value, short index) {
    while (index < 258) {
        Tree[index] += value;
        index += (index & (-index));
    }
}

long long get_prob_ppm (long long* Tree, short index) {
    long long value;
    short parent;
    value = Tree[index];
    if (index > 0) {
        parent = index & (index - 1);
        index -= 1;
        while (parent != index) {
            value -= Tree[index];
            index = index & (index - 1);
        }
    }
    return value;
}

void  rescaling_ppm(long long* Tree) {
    for (short i = 1; i < 258; i++) {
        long long temp = (get_prob_ppm(Tree, i) - 1) / 2;
        expand_range_ppm(Tree, -temp, i);
    }
}

short get_index_of_range_ppm (long long* Tree, long long prob) {
    short mask = 128;
    long long safe_prob = prob;
    short index = 0, test_index;
    if (prob > Tree[0]) {
        while (mask != 0) {
            test_index = index + mask;
            if(prob >= Tree[test_index]) {
                index = test_index;
                prob -= Tree[index];
            }
            mask /= 2;
        }
    }

    while (index < 257 && get_range_ppm(Tree, index) <= safe_prob) {
                index ++;
    }
    return index;
}
long long ReadBit_ppm (FILE* in, struct buff* buffer) {
    long long given_bit;
    if (!buffer->bits_quantity) {
      buffer->current_value=getc (in);
      buffer->bits_quantity = 8;
    }
    given_bit=buffer->current_value & 1;
    buffer->current_value >>= 1;
    buffer->bits_quantity -= 1;
    return given_bit;
}

void WriteBit_ppm (FILE* out, long long bit, struct buff* buffer) {
    buffer->current_value >>= 1;
  if (bit) buffer->current_value |= 128;
  buffer->bits_quantity --;
  if (!buffer->bits_quantity) {
    putc(buffer->current_value, out);
    buffer->bits_quantity = 8;
    buffer->current_value = 0;
  }
}

void bits_plus_follow_ppm (FILE* out, long long bit, long long* bits_to_follow, struct buff* buffer) {
    WriteBit_ppm(out, bit, buffer);
    while (*bits_to_follow > 0) {
        WriteBit_ppm(out, !bit,buffer);
        *bits_to_follow -= 1;
    }
}

void code_current_ppm (short j,long long* l, long long* h, FILE* out, long long* prob,
                long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr,
                        struct buff* buffer) {

    long long interval = *h - *l + 1;
    long long delitel = get_range_ppm(prob, 257);

    *h = *l + get_range_ppm(prob, j) * interval / delitel - 1;
    *l = *l + get_range_ppm(prob, j - 1) * interval / delitel;

    for(;;) {
        if(*h < Half) {
            bits_plus_follow_ppm(out, 0, bits_to_follow, buffer);
        }
        else {
            if(*l >= Half) {
                bits_plus_follow_ppm(out, 1, bits_to_follow, buffer);
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

void code_context_ppm(short* status, short cur_char, long long* l, long long* h, FILE* out, long long* prob,short index[],
        long long* bits_to_follow, const long long Half, const long long First_qtr, const long long Third_qtr, const long long Max_Of_Prob, struct buff* buffer) {
    short j = index[cur_char];
    if (get_prob_ppm(prob, j) > 0) {
        *status = 1;
        code_current_ppm(j, l, h, out, prob, bits_to_follow, Half, First_qtr, Third_qtr, buffer);
    }
    else {
        *status = 0;
        if (get_prob_ppm(prob, 257) > 0) {
            code_current_ppm(257, l, h, out, prob, bits_to_follow, Half, First_qtr, Third_qtr, buffer);
        }
    }
}

void compress_ppm(char *ifile, char *ofile) {


    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    short index[257];


   long long *** global_model = (long long ***) malloc(257 * sizeof(long long**));
   for (int i = 0; i < 257; i++) {
       global_model[i] = (long long **) malloc(257 * sizeof(long long*));
       for(int j = 0; j < 257; j++) {
           global_model[i][j] = (long long *) malloc(258 * sizeof(long long ));

       }
   }

   for (int i = 0; i < 257; i++) {
       for (int j = 0;j < 257; j++) {
           for (int h = 0;h < 257; h++) {
               global_model[i][j][h] = 0;
           }
       }
   }

    for (short i = 0; i < 257; i++) {
          index[i] = i + 1;
    }

    for (short i = 1; i < 258; i++){
        expand_range_ppm(global_model[256][256], 20, i);
     }

    long long l = 0;
    long long bits_to_follow = 0;
    long long h = 4294967296;
    long long Max_Of_Prob = 32767;

    long long First_qtr = (h + 1) / 4;
    long long Half = First_qtr * 2;
    long long Third_qtr = First_qtr * 3;

    struct buff buffer;
    buffer.current_value = 0;
    buffer.bits_quantity = 8;
    
    short c, status;
    short context[2];
    context[0] = 1;
    context[1] = 1;

    c = getc(ifp);

    while(!feof(ifp)) {
        short status_1 = 1, status_2 = 1, status_3 = 1;
        code_context_ppm(&status, c, &l, &h, ofp, global_model[context[1]][context[0]], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
        status_1 = status;

        if (status == 1) {

        } else {
            code_context_ppm(&status, c, &l, &h, ofp, global_model[256][context[0]], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
            status_2 = status;
            if (status == 1) {

            } else {
                code_context_ppm(&status, c, &l, &h, ofp, global_model[256][256], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
                status_3 = status;
            }
        }
       short  j = c + 1;

       if (status_1 == 0 && status_2 == 0) {
           if (get_range_ppm(global_model[256][256], 257) >= Max_Of_Prob) {
               rescaling_ppm(global_model[256][256]);
           }
           if (get_prob_ppm(global_model[256][256],j) == 0) {
               expand_range_ppm(global_model[256][256], 20, 257);
           }
           expand_range_ppm(global_model[256][256], 20, j);
       }
       if (status_1 == 0) {
           if (get_range_ppm(global_model[256][context[0]], 257) >= Max_Of_Prob) {
               rescaling_ppm(global_model[256][context[0]]);
           }
           if (get_prob_ppm(global_model[256][context[0]],j) == 0) {
               expand_range_ppm(global_model[256][context[0]], 20, 257);
           }
           expand_range_ppm(global_model[256][context[0]], 20, j);
      }
       if (get_range_ppm(global_model[context[1]][context[0]], 257) >= Max_Of_Prob) {
               rescaling_ppm(global_model[context[1]][context[0]]);
           }
           if (get_prob_ppm(global_model[context[1]][context[0]],j) == 0) {
               expand_range_ppm(global_model[context[1]][context[0]], 20, 257);
           }
           expand_range_ppm(global_model[context[1]][context[0]], 20, j);
           context[1] = context[0];
           context[0] = c;
           c = getc (ifp);
    }
    code_context_ppm(&status, 256, &l, &h, ofp, global_model[context[1]][context[0]], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);


    code_context_ppm(&status, 256, &l, &h, ofp, global_model[256][context[0]], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);

    code_context_ppm(&status, 256, &l, &h, ofp, global_model[256][256], index, &bits_to_follow, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
    short bit_temp;
    if (l < First_qtr) {
        bit_temp = 0;
    } else {
        bit_temp = 1;
    }
    bits_to_follow ++;
    bits_plus_follow_ppm (ofp, bit_temp, &bits_to_follow, &buffer);
    buffer.current_value>>=(buffer.bits_quantity);
    putc(buffer.current_value, ofp);

    for (int i = 0; i < 257; i++) {

        for(int j = 0; j < 257; j++) {
            free(global_model[i][j]);
        }
        free(global_model[i]);
    }
    free(global_model);
    fclose(ifp);
    fclose(ofp);

}


long long decode_current_ppm(long long* value,long long* l, long long* h,
                FILE* in, long long* prob, const long long Half,
                const long long First_qtr, const long long Third_qtr,
                struct buff* buffer) {
    short j=0;
    long long delitel = get_range_ppm(prob, 257);

    long long freq = ((*value - *l + 1) * delitel - 1) / (*h - *l + 1);

    j = get_index_of_range_ppm(prob, freq);

    long long length = *h - *l + 1;
    *h = *l + get_range_ppm(prob, j) * length / delitel - 1;
    *l = *l + get_range_ppm(prob, j - 1) * length / delitel;

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
        long long bit_t = ReadBit_ppm(in, buffer);
        *value=((*value) << 1) + bit_t;

    }

    return j;


}

long long decode_context_ppm(short* status, long long* value, long long* l, long long* h, FILE* in , long long* prob,short index[], const long long Half, const long long First_qtr, const long long Third_qtr, const long long Max_Of_Prob,
                struct buff* buffer) {
    short j;
    if (get_prob_ppm(prob, 257) > 0) {

        j = decode_current_ppm(value, l, h, in, prob, Half, First_qtr, Third_qtr,buffer);
        if (j == 257) {
            *status = 0;
            return 257;
        } else {
            *status = 1;
            return j;

        }
    }
    else {
        *status = 0;
        return 0;
    }
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    short index[257];
    short short_[258];

    long long *** global_model = (long long ***) malloc(257 * sizeof(long long**));
    for (int i = 0; i < 257; i++) {
        global_model[i] = (long long **) malloc(257 * sizeof(long long*));
        for(int j = 0; j < 257; j++) {
            global_model[i][j] = (long long *) malloc(258 * sizeof(long long ));

        }
    }

    for (int i = 0; i < 257; i++) {
       for (int j = 0; j < 257; j++) {
           for (int h = 0; h < 257; h++) {
               global_model[i][j][h] = 0;
           }
       }
    }
    for (short i = 0; i < 257; i++) {
          index[ i ] = i + 1;
          short_[i + 1] = i;
    }

    for (short i = 1; i < 258; i++){
        expand_range_ppm(global_model[256][256], 20, i);
    }
    
    long long l = 0;
    long long h = 4294967296;
    long long Max_Of_Prob = 32767;

    struct buff buffer;
    buffer.bits_quantity = 0;
    buffer.current_value = 0;

    long long value = 0;
    for (short i = 0; i < 32; i++) {
        long long temp_bit = ReadBit_ppm(ifp, &buffer);
        value = value << 1;
        value += temp_bit;
    }
    long long First_qtr = (h + 1) / 4;
    long long Half = First_qtr * 2;
    long long Third_qtr = First_qtr * 3;

    short status;
    short context[2];
    context[0] = 1;
    context[1] = 1;


    while(1) {
        short status_1 = 1, status_2 = 1;
        long long current;
        current = decode_context_ppm(&status, &value, &l, &h, ifp, global_model[context[1]][context[0]], index, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
        status_1 = status;
        if (status == 1) {

        } else {
            current = decode_context_ppm(&status, &value, &l, &h, ifp, global_model[256][context[0]], index, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);
            status_2 = status;
            if (status == 1 ) {

            } else {
                current = decode_context_ppm(&status, &value, &l, &h, ifp, global_model[256][256], index, Half, First_qtr, Third_qtr, Max_Of_Prob, &buffer);

                if (current==257) {
                    break;
                }
            }
        }

        short current_short = short_[current];

        if (status_1 == 0 && status_2 == 0) {
            if (get_range_ppm(global_model[256][256],257) >= Max_Of_Prob) {
                rescaling_ppm(global_model[256][256]);
            }

            if (get_prob_ppm(global_model[256][256],current) == 0) {
                expand_range_ppm(global_model[256][256], 20, 257);
            }
            expand_range_ppm(global_model[256][256], 20, current);

        }
        if (status_1 == 0) {
            if (get_range_ppm(global_model[256][context[0]], 257) >= Max_Of_Prob) {
                rescaling_ppm(global_model[256][context[0]]);
            }

            if (get_prob_ppm(global_model[256][context[0]], current) == 0) {
                expand_range_ppm(global_model[256][context[0]], 20, 257);
            }
            expand_range_ppm(global_model[256][context[0]], 20, current);
       }
        if (get_range_ppm(global_model[context[1]][context[0]], 257) >= Max_Of_Prob) {
                rescaling_ppm(global_model[context[1]][context[0]]);
            }

            if (get_prob_ppm(global_model[context[1]][context[0]], current) == 0) {
                expand_range_ppm(global_model[context[1]][context[0]], 20, 257);
            }
            expand_range_ppm(global_model[context[1]][context[0]], 20, current);

            putc(current_short,ofp);

            context[1] = context[0];
            context[0] = current_short;

    }


    for (int i = 0; i < 257; i++) {
        for(int j = 0; j < 257; j++) {
            free(global_model[i][j]);
        }
        free(global_model[i]);
    }
    free(global_model);
    fclose(ifp);
    fclose(ofp);
}
