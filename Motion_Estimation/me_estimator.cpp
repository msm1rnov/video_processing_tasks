#include "me_estimator.h"
#include "metric.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <set>

#include <fstream>

MotionEstimator::MotionEstimator(size_t width, size_t height, unsigned char quality, bool use_half_pixel)
    : width(width)
    , height(height)
    , quality(quality)
    , use_half_pixel(use_half_pixel)
    , width_ext(width + 2 * BORDER)
    , num_blocks_hor((width + BLOCK_SIZE - 1) / BLOCK_SIZE)
    , num_blocks_vert((height + BLOCK_SIZE - 1) / BLOCK_SIZE)
    , first_row_offset(width_ext * BORDER + BORDER)
    , me_field(num_blocks_hor, num_blocks_vert, BLOCK_SIZE)
    ,first_time(true)
    ,prev_me_field(num_blocks_hor, num_blocks_vert, BLOCK_SIZE)
    , width_borders(width + 2 * BORDER)
    , height_borders(height + 2 * BORDER)
    
{    
    if (quality == 20) {
        BLOCK_SIZE = 16;
    }
    if (quality == 40) {
        BLOCK_SIZE = 16;
    }
    if (quality == 60) {
        BLOCK_SIZE = 16;
    }
    if (quality == 80) {
        BLOCK_SIZE = 16;
    }
    if (quality == 100) {
        BLOCK_SIZE = 8;
    }

    num_blocks_hor = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    num_blocks_vert = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;

    me_field = MEField(num_blocks_hor, num_blocks_vert, BLOCK_SIZE);
   
    prev_me_field = MEField(num_blocks_hor, num_blocks_vert, BLOCK_SIZE);
    
    
    cur_Y_borders = new unsigned char[width_borders * height_borders];
    prev_Y_borders = new unsigned char[width_borders * height_borders];
    vector_candidates= new std::vector<MV>[num_blocks_vert * num_blocks_hor];
    if (use_half_pixel) {
        prev_Y_up_borders = new unsigned char[width_borders * height_borders];
        prev_Y_up_left_borders = new unsigned char[width_borders * height_borders];
        prev_Y_left_borders = new unsigned char[width_borders * height_borders];
    }
    

}

    MotionEstimator::~MotionEstimator() {
        delete[] cur_Y_borders;
        delete[] prev_Y_borders;
        
        delete[] vector_candidates;
        if (use_half_pixel) {
            delete[] prev_Y_up_borders;
            delete[] prev_Y_up_left_borders;
            delete[] prev_Y_left_borders;
        }

    }



    void extend_with_borders(
        unsigned char *input,
        unsigned char *output,
        size_t height,
        size_t width,
        size_t border_size
    ) {
    

    // Copy frame to center of new
    size_t new_width = width + 2 * border_size;
    auto p_output = output + new_width * border_size + border_size;
    auto p_input = input;
    for (size_t y = 0; y < height; ++y, p_output += 2 * border_size) {
        for (size_t x = 0; x < width; ++x, ++p_output, ++p_input) {
            *p_output = *p_input;
        }
    }
 
    // Left and right borders.
    p_output = output + new_width * border_size;
    for (size_t y = 0; y < height; ++y) {
        memset(p_output, p_output[border_size], border_size);
        p_output += border_size + width;
        memset(p_output, p_output[-1], border_size);
        p_output += border_size;
    }
    
    // Top and bottom borders.
    p_output = output;
    auto p_output_reference_row = p_output + new_width * border_size;

    for (size_t y = 0; y < border_size; ++y) {
        memcpy(p_output, p_output_reference_row, new_width);
        p_output += new_width;
    }
    
    p_output = output + new_width * (height + border_size);
    p_output_reference_row = p_output_reference_row - new_width;

    for (size_t y = 0; y < border_size; ++y) {
        memcpy(p_output, p_output_reference_row, new_width);
        p_output += new_width;
    }

}

