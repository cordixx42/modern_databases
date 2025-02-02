#include "moderndbs/buffer_manager.h"
#include "moderndbs/file.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace moderndbs {

char* BufferFrame::get_data() {
   return data;
}

BufferManager::BufferManager(size_t page_size, size_t page_count) {
   page_id_to_buf_frame.reserve(page_count);
   memory.reserve(page_count * page_size);
   fifoQueue.reserve(page_count * sizeof(uint64_t));
   // lruQueue.reserve(page_count * sizeof(uint64_t));
   page_c = page_count;
   page_s = page_size;
   map_size = 0;
}

BufferManager::~BufferManager() {
   BufferFrame* unfixedFrame;

   for (auto it = fifoQueue.begin(); it != fifoQueue.end(); it++) {
      // assert(page_id_to_buf_frame.contains(*it));
      unfixedFrame = page_id_to_buf_frame.at(*it);
      if (unfixedFrame->state == PageState::DIRTY) {
         File* file = segment_id_to_file.at(get_segment_id(unfixedFrame->page_id)).get();
         uint64_t indexInFile = get_segment_page_id(unfixedFrame->page_id) * page_s;
         file->write_block(unfixedFrame->data, indexInFile, page_s);
      }
      // std::cout << "freeing page id " << page_id_to_buf_frame.at(*it)->page_id << "\n";
      delete page_id_to_buf_frame.at(*it);
   }

   for (auto it = lruQueue.begin(); it != lruQueue.end(); it++) {
      // assert(page_id_to_buf_frame.contains(*it));
      unfixedFrame = page_id_to_buf_frame.at(*it);
      if (unfixedFrame->state == PageState::DIRTY) {
         File* file = segment_id_to_file.at(get_segment_id(unfixedFrame->page_id)).get();
         uint64_t indexInFile = get_segment_page_id(unfixedFrame->page_id) * page_s;
         file->write_block(unfixedFrame->data, indexInFile, page_s);
      }
      // std::cout << "freeing page id " << page_id_to_buf_frame.at(*it)->page_id << "\n";
      delete page_id_to_buf_frame.at(*it);
   }
}

