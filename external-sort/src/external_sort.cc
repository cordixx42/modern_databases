#include "moderndbs/external_sort.h"
#include "moderndbs/file.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <vector>

namespace moderndbs {

void external_sort(File& input, size_t num_values, File& output, size_t mem_size) {
   int allbytes = sizeof(uint64_t) * num_values;
   mem_size &= ~7;

   output.resize(num_values * sizeof(uint64_t));

   // int k = 2;

   int num_blocks = allbytes / mem_size;
   int rest_mem_size = allbytes % mem_size;

   std::pair<int, bool> last_block_info = std::make_pair(mem_size, 0);

   if (rest_mem_size != 0) {
      num_blocks += 1;
      last_block_info.first = rest_mem_size;
   }

   std::unique_ptr<File> allFiles[2][2];
   for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
         allFiles[i][j] = File::make_temporary_file();
      }
   }

   char* buf = (char*) malloc(mem_size);

   for (int i = 0; i < num_blocks; i++) {
      size_t bufSize = mem_size;
      if (rest_mem_size != 0 && i == num_blocks - 1) {
         bufSize = rest_mem_size;
      }
      input.read_block(i * mem_size, bufSize, buf);
      uint64_t* arr = (uint64_t*) buf;
      std::sort(arr, arr + (bufSize / 8));
      (num_values * sizeof(uint64_t) <= mem_size) ? output.write_block(buf, (i / 2) * mem_size, bufSize) : allFiles[0][i % 2]->write_block(buf, (i / 2) * mem_size, bufSize);
   }

   free(buf);

   // WORKS
   // char* buff = (char*) malloc(mem_size);
   // uint64_t* arrr = (uint64_t*) buff;
   // for (int i = 0; i < num_blocks; i++) {
   //    allFiles[0][i % 2]->read_block((i / 2) * mem_size, mem_size, buff);
   //    for (int j = 0; j < (mem_size / 8); j++) {
   //       std::cout << j << " " << arrr[j] << "\n";
   //    }
   // }

   bool current_location = false;

   int current_size = mem_size;
   int current_num_blocks = num_blocks;
   int current_rest_size = rest_mem_size;

   int smaller_size = 0;
   if (mem_size >= 32) {
      smaller_size = ((mem_size / 4) & ~7);
   } else {
      smaller_size = sizeof(uint64_t);
   }

   // std::cout << "smaller size is " << smaller_size << "\n";

   char* buf0 = (char*) malloc(smaller_size);
   char* buf1 = (char*) malloc(smaller_size);
   char* bufm = (char*) malloc(2 * smaller_size);

   uint64_t* buf0_val = (uint64_t*) buf0;
   uint64_t* buf1_val = (uint64_t*) buf1;
   uint64_t* bufm_val = (uint64_t*) bufm;

   // 0 if divisible by 2, 1 if not
   last_block_info.second = num_blocks % 2;

   int j0 = 0;
   int j1 = 0;

   int j00 = 0;
   int j11 = 0;
   int jm = 0;

   int jmm = 0;

   uint64_t val0 = 0;
   uint64_t val1 = 0;

   bool buf0_empty = false;
   bool buf1_empty = false;

   while (current_num_blocks > 1) {
      int current_full_blocks = current_num_blocks;
      if (last_block_info.first < current_size) {
         current_full_blocks -= 1;
      }
      int current_full_rows = current_full_blocks / 2;

      // std::cout << "current_full_blocks " << current_full_blocks << "\n";
      // std::cout << "current_full_rows " << current_full_rows << "\n";
      // std::cout << "lastblockfirst " << last_block_info.first << "\n";
      // std::cout << "lastblocksnd " << last_block_info.second << "\n";

      for (int i = 0; i < current_full_rows; i++) {
         allFiles[current_location][0]->read_block((i * current_size) + smaller_size * j0, smaller_size, buf0);
         allFiles[current_location][1]->read_block((i * current_size) + smaller_size * j1, smaller_size, buf1);
         val0 = buf0_val[j00];
         // std::cout << "val0 " << val0 << "\n";
         val1 = buf1_val[j11];
         // std::cout << "val1 " << val1 << "\n";
         j0++;
         j1++;
         j00++;
         j11++;
         while (!(buf0_empty && buf1_empty)) {
            // std::cout << "inner loop" << "\n";
            // buf0 smaller than buf1
            if (buf1_empty || ((val0 < val1) && !buf0_empty)) {
               bufm_val[jm] = val0;
               jm++;
               if (j00 < (smaller_size / 8)) {
                  val0 = buf0_val[j00];
                  j00++;
               } else {
                  if (j0 < (current_size / smaller_size)) {
                     allFiles[current_location][0]->read_block((i * current_size) + smaller_size * j0, smaller_size, buf0);
                     j0++;
                     val0 = buf0_val[0];
                     // std::cout << "val0 " << val0 << "\n";
                     j00 = 1;
                  } else {
                     // std::cout << "buf0_empty" << "\n";
                     buf0_empty = true;
                     j0 = 0;
                     j00 = 0;
                  }
               }
            } else if (buf0_empty || ((val1 <= val0) && !buf1_empty)) {
               bufm_val[jm] = val1;
               jm++;
               if (j11 < (smaller_size / 8)) {
                  val1 = buf1_val[j11];
                  j11++;
               } else {
                  if (j1 < (current_size / smaller_size)) {
                     allFiles[current_location][1]->read_block((i * current_size) + smaller_size * j1, smaller_size, buf1);
                     j1++;
                     j11 = 0;
                     val1 = buf1_val[j11];
                     // std::cout << "val1 " << val1 << "\n";
                     j11++;
                  } else {
                     // std::cout << "buf1_empty" << "\n";
                     buf1_empty = true;
                     j1 = 0;
                     j11 = 0;
                  }
               }
            }
            if (jm == (2 * smaller_size) / sizeof(uint64_t)) {
               // for (int j = 0; j < (2 * smaller_size) / 8; j++) {
               //    std::cout << "bufm " << bufm_val[j] << "\n";
               // }
               (current_full_blocks == 2 && !last_block_info.second) ? output.write_block(bufm, (i / 2) * 2 * current_size + jmm * 2 * smaller_size, 2 * smaller_size) : allFiles[!current_location][(i % 2)]->write_block(bufm, (i / 2) * 2 * current_size + jmm * 2 * smaller_size, 2 * smaller_size);
               jm = 0;
               jmm++;
            }
         }
         buf0_empty = false;
         buf1_empty = false;
         jmm = 0;
      }

      // handle not full last row
      if (!(last_block_info.first == current_size && !last_block_info.second)) {
         // case only one: just copy this
         // currently very bad, just piecewise copy

         if (last_block_info.second) {
            for (int j = 0; j < (last_block_info.first / sizeof(uint64_t)); j++) {
               allFiles[current_location][0]->read_block((current_full_rows * current_size) + j * sizeof(uint64_t), sizeof(uint64_t), buf0);
               // std::cout << "last " << buf0_val[0] << "\n";
               current_num_blocks <= 2 ? output.write_block(buf0, (current_full_rows / 2) * 2 * current_size + j * sizeof(uint64_t), sizeof(uint64_t)) : allFiles[!current_location][(current_full_rows % 2)]->write_block(buf0, (current_full_rows / 2) * 2 * current_size + j * sizeof(uint64_t), sizeof(uint64_t));
            }
            // first block full, second block not full
         } else {
            // adapt smaller size
            // if (last_block_info.first < smaller_size) {
            //    smaller_size = last_block_info.first;
            // }

            allFiles[current_location][0]->read_block((current_full_rows * current_size) + smaller_size * j0, smaller_size, buf0);
            allFiles[current_location][1]->read_block((current_full_rows * current_size), sizeof(uint64_t), buf1);
            val0 = buf0_val[j00];

            val1 = buf1_val[0];
            j0++;
            j1++;
            j00++;

            while (!(buf0_empty && buf1_empty)) {
               // std::cout << "inner loop" << "\n";
               // buf0 smaller than buf1
               if (buf1_empty || ((val0 < val1) && !buf0_empty)) {
                  // std::cout << "write rest val0 " << val0 << "\n";
                  current_num_blocks <= 2 ? output.write_block((char*) &val0, (current_full_rows / 2) * 2 * current_size + jm * sizeof(uint64_t), sizeof(uint64_t)) : allFiles[!current_location][(current_full_rows % 2)]->write_block((char*) &val0, (current_full_rows / 2) * 2 * current_size + jm * sizeof(uint64_t), sizeof(uint64_t));
                  jm++;
                  if (j00 < (smaller_size / 8)) {
                     val0 = buf0_val[j00];
                     j00++;
                  } else {
                     if (j0 < (current_size / smaller_size)) {
                        allFiles[current_location][0]->read_block((current_full_rows * current_size) + smaller_size * j0, smaller_size, buf0);
                        j0++;
                        j00 = 0;
                        val0 = buf0_val[j00];
                        // std::cout << "val0 " << val0 << "\n";
                        j00++;
                     } else {
                        // std::cout << "buf0_empty" << "\n";
                        buf0_empty = true;
                        j0 = 0;
                        j00 = 0;
                     }
                  }
               } else if (buf0_empty || ((val1 <= val0) && !buf1_empty)) {
                  // std::cout << "write rest val1 " << val1 << "\n";
                  current_num_blocks <= 2 ? output.write_block((char*) &val1, (current_full_rows / 2) * 2 * current_size + jm * sizeof(uint64_t), sizeof(uint64_t)) : allFiles[!current_location][(current_full_rows % 2)]->write_block((char*) &val1, (current_full_rows / 2) * 2 * current_size + jm * sizeof(uint64_t), sizeof(uint64_t));
                  jm++;
                  if (j1 == (last_block_info.first / sizeof(uint64_t))) {
                     buf1_empty = true;
                     j1 = 0;
                  }
                  allFiles[current_location][1]->read_block((current_full_rows * current_size) + sizeof(uint64_t) * j1, sizeof(uint64_t), buf1);
                  val1 = buf1_val[0];
                  j1++;
               }
            }

            j0 = 0;
            j00 = 0;
            jm = 0;
            j1 = 0;
            buf0_empty = false;
            buf1_empty = false;
         }
      }

      current_location = !current_location;

      // case merged full one with maybe partly full block
      if (!last_block_info.second) {
         last_block_info.first = last_block_info.first + current_size;
      }
      // double
      current_size = current_size * 2;

      current_num_blocks = current_full_rows;
      if (!(last_block_info.first == current_size && !last_block_info.second)) {
         current_num_blocks += 1;
      }
      // current_num_blocks = (1 << (((8 * sizeof(int)) - __builtin_clzl(current_size)) - 1));

      last_block_info.second = current_num_blocks % 2;
   }

   // output.read_block(0, (num_values * sizeof(uint64_t)), buf_test);
   // for (int i = 0; i < (num_values); i++) {
   //    std::cout << "output result " << buf_test_val[i] << "\n";
   // }

   free(buf0);
   free(buf1);
   free(bufm);
}