void MotionEstimator::CEstimate(const unsigned char* cur_Y,
                               const unsigned char* prev_Y,
                               const uint8_t* prev_Y_up,
                               const uint8_t* prev_Y_left,
                               const uint8_t* prev_Y_upleft,
                               MEField& mvectors ) {
    std::unordered_map<ShiftDir, const uint8_t*, ShiftDirHash> prev_map {
        { ShiftDir::NONE, prev_Y }
    };

    if (use_half_pixel) {
        prev_map.emplace(ShiftDir::UP, prev_Y_up);
        prev_map.emplace(ShiftDir::LEFT, prev_Y_left);
        prev_map.emplace(ShiftDir::UPLEFT, prev_Y_upleft);
    }
    
    if (quality == 0 || quality == 20 || quality == 40 || quality == 60  || quality == 80 || quality == 100 ) {
    
        std::vector<MV> diamond_vector;
        MV diam;
        diam.x =- 1; diam.y = 0;
        diamond_vector.push_back(diam);
        diam.x = 1; diam.y = 0;
        diamond_vector.push_back(diam);
        diam.x = 0; diam.y = -1;
        diamond_vector.push_back(diam);
        diam.x = 0; diam.y = 1;
        diamond_vector.push_back(diam);
  
    
        int diamond_vector_size = diamond_vector.size();
        int expect = 0;
        
      
        for (size_t i = 0; i < num_blocks_vert; ++i) {
            for (size_t j = 0; j < num_blocks_hor; ++j) {

                const auto hor_offset = j * BLOCK_SIZE;
                const auto vert_offset = first_row_offset + i * BLOCK_SIZE * width_ext;
                const auto cur = cur_Y + vert_offset + hor_offset;

            if (i >= 1 && j >= 1 && i < num_blocks_vert && j < num_blocks_hor - 1) {
                
                
                vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j - 1, i));
                vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j - 1, i - 1));
                vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j, i - 1));
                vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j + 1, i - 1));
                
            } else {
                if (i >= 1 && j >= 0 && i < num_blocks_vert && j < 1) {
                    vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j, i - 1));
                    vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j + 1, i - 1));
                } else {
               
                    if (i==0 && j==0) {
                        
                        MV rrr;
                        rrr.error = std::numeric_limits<long>::max();
                        const auto prev = prev_map[ShiftDir::NONE] + vert_offset + hor_offset;
                        for (int y = -1;y <= 1 ; ++y) {
                            for (int x = -1; x <= 1; ++x) {
                                const auto comp = prev + y * width_ext + x;
                                const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE);
                                if (error < rrr.error) {
                                    rrr.x = x;
                                    rrr.y = y;
                                    rrr.shift_dir = ShiftDir::NONE;
                                    rrr.error = error;
                                }
                                
                            }
                        }
                       
                        vector_candidates[j + i * num_blocks_hor].push_back(rrr);
                      
                        
                    } else {
                        if (i >= 1 && j >= num_blocks_hor - 1 && i < num_blocks_vert && j < num_blocks_hor) {
                            vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j - 1, i));
                            vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j - 1, i - 1));
                            vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j, i - 1));
                        } else {
                            vector_candidates[j + i * num_blocks_hor].push_back(mvectors.get_mv(j - 1, i));
                        }
                    }
                }
            }
               
                
                int THRESHOLD = 10;
              
                int max_delta, max_delta_x = 0, max_delta_y = 0, sum_cand_x = 0, sum_cand_y = 0, mean_cand_x, mean_cand_y;
                
                int vect_size = vector_candidates[j + i * num_blocks_hor].size();
    
                for (int k_1 = 0; k_1 < vect_size; k_1++) {
                    
                    MV first_vector = vector_candidates[j + i * num_blocks_hor][k_1];
                    sum_cand_x += first_vector.x;
                    sum_cand_y += first_vector.y;
                    for (int k_2 = k_1 + 1; k_2 < vect_size; k_2 ++) {
                    
                        int delta_x = abs(first_vector.x - vector_candidates[j + i * num_blocks_hor][k_2].x);
                        int delta_y = abs(first_vector.y - vector_candidates[j + i * num_blocks_hor][k_2].y);
                        if (delta_x > max_delta_x) {
                            max_delta_x = delta_x;
                        }
                        if (delta_y > max_delta_y) {
                            max_delta_y = delta_y;
                        }
                    }
                }
            
                if (max_delta_y > max_delta_x) {
                    max_delta = max_delta_y;
                } else {
                    max_delta = max_delta_x;
                }
          
                MV best_vector;
                best_vector.error = std::numeric_limits<long>::max();
         
                if (max_delta < THRESHOLD) {
                    
                    mean_cand_x = sum_cand_x / vect_size;
                    mean_cand_y = sum_cand_y / vect_size;
                    best_vector.x = mean_cand_x;
                    best_vector.y = mean_cand_y;
             
                    const auto prev = prev_map[ShiftDir::NONE] + vert_offset + hor_offset;
                    const auto comp = prev + best_vector.y * width_ext + best_vector.x;
                    const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE);
                    best_vector.error = error;
                    best_vector.shift_dir = ShiftDir::NONE;
            
                
               } else {
                    std::set<int> to_erase;
                
                    for (int k_1 = 0; k_1 < vect_size; k_1++) {
                        if (to_erase.count(k_1) == 0) {
                            for (int k_2 = k_1 + 1; k_2 < vect_size; k_2 ++) {
                                if (to_erase.count(k_2) == 0) {
                                    int delta_x = abs(vector_candidates[j+i*num_blocks_hor][k_1].x - vector_candidates[j + i * num_blocks_hor][k_2].x);
                                    int delta_y = abs(vector_candidates[j + i * num_blocks_hor][k_1].y - vector_candidates[j + i * num_blocks_hor][k_2].y);
                                    int dist = delta_x + delta_y;
                                    if (dist < 4) {
                                        to_erase.insert(k_2);
                                    }

                                }
                            }
                        }
                    }
                    
                    int zap = 0;
                    for (auto el: to_erase) {
                        vector_candidates[j + i * num_blocks_hor].erase(vector_candidates[j + i * num_blocks_hor].begin() + el - zap);
                        zap ++;
                    }
                        
                   int new_vector_size = vector_candidates[j + i * num_blocks_hor].size();
                   const auto prev = prev_map[ShiftDir::NONE] + vert_offset + hor_offset;
                   for (int el = 0; el < new_vector_size; el++) {
                            const auto comp = prev + vector_candidates[j+i*num_blocks_hor][el].y * width_ext + vector_candidates[j+i*num_blocks_hor][el].x;
                            const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE);
                            if (error < best_vector.error) {
                                best_vector.x = vector_candidates[j + i * num_blocks_hor][el].x;
                                best_vector.y = vector_candidates[j + i * num_blocks_hor][el].y;
                                best_vector.shift_dir = ShiftDir::NONE;
                                best_vector.error = error;
                            }
                   }
                  
              }

            int max_steps = 40;
            int count_steps = 0;
            int min_index = 0;
                
            bool flag = true;
            
            
                
            while ((count_steps < max_steps) && flag) {
                    const auto prev = prev_map[ShiftDir::NONE] + vert_offset + hor_offset;
                    const auto prev_new = prev + best_vector.y * int(width_ext) + best_vector.x;
                    flag = false;
                    MV gogol;
                    gogol.x = 0;
                    gogol.y = 0;
                    gogol.error=best_vector.error;
                    for (int k = 0; k < diamond_vector_size; k++) {
                        const auto comp = prev_new +  diamond_vector[k].y * int(width_ext) +  diamond_vector[k].x;
                        const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE);
     
                        if (error < gogol.error) {
                            gogol.x = diamond_vector[k].x;
                            gogol.y = diamond_vector[k].y;
                            gogol.shift_dir = ShiftDir::NONE;
                            gogol.error = error;
                            flag = true;
                        }
                    }
                    best_vector.x += gogol.x;
                    best_vector.y += gogol.y;
                    best_vector.error = gogol.error;
                    count_steps ++;
                }
                
                
                for (const auto& prev_pair : prev_map) {
                    if (prev_pair.first!=ShiftDir::NONE) {
                        const auto prev = prev_pair.second + vert_offset + hor_offset;
                        const auto comp = prev + best_vector.y * width_ext + best_vector.x;
                        const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE);
                        if (error < best_vector.error) {
                            
                            best_vector.shift_dir = prev_pair.first;
                            best_vector.error = error;
                        }
                    }
                }
                 
                expect += best_vector.error;
                mvectors.set_mv(j, i, best_vector);
              
            }
         }
                
        double expect_main = expect / (num_blocks_vert * num_blocks_hor);
        double disp = 0;
        
        for (size_t i = 0; i < num_blocks_vert; ++i) {
            for (size_t j = 0; j < num_blocks_hor; ++j) {
                MV temp = mvectors.get_mv(j,i);
               disp += pow(temp.error - expect_main, 2);
              vector_candidates[i * num_blocks_hor + j].clear();
            }
        }
    
        double dev = disp / (num_blocks_vert * num_blocks_hor);
        dev = sqrt(dev);
        
        if (!first_time) {
            expectation = coeff_expectation*expect_main + (1 - coeff_expectation) * expectation;
            deviation = coeff_deviation*dev + (1 - coeff_deviation) * deviation;
        } else {
            expectation = coeff_expectation * expect_main;
            deviation = coeff_deviation * dev;
        }
        
        double THRESHOLD_SEC = expectation + 3 * deviation;
        
       
        if (quality == 20) {
        }
        if (quality == 40) {
            THRESHOLD_SEC /= 2;
        }
        if (quality == 60) {
            THRESHOLD_SEC /= 4;
        }
        if (quality == 80) {
            THRESHOLD_SEC /= 8;
        }
        if (quality == 100) {
            THRESHOLD_SEC /= 4;
            
        }
        

        for (size_t i = 0; i < num_blocks_vert; ++i) {
            for (size_t j = 0; j < num_blocks_hor; ++j) {
                MV best_vector = mvectors.get_mv(j,i);
     
                if( best_vector.error > THRESHOLD_SEC) {
                
                    best_vector.Split();
                    
                    for (int h = 0; h < 4; ++h) {
                        auto& subvector = best_vector.SubVector(h);
                        subvector.error = std::numeric_limits<long>::max();
                        subvector.x = best_vector.x;
                        subvector.y = best_vector.y;
                        subvector.shift_dir = best_vector.shift_dir;
                        auto hor_offset = j * BLOCK_SIZE + ((h & 1) ? BLOCK_SIZE / 2 : 0);
                        auto vert_offset = first_row_offset + (i * BLOCK_SIZE + ((h > 1) ? BLOCK_SIZE / 2 : 0)) * width_ext;
                        auto cur = cur_Y + vert_offset + hor_offset;

                        int max_steps = 40;
                        int count_steps = 0;
                        int min_index = 0;
                        
                        bool flag = true;
    
                        while ((count_steps<max_steps) && flag) {
                            const auto prev = prev_map[best_vector.shift_dir] + vert_offset + hor_offset;
                            const auto prev_new = prev + subvector.y * int(width_ext) + subvector.x;
                            flag = false;
                            MV gogol;
                            gogol.x = 0;
                            gogol.y = 0;
                            gogol.error = subvector.error;
                            for (int k = 0;k < diamond_vector_size; k++) {
                                const auto comp = prev_new +  diamond_vector[k].y * int(width_ext) +  diamond_vector[k].x;
                                const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE / 2);
                                if (error < gogol.error) {
                                    gogol.x = diamond_vector[k].x;
                                    gogol.y = diamond_vector[k].y;
                                    gogol.shift_dir = best_vector.shift_dir;
                                    gogol.error = error;
                                    flag = true;
                                }
                            }
                            subvector.x += gogol.x;
                            subvector.y += gogol.y;
                            
                            subvector.error = gogol.error;
                            
                            count_steps ++;
                        }
                        for (const auto& prev_pair : prev_map) {
                            const auto prev = prev_pair.second + vert_offset + hor_offset;
                            const auto comp = prev + subvector.y * width_ext + subvector.x;
                            const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE / 2);

                            if (error < subvector.error) {
                                subvector.shift_dir = prev_pair.first;
                                subvector.error = error;
                            }
                        }
                        
                        if (subvector.error > THRESHOLD_SEC) {
                            subvector.Split();
                            
                            for (int p = 0; p < 4; ++ p) {
                                auto& subvector_t = subvector.SubVector(p);
                                subvector_t.error = std::numeric_limits<long>::max();
                                subvector_t.x = subvector.x;
                                subvector_t.y = subvector.y;
                                subvector_t.shift_dir = subvector.shift_dir;
                                hor_offset = j * BLOCK_SIZE + ((h & 1) ? BLOCK_SIZE / 2 : 0)+((p & 1) ? BLOCK_SIZE / 4 : 0);
                                vert_offset = first_row_offset + (i * BLOCK_SIZE + ((h > 1) ? BLOCK_SIZE / 2: 0)) * width_ext+((p > 1) ? BLOCK_SIZE / 4 : 0) * width_ext;
                                cur = cur_Y + vert_offset + hor_offset;
                                
                                max_steps = 40;
                                count_steps = 0;
                                min_index = 0;
                                    
                                flag=true;
                                while ((count_steps<max_steps) && flag) {
                                    const auto prev = prev_map[subvector.shift_dir] + vert_offset + hor_offset;
                                    const auto prev_new = prev + subvector_t.y * int(width_ext) + subvector_t.x;
                                    flag = false;
                                    MV gogol;
                                    gogol.error = subvector_t.error;
                                    for (int k = 0; k < diamond_vector_size; k++) {
                                        const auto comp = prev_new + diamond_vector[k].y * int(width_ext) +  diamond_vector[k].x;
                                        const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE / 4);
                                        if (error < gogol.error) {
                                            gogol.x = diamond_vector[k].x;
                                            gogol.y = diamond_vector[k].y;
                                            gogol.shift_dir = subvector.shift_dir;
                                            gogol.error = error;
                                            flag = true;
                                        }
                                    }
                                    subvector_t.x += gogol.x;
                                    subvector_t.y += gogol.y;
                                    subvector_t.error = gogol.error;
                                    count_steps ++;
                                }
                                    
                                
                                
                                for (const auto& prev_pair : prev_map) {
                                    const auto prev = prev_pair.second + vert_offset + hor_offset;
                                            const auto comp = prev + subvector_t.y * width_ext + subvector_t.x;
                                            const int error = GetErrorSAD(cur, comp, width_ext, BLOCK_SIZE / 4);
                                            if (error < subvector_t.error) {
                                                subvector.shift_dir = prev_pair.first;
                                                subvector_t.error = error;
                                            }
                                }
                            }
                              
                    }
                    
                    }

                    if (best_vector.SubVector(0).error
                        + best_vector.SubVector(1).error
                        + best_vector.SubVector(2).error
                        + best_vector.SubVector(3).error > best_vector.error * 0.7
                    )
                    mvectors.set_mv(j, i, best_vector);
                }
                int del_x = best_vector.x / BLOCK_SIZE;
                int del_y = best_vector.y / BLOCK_SIZE;
                if (i >= 2 && j < num_blocks_hor - 1 && j >= 1) {
                   vector_candidates[j + 1 + num_blocks_hor * (i - 2)].push_back(best_vector);
                   vector_candidates[j - 1 + num_blocks_hor * (i - 2)].push_back(best_vector);
                }
                int tempok_x = j + del_x;
                int tempok_y = i + del_y;
                if (tempok_x >= 0 && tempok_x < num_blocks_hor && tempok_y >= 0 && tempok_y < num_blocks_vert) {
                    vector_candidates[tempok_x + tempok_y * num_blocks_hor].push_back(best_vector);
                }
            }
        }
    }
}