BufferFrame& BufferManager::fix_page(uint64_t page_id, bool exclusive) {
again:
   // already in memory
   map_mutex.lock();
   // std::cout << "fixing page " << page_id << "\n";

   if (page_id_to_buf_frame.contains(page_id)) {
      map_mutex.unlock();

      // assert(page_id_to_buf_frame.contains(page_id));
      if (!page_id_to_buf_frame.contains(page_id)) {
         goto again;
      }
      // assert(page_id_to_buf_frame.contains(page_id));
      while (page_id_to_buf_frame.at(page_id) == nullptr) {
         std::this_thread::sleep_for(std::chrono::milliseconds(50));
         if (!page_id_to_buf_frame.contains(page_id)) {
            goto again;
         }
         // assert(page_id_to_buf_frame.contains(page_id));
      }

      // assert(page_id_to_buf_frame.contains(page_id));
      BufferFrame& frame = *(page_id_to_buf_frame.at(page_id));

      if (exclusive) {
         frame.num_waiting++;
         frame.mutex_.lock();
         frame.num_waiting--;
         frame.is_exclusive = true;
         frame.num_fixed = 0;
      } else {
         frame.num_waiting++;
         frame.mutex_.lock_shared();
         frame.num_waiting--;
         frame.is_exclusive = false;
         frame.num_fixed++;
      }

      // if in fifo queue
      // fifo_mutex.lock();
      map_mutex.lock();
      if (frame.in_fifo) {
         for (auto it = fifoQueue.begin(); it < fifoQueue.end(); it++) {
            if (*it == page_id) {
               lruQueue.push_front(page_id);
               fifoQueue.erase(it);
               frame.in_fifo = false;
               break;
            }
         }
      }
      map_mutex.unlock();

      // else in lru, we don't do anything
      frame.fixed = true;

      return frame;
   }

   // load from disk

   if (map_size == page_c) {
      // std::cout << "loading by evicting" << "\n";
      page_id_to_buf_frame.insert(std::make_pair(page_id, nullptr));
      map_mutex.unlock();

      BufferFrame* to_evict;

      // fifo_mutex.lock();
      map_mutex.lock();
      auto it = fifoQueue.begin();
      // try to evict page
      for (; it != fifoQueue.end(); it++) {
         // assert(page_id_to_buf_frame.contains(*it));
         to_evict = page_id_to_buf_frame.at(*it);
         if (!to_evict->fixed && to_evict->num_waiting == 0) {
            break;
         }
      }

      // lru_mutex.lock();
      auto itt = lruQueue.begin();
      // not found in fifo queue
      if (it == fifoQueue.end()) {
         for (; itt != lruQueue.end(); itt++) {
            // assert(page_id_to_buf_frame.contains(*itt));
            to_evict = page_id_to_buf_frame.at(*itt);
            if (!to_evict->fixed && to_evict->num_waiting == 0) {
               break;
            }
         }

         // if did not find unfixed page in either fifo or lru
         // must not evict fixed pages
         if (itt == lruQueue.end()) {
            //TODO unlock everything?
            // lru_mutex.unlock();
            // fifo_mutex.unlock();
            page_id_to_buf_frame.erase(page_id);
            map_mutex.unlock();
            throw buffer_full_error{};
         }
         // lru_mutex.lock();
         if (to_evict)
            lruQueue.erase(itt);
         // lru_mutex.unlock();
      } else {
         // fifo_mutex.lock();
         fifoQueue.erase(it);
         // fifo_mutex.unlock();
      }

      map_mutex.unlock();

      // lru_mutex.unlock();
      // fifo_mutex.unlock();

      if (to_evict->fixed || to_evict->num_waiting > 0) {
         goto again;
      }
      // assert(!to_evict->fixed);

      to_evict->mutex_.lock();

      // write back to disk if dirty
      if (to_evict->state == PageState::DIRTY) {
         std::string fileName = std::to_string(get_segment_id(to_evict->page_id));

         // has to exist

         file_mutex.lock();
         File* file = segment_id_to_file.at(get_segment_id(to_evict->page_id)).get();
         file_mutex.unlock();

         uint64_t indexInFile = get_segment_page_id(to_evict->page_id) * page_s;

         uint64_t value = *reinterpret_cast<uint64_t*>(to_evict->data);

         file->write_block(to_evict->data, indexInFile, page_s);

         // std::cout << "written dirty block for page id " << to_evict->page_id << " back to file " << fileName << " at index " << indexInFile << " with value " << value << "\n";
      }

      to_evict->mutex_.unlock();

      // lock map
      map_mutex.lock();
      page_id_to_buf_frame.erase(to_evict->page_id);
      page_id_to_buf_frame[page_id] = to_evict;
      map_mutex.unlock();
      // unlock map

      if (exclusive) {
         to_evict->num_waiting++;
         to_evict->mutex_.lock();
         to_evict->num_waiting--;
         to_evict->is_exclusive = true;
         to_evict->num_fixed = 0;
      } else {
         to_evict->num_waiting++;
         to_evict->mutex_.lock_shared();
         to_evict->num_waiting--;
         to_evict->is_exclusive = false;
         to_evict->num_fixed = 1;
      }

      to_evict->page_id = page_id;
      to_evict->fixed = true;

      uint16_t seg_id = get_segment_id(page_id);
      std::string fileName = std::to_string(seg_id);
      File* file;

      // locks

      file_mutex.lock();
      if (segment_id_to_file.contains(seg_id)) {
         file = segment_id_to_file.at(seg_id).get();
      } else {
         segment_id_to_file.emplace(seg_id, std::make_unique<PosixFile>(fileName.c_str(), File::WRITE));
         file = segment_id_to_file.at(seg_id).get();
         file->resize(page_c * page_s);
      }

      uint64_t indexInFile = get_segment_page_id(page_id) * page_s;
      uint64_t endIndex = indexInFile + page_s;

      if (file->size() < indexInFile) {
         file->resize(endIndex);
      }
      file_mutex.unlock();

      file->read_block(indexInFile, page_s, (char*) &memory[to_evict->memoryIdx]);

      // uint64_t value = *reinterpret_cast<uint64_t*>((char*) &memory[to_evict->memoryIdx]);

      // std::cout << "reading block for page id " << page_id << " from file " << fileName << " at index " << indexInFile << " with value " << value << "\n";

      to_evict->state = PageState::CLEAN;

      map_mutex.lock();
      fifoQueue.push_back(page_id);
      to_evict->in_fifo = true;
      map_mutex.unlock();

      return *to_evict;
   } else {
      map_size++;
      page_id_to_buf_frame.insert(std::make_pair(page_id, nullptr));
      map_mutex.unlock();

      // load from File

      uint16_t seg_id = get_segment_id(page_id);
      std::string fileName = std::to_string(seg_id);
      File* file;

      //slow
      BufferFrame* newFrame = new BufferFrame();
      newFrame->num_waiting = 0;

      if (exclusive) {
         newFrame->num_waiting++;
         newFrame->mutex_.lock();
         newFrame->num_waiting--;
         newFrame->is_exclusive = true;
         newFrame->num_fixed = 0;
      } else {
         newFrame->num_waiting++;
         newFrame->mutex_.lock_shared();
         newFrame->num_waiting--;
         newFrame->is_exclusive = false;
         newFrame->num_fixed = 1;
      }

      newFrame->page_id = page_id;

      file_mutex.lock();
      if (segment_id_to_file.contains(seg_id)) {
         file = segment_id_to_file.at(seg_id).get();
      } else {
         segment_id_to_file.emplace(seg_id, std::make_unique<PosixFile>(fileName.c_str(), File::WRITE));
         file = segment_id_to_file.at(seg_id).get();
         file->resize(page_c * page_s);
      }
      uint64_t indexInFile = get_segment_page_id(page_id) * page_s;
      uint64_t endIndex = indexInFile + page_s;

      if (file->size() < indexInFile) {
         file->resize(endIndex);
      }
      file_mutex.unlock();

      // save this ??

      size_t memoryIdx = memory.size();

      newFrame->memoryIdx = memoryIdx;

      memory.resize(memory.size() + page_s);

      // read into memory
      file->read_block(indexInFile, page_s, (char*) &memory[memoryIdx]);

      uint64_t value = *reinterpret_cast<uint64_t*>((char*) &memory[memoryIdx]);

      // std::cout << "reading block for page id " << page_id << " from file " << fileName << " at index " << indexInFile << " with value " << value << "\n";

      newFrame->data = (char*) &memory[memoryIdx];

      map_mutex.lock();
      page_id_to_buf_frame[page_id] = newFrame;
      // std::cout << "inserted new page " << page_id << "\n";
      fifoQueue.push_back(page_id);
      newFrame->in_fifo = true;
      map_mutex.unlock();

      newFrame->fixed = true;
      newFrame->state = PageState::NEW;

      // mutex_fix.unlock();

      return *newFrame;
   }
}

void BufferManager::unfix_page(BufferFrame& page, bool is_dirty) {
   // std::cout << "unfixing page " << page.page_id << "\n";

   if (is_dirty) {
      page.state = PageState::DIRTY;
   }

   // put to end of lru
   if (!page.in_fifo) {
      map_mutex.lock();
      for (auto it = lruQueue.begin(); it < lruQueue.end(); it++) {
         if (*it == page.page_id) {
            lruQueue.erase(it);
            lruQueue.push_back(page.page_id);
            break;
         }
      }
      map_mutex.unlock();
   }

   if (page.is_exclusive) {
      page.fixed = false;
      page.mutex_.unlock();
   } else {
      uint32_t current_fixed = --page.num_fixed;
      if (current_fixed == 0) {
         page.fixed = false;
      }
      page.mutex_.unlock_shared();
   }
}

std::vector<uint64_t> BufferManager::get_fifo_list() const {
   return fifoQueue;
}

std::vector<uint64_t> BufferManager::get_lru_list() const {
   return std::vector<uint64_t>{lruQueue.begin(), lruQueue.end()};
}

} // namespace moderndbs