// void external_sort(File& input, size_t num_values, File& output, size_t mem_size) {
//    // TODO: add your implementation here

//    std::cout << "max mem is " << mem_size << "\n";

//    int allbytes = sizeof(uint64_t) * num_values;
//    //    std::cout << "num_values is " << num_values << "\n";
//    //    std::cout << "allbytes is " << num_values << "\n";

//    if (mem_size % 8 != 0) {
//       mem_size = (mem_size / 8) * 8;
//    }

//    if (mem_size == 0) {
//       //   std::cout << "too small mem_size" << "\n";
//    }

//    //    if (mem_size > 16) {
//    //       mem_size = mem_size / 2;
//    //    }

//    std::cout << "mem_size is " << mem_size << "\n";

//    int num_blocks = allbytes / mem_size;
//    int rest_block_bytes = allbytes % mem_size;

//    //    std::cout << "numblocks is " << num_blocks << "\n";
//    //    std::cout << "rest is " << rest_block_bytes << "\n";

//    std::vector<std::unique_ptr<File>> allBlocks;
//    allBlocks.reserve(mem_size + 10);

//    std::cout << "allBlocks vector size " << sizeof(std::unique_ptr<File>) * (num_blocks + 1) << "\n";

//    for (int i = 0; i < num_blocks; i++) {
//       char* buf = (char*) malloc(mem_size);
//       input.read_block(i * mem_size, mem_size, buf);