void generate_subpixel_arrays(
    unsigned char* input,
    unsigned char* output_up,
    unsigned char* output_left,
    unsigned char* output_up_left,
    size_t height,
    size_t width
) {
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t cur_pixel_pos = y * width + x;
            size_t left_pixel_pos = y * width + x - 1;
            size_t left_up_pixel_pos = (y - 1) * width + x - 1;
            size_t up_pixel_pos = (y - 1) * width + x;
            
            if (y > 0) {
                output_up[cur_pixel_pos] = (int(input[cur_pixel_pos]) + input[up_pixel_pos]) / 2;
            } else {
                output_up[cur_pixel_pos] = input[cur_pixel_pos];
            }
            if (x > 0) {
                output_left[cur_pixel_pos] = (int(input[cur_pixel_pos]) + input[left_pixel_pos]) / 2;
            } else {
                output_left[cur_pixel_pos] = input[cur_pixel_pos];
            }

            if (x > 0 && y > 0) {
                output_up_left[cur_pixel_pos] = (
                        int(input[cur_pixel_pos]) +
                        input[left_pixel_pos] +
                        input[left_up_pixel_pos] +
                        input[up_pixel_pos]
                ) / 4;
            } else if (y == 0) {
                output_up_left[cur_pixel_pos] = output_left[cur_pixel_pos];
            } else {
                output_up_left[cur_pixel_pos] = output_up[cur_pixel_pos];
            }
        }
    }
}

MEField MotionEstimator::Estimate(
    py::array_t<unsigned char> cur_Y,
    py::array_t<unsigned char> prev_Y
) {
    
    extend_with_borders((unsigned char *)cur_Y.request().ptr, cur_Y_borders, height, width, BORDER);
    extend_with_borders((unsigned char *)prev_Y.request().ptr, prev_Y_borders, height, width, BORDER);
    
    if (cur_Y.size() != prev_Y.size()){
        throw std::runtime_error("Input shapes must match");
    }
    
    if (use_half_pixel) {
        generate_subpixel_arrays(
            prev_Y_borders,
            prev_Y_up_borders,
            prev_Y_left_borders,
            prev_Y_up_left_borders,
            width_borders,
            height_borders
        );
    }

    MotionEstimator::CEstimate(
        cur_Y_borders,
        prev_Y_borders,
        prev_Y_up_borders,
        prev_Y_left_borders,
        prev_Y_up_left_borders,
        me_field
    );
    MotionEstimator::first_time=false;
  
    return me_field;
    
}