//       uint64_t* arr = (uint64_t*) buf;

//       std::sort(arr, arr + (mem_size / 8));

//       allBlocks.push_back(File::make_temporary_file());
//       allBlocks[i]->resize((mem_size / 8) * sizeof(uint64_t));
//       allBlocks[i]->write_block(buf, 0, mem_size);

//       free(buf);
//    }

//    if (rest_block_bytes != 0) {
//       char* buf = (char*) malloc(rest_block_bytes);
//       input.read_block(num_blocks * mem_size, rest_block_bytes, buf);
//       uint64_t* arr = (uint64_t*) buf;
//       std::sort(arr, arr + (rest_block_bytes / 8));
//       allBlocks.push_back(File::make_temporary_file());
//       allBlocks[num_blocks]->resize((rest_block_bytes / 8) * sizeof(uint64_t));
//       allBlocks[num_blocks]->write_block(buf, 0, rest_block_bytes);
//       free(buf);
//    }

//    //    for (int i = 0; i < allBlocks.size(); i++) {
//    //       char* buf = (char*) malloc(mem_size);
//    //       allBlocks[i]->read_block(0, mem_size, buf);
//    //       uint64_t* arr = (uint64_t*) buf;
//    //       for (int j = 0; j < (mem_size / 8); j++) {
//    //          std::cout << j << " " << arr[j] << "\n";
//    //       }
//    //       free(buf);
//    //    }

//    // first is value, second is index of allBlocks

//    //    std::vector<uint64_t> mins;
//    //    mins.reserve(allBlocks.size());

//    std::cout << "mins size " << sizeof(uint64_t) * (num_blocks + 1) << "\n";
//    //    std::vector<std::pair<uint64_t, uint32_t>> min_elems;
//    //    min_elems.reserve(allBlocks.size());

//    std::priority_queue<std::pair<uint64_t, uint32_t>, std::vector<std::pair<uint64_t, uint32_t>>, std::greater<>> mins;

//    std::vector<uint32_t> elemsLeft(allBlocks.size(), mem_size / 8);
//    if (rest_block_bytes != 0) {
//       elemsLeft[num_blocks] = rest_block_bytes / 8;
//    }

//    for (int i = 0; i < allBlocks.size(); i++) {
//       char* val = (char*) malloc(sizeof(uint64_t));
//       allBlocks[i]->read_block(0, sizeof(uint64_t), val);
//       // mins.push_back(*(uint64_t*) val);
//       // min_elems.push_back(std::make_pair(*(uint64_t*) val, (uint32_t) i));
//       mins.push(std::make_pair(*(uint64_t*) val, (uint32_t) i));
//       elemsLeft[i] -= 1;
//       free(val);
//    }

//    int offset = 0;

//    output.resize(num_values * sizeof(uint64_t));

//    while (!mins.empty()) {
//       //   std::cout << "min element " << min_elems.back().first << " from block " << min_elems.back().second << "\n";
//       output.write_block((char*) &mins.top().first, offset, sizeof(uint64_t));
//       offset += sizeof(uint64_t);
//       uint32_t nextBlock = mins.top().second;
//       mins.pop();
//       if (elemsLeft[nextBlock] == 0) {
//          continue;
//       } else {
//          char* val = (char*) malloc(sizeof(uint64_t));
//          if (nextBlock == num_blocks) {
//             allBlocks[nextBlock]->read_block(8 * ((rest_block_bytes / 8) - elemsLeft[nextBlock]), sizeof(uint64_t), val);
//          } else {
//             allBlocks[nextBlock]->read_block(8 * ((mem_size / 8) - elemsLeft[nextBlock]), sizeof(uint64_t), val);
//          }
//          //  std::cout << "hhh " << *(uint64_t*) val << "\n";
//          elemsLeft[nextBlock] -= 1;
//          mins.push(std::make_pair(*(uint64_t*) val, (uint32_t) nextBlock));
//          free(val);
//       }
//    }
// }

} // namespace moderndbs
