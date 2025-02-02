#ifndef INCLUDE_MODERNDBS_BTREE_H
#define INCLUDE_MODERNDBS_BTREE_H

#include "moderndbs/buffer_manager.h"
#include "moderndbs/segment.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>
#include <optional>
#include <shared_mutex>

namespace moderndbs {

template <typename KeyT, typename ValueT, typename ComparatorT, size_t PageSize>
struct BTree : public Segment {
   struct Node {
      /// The level in the tree.
      uint16_t level;
      /// The number of children.
      uint16_t count;

      // Constructor
      Node(uint16_t level, uint16_t count)
         : level(level), count(count) {}

      /// Is the node a leaf node?
      bool is_leaf() const { return level == 0; }
   };

   struct InnerNode : public Node {
      /// The capacity of a node.
      /// TODO think about the capacity that the nodes have.
      static constexpr uint32_t kCapacity = (((uint64_t) PageSize - sizeof(Node)) - sizeof(uint64_t)) / (sizeof(KeyT) + sizeof(uint64_t));

      /// The keys.
      KeyT keys[kCapacity]; // TODO adjust this
      /// The values.
      uint64_t children[kCapacity + 1]; // TODO adjust this

      /// Constructor.
      InnerNode() : Node(0, 0) {}

      // constructor with level
      // count is number of children, number of keys + 1

      InnerNode(uint16_t level) : Node(level, 1) {}

      /// Get the index of the first key that is not less than than a provided key.
      /// @param[in] key          The key that should be inserted.
      std::pair<uint32_t, bool> lower_bound(const KeyT& key) {
         if (0 == this->count - 1) {
            return std::make_pair(0, keys[0] == key);
         }
         uint32_t idx = binarySearch(key, 0, this->count - 1);
         if (keys[idx] == key) {
            return std::make_pair(idx, true);
         } else {
            return std::make_pair(idx, false);
         }
      }

      /// Insert a key.
      /// @param[in] key          The key that should be inserted.
      /// @param[in] split_page   The child that should be inserted.
      void insert_split(const KeyT& key, uint64_t split_page) {
         assert(this->count < (kCapacity + 1));

         std::pair<uint32_t, bool> lb = lower_bound(key);

         // overwrite case does not happen
         if (lb.second && lb.first < (this->count - 1)) {
            children[lb.first] = split_page;
         }
         // new insertion case
         else {
            std::memmove(&keys[lb.first + 1], &keys[lb.first], sizeof(KeyT) * ((this->count - 1) - lb.first));
            std::memmove(&children[lb.first + 2], &children[lb.first + 1], sizeof(uint64_t) * ((this->count - 1) - lb.first));
            keys[lb.first] = key;
            children[lb.first + 1] = split_page;
            this->count += 1;
         }
      }

      /// Split the node.
      /// @param[in] buffer       The buffer for the new page.
      /// @return                 The separator key.
      KeyT split(std::byte* buffer) {
         InnerNode* right = reinterpret_cast<InnerNode*>(buffer);

         int32_t splitIdx = this->count / 2;
         KeyT splitKey = keys[splitIdx];

         std::memmove(&right->keys[0], &keys[splitIdx + 1], sizeof(KeyT) * (this->count - splitIdx - 2));
         std::memmove(&right->children[0], &children[splitIdx + 1], sizeof(uint64_t) * (this->count - splitIdx - 1));

         right->count = (this->count - splitIdx - 1);
         this->count = splitIdx + 1;
         return splitKey;
      }

      /// Returns the keys.
      std::vector<KeyT> get_key_vector() {
         return std::vector<KeyT>(std::begin(keys), std::begin(keys) + (this->count - 1));
      }

      // TODO: branchless
      // uint32_t binarySearch(const KeyT& key, uint32_t lower, uint32_t upper) const {
      //    if (lower == upper) {
      //       return lower;
      //    }
      //    do {
      //       uint32_t mid = ((upper - lower) / 2) + lower;
      //       if (key < keys[mid]) {
      //          upper = mid;
      //       } else if (key > keys[mid]) {
      //          lower = mid + 1;
      //       } else {
      //          return mid;
      //       }
      //    } while (lower < upper);
      //    return lower;
      // }

      uint32_t binarySearch(const KeyT& key, uint32_t lower, uint32_t upper) const {
         // if (lower == upper) {
         //    return lower;
         // }
         uint32_t n = upper;
         while (n > 1) {
            uint32_t half = n / 2;
            lower = (keys[lower + half] < key) ? (lower + half) : lower;
            n -= half;
         }
         return (keys[lower] < key) + lower;
      }
   };

   struct LeafNode : public Node {
      /// The capacity of a node.
      static constexpr uint32_t kCapacity = (((uint64_t) PageSize - sizeof(Node)) / (sizeof(KeyT) + sizeof(ValueT)));

      /// The keys.
      KeyT keys[kCapacity]; // adjust this
      /// The values.
      ValueT values[kCapacity]; // adjust this

      /// Constructor.
      LeafNode() : Node(0, 0) {}

      /// Get the index of the first key that is not less than than a provided key.
      std::pair<uint32_t, bool> lower_bound(const KeyT& key) {
         if (0 == this->count) {
            return std::make_pair(0, keys[0] == key);
         }
         uint32_t idx = binarySearch(key, 0, this->count);
         if (keys[idx] == key) {
            return std::make_pair(idx, true);
         } else {
            return std::make_pair(idx, false);
         }
      }

      /// Insert a key.
      /// @param[in] key          The key that should be inserted.
      /// @param[in] value        The value that should be inserted.
      void insert(const KeyT& key, const ValueT& value) {
         // TODO
         assert(this->count < kCapacity);
         std::pair<uint32_t, bool> lb = lower_bound(key);

         // overwrite case
         if (lb.second && lb.first < this->count) {
            values[lb.first] = value;
         }
         // new insertion case
         else {
            // insert last
            // if (lb.first == this->count) {
            //    keys[this->count] = key;
            //    values[this->count] = value;
            // }
            // // insert somewhere
            // else {
            std::memmove(&keys[lb.first + 1], &keys[lb.first], sizeof(KeyT) * (this->count - lb.first));
            std::memmove(&values[lb.first + 1], &values[lb.first], sizeof(ValueT) * (this->count - lb.first));
            keys[lb.first] = key;
            values[lb.first] = value;
            // }
            this->count += 1;
         }
      }

      /// Erase a key.
      void erase(const KeyT& key) {
         std::pair<uint32_t, bool> lb = lower_bound(key);
         assert(lb.second);

         // delete last
         if (lb.first == this->count) {
            this->count -= 1;
         }
         // delete in middle
         else {
            std::memmove(&keys[lb.first], &keys[lb.first + 1], sizeof(KeyT) * (this->count - lb.first - 1));
            std::memmove(&values[lb.first], &values[lb.first + 1], sizeof(ValueT) * (this->count - lb.first - 1));
            this->count -= 1;
         }
      }

      /// Split the node.
      /// @param[in] buffer       The buffer for the new page.
      /// @return                 The separator key.
      KeyT split(std::byte* buffer) {
         LeafNode* right = reinterpret_cast<LeafNode*>(buffer);
         //on the right side count/2 keys, count/2 values
         // on the left side count - count/2 keys, count - count/2 values

         int32_t splitIdx = this->count / 2;
         KeyT splitKey = keys[splitIdx];

         std::memmove(&right->keys[0], &keys[splitIdx + 1], sizeof(KeyT) * (this->count - (this->count / 2) - 1));
         std::memmove(&right->values[0], &values[splitIdx + 1], sizeof(uint64_t) * (this->count - (this->count / 2) - 1));

         right->count = (this->count - this->count / 2 - 1);
         this->count = this->count / 2 + 1;
         return splitKey;
      }

      /// Returns the keys.
      std::vector<KeyT> get_key_vector() {
         return std::vector<KeyT>(std::begin(keys), std::begin(keys) + this->count);
      }

      /// Returns the values.
      std::vector<ValueT> get_value_vector() {
         return std::vector<ValueT>(std::begin(values), std::begin(values) + this->count);
      }

      // uint32_t binarySearch(const KeyT& key, uint32_t lower, uint32_t upper) const {
      //    if (lower == upper) {
      //       return lower;
      //    }
      //    do {
      //       uint32_t mid = ((upper - lower) / 2) + lower;
      //       if (key < keys[mid]) {
      //          upper = mid;
      //       } else if (key > keys[mid]) {
      //          lower = mid + 1;
      //       } else {
      //          return mid;
      //       }
      //    } while (lower < upper);
      //    return lower;
      // }

      uint32_t binarySearch(const KeyT& key, uint32_t lower, uint32_t upper) const {
         uint32_t n = upper;
         while (n > 1) {
            uint32_t half = n / 2;
            lower = (keys[lower + half] < key) ? (lower + half) : lower;
            n -= half;
         }
         return (keys[lower] < key) + lower;
      }

      bool containsKey(const KeyT& key) {
         auto& lb = lower_bound(key);
         return lb.second;
      }
   };

   /// The root.
   uint64_t root; // TODO ensure thread safety of the root node
   // when overwriting this value

   // std::mutex root_mutex;

   std::mutex root_mutex;

   //TODO: try this out
   std::atomic<uint32_t> mutex;

   // use atomic for thread safety
   std::atomic<uint32_t> page_counter = 0;

   /// Constructor.
   BTree(uint16_t segment_id, BufferManager& buffer_manager)
      : Segment(segment_id, buffer_manager) {
      //persist btree in buffer manager maybee
      uint64_t seg_id = segment_id;
      // root_mutex.lock();
      auto& frame = buffer_manager.fix_page((seg_id << 48) ^ page_counter, true);
      root = page_counter++;
      // root_mutex.unlock();
      new (frame.get_data()) LeafNode{};
      buffer_manager.unfix_page(frame, true);
   }

   /// Destructor.
   ~BTree() = default;

   /// Lookup an entry in the tree.
   /// @param[in] key      The key that should be searched.
   /// @return             Whether the key was in the tree.
   std::optional<ValueT> lookup(const KeyT& key) {
      uint64_t seg_id = segment_id;

      root_mutex.lock();
      BufferFrame* current_frame = &buffer_manager.fix_page((seg_id << 48) ^ root, false);

      Node* current_node = reinterpret_cast<Node*>(current_frame->get_data());

      BufferFrame* parent_frame = nullptr;

      while (!current_node->is_leaf()) {
         InnerNode* current_inner = static_cast<InnerNode*>(current_node);
         std::pair<uint32_t, bool> lb = current_inner->lower_bound(key);

         uint64_t next_pid = current_inner->children[lb.first];

         BufferFrame* parent_before = parent_frame;

         parent_frame = current_frame;

         current_frame = &buffer_manager.fix_page((seg_id << 48) ^ next_pid, false);
         current_node = reinterpret_cast<Node*>(current_frame->get_data());

         if (parent_before == nullptr) {
            root_mutex.unlock();
         }

         if (parent_before != nullptr) {
            buffer_manager.unfix_page(*parent_before, false);
         }
      }

      LeafNode* current_leaf = static_cast<LeafNode*>(current_node);
      std::pair<uint32_t, bool> lb = current_leaf->lower_bound(key);
      if (!lb.second || lb.first >= current_leaf->count) {
         if (parent_frame != nullptr) {
            buffer_manager.unfix_page(*parent_frame, false);
         }

         if (parent_frame == nullptr) {
            root_mutex.unlock();
         }

         buffer_manager.unfix_page(*current_frame, true);

         return std::nullopt;
      } else {
         if (parent_frame != nullptr) {
            buffer_manager.unfix_page(*parent_frame, false);
         }

         if (parent_frame == nullptr) {
            root_mutex.unlock();
         }

         buffer_manager.unfix_page(*current_frame, true);

         return std::make_optional(current_leaf->values[lb.first]);
      }
   }

   /// Erase an entry in the tree.
   /// @param[in] key      The key that should be searched.
   void erase(const KeyT& key) {
      // (void) key;
      // // TODO

      uint64_t seg_id = segment_id;

      root_mutex.lock();
      BufferFrame* current_frame = &buffer_manager.fix_page((seg_id << 48) ^ root, false);

      Node* current_node = reinterpret_cast<Node*>(current_frame->get_data());

      BufferFrame* parent_frame = nullptr;

      while (!current_node->is_leaf()) {
         InnerNode* current_inner = static_cast<InnerNode*>(current_node);
         std::pair<uint32_t, bool> lb = current_inner->lower_bound(key);

         uint64_t next_pid = current_inner->children[lb.first];

         BufferFrame* parent_before = parent_frame;

         parent_frame = current_frame;

         current_frame = &buffer_manager.fix_page((seg_id << 48) ^ next_pid, false);
         current_node = reinterpret_cast<Node*>(current_frame->get_data());

         if (parent_before == nullptr) {
            root_mutex.unlock();
         }

         if (parent_before != nullptr) {
            buffer_manager.unfix_page(*parent_before, false);
         }
      }

      LeafNode* current_leaf = static_cast<LeafNode*>(current_node);
      std::pair<uint32_t, bool> lb = current_leaf->lower_bound(key);
      if (lb.second && lb.first < current_leaf->count) {
         current_leaf->erase(key);
      }

      if (parent_frame != nullptr) {
         buffer_manager.unfix_page(*parent_frame, false);
      }

      if (parent_frame == nullptr) {
         root_mutex.unlock();
      }

      buffer_manager.unfix_page(*current_frame, true);
   }

   /// Inserts a new entry into the tree.
   /// @param[in] key      The key that should be inserted.
   /// @param[in] value    The value that should be inserted.
   void insert(const KeyT& key, const ValueT& value) {
      uint64_t seg_id = segment_id;

      root_mutex.lock();
      BufferFrame* current_frame = &buffer_manager.fix_page((seg_id << 48) ^ root, true);
      Node* current_node = reinterpret_cast<Node*>(current_frame->get_data());
      uint64_t current_pid = root;

      BufferFrame* parent_frame = nullptr;
      Node* parent_node = nullptr;
      uint64_t parent_pid = 0;

      while (!current_node->is_leaf()) {
         InnerNode* current_inner = static_cast<InnerNode*>(current_node);

         // eager splitting inner nodes
         if (current_inner->count - 1 >= current_inner->kCapacity) {
            uint32_t right_pid = page_counter++;

            auto& right_frame = buffer_manager.fix_page((seg_id << 48) ^ right_pid, true);
            new (right_frame.get_data()) InnerNode{static_cast<uint16_t>(current_node->level)};

            KeyT splitKey = current_inner->split((std::byte*) right_frame.get_data());

            // new inner root: fix new_root and insert current_pid and right_pid
            if (parent_node == nullptr) {
               uint32_t new_root_pid = page_counter++;

               auto& new_root = buffer_manager.fix_page((seg_id << 48) ^ new_root_pid, true);
               parent_frame = &new_root;
               parent_pid = new_root_pid;
               //new root level 1
               new (new_root.get_data()) InnerNode{static_cast<uint16_t>(current_node->level + 1)};
               InnerNode* new_root_node = reinterpret_cast<InnerNode*>(new_root.get_data());
               new_root_node->children[0] = current_pid;
               new_root_node->insert_split(splitKey, right_pid);
               root = new_root_pid;

               root_mutex.unlock();

            } else {
               InnerNode* parent_inner = static_cast<InnerNode*>(parent_node);
               parent_inner->insert_split(splitKey, right_pid);
            }

            // set current_frame to the right_frame
            if (key > splitKey) {
               buffer_manager.unfix_page(*current_frame, false);
               current_frame = &right_frame;
               current_inner = reinterpret_cast<InnerNode*>(current_frame->get_data());
               current_pid = right_pid;
            } else {
               buffer_manager.unfix_page(right_frame, false);
            }
         }

         std::pair<uint32_t, bool> lb = current_inner->lower_bound(key);

         BufferFrame* parent_before = parent_frame;

         parent_node = current_node;
         parent_frame = current_frame;
         parent_pid = current_pid;

         current_pid = current_inner->children[lb.first];
         current_frame = &buffer_manager.fix_page((seg_id << 48) ^ current_pid, true);
         current_node = reinterpret_cast<Node*>(current_frame->get_data());

         if (parent_before != nullptr) {
            buffer_manager.unfix_page(*parent_before, false);
         }

         if (parent_before == nullptr) {
            root_mutex.unlock();
         }
      }

      // at this point one fixed current_frame, one potential fixed parent_frame

      assert(current_node->is_leaf());
      LeafNode* leaf_node = static_cast<LeafNode*>(current_node);

      // split leaf node
      if (leaf_node->count >= leaf_node->kCapacity) {
         uint32_t right_pid = page_counter++;

         auto& right_frame = buffer_manager.fix_page((seg_id << 48) ^ right_pid, true);
         new (right_frame.get_data()) LeafNode{};

         // split leaf
         KeyT splitKey = leaf_node->split((std::byte*) right_frame.get_data());

         // if new root needed
         if (parent_node == nullptr) {
            uint32_t new_root_pid = page_counter++;
            auto& new_root = buffer_manager.fix_page((seg_id << 48) ^ new_root_pid, true);
            parent_frame = &new_root;
            parent_pid = new_root_pid;
            //set new root level
            new (new_root.get_data()) InnerNode{static_cast<uint16_t>(current_node->level + 1)};
            InnerNode* new_root_node = reinterpret_cast<InnerNode*>(new_root.get_data());
            new_root_node->children[0] = current_pid;
            new_root_node->insert_split(splitKey, right_pid);
            root = new_root_pid;
            root_mutex.unlock();
         } else {
            InnerNode* parent_inner = static_cast<InnerNode*>(parent_node);
            parent_inner->insert_split(splitKey, right_pid);
         }

         if (key > splitKey) {
            leaf_node = reinterpret_cast<LeafNode*>(right_frame.get_data());
         }

         leaf_node->insert(key, value);

         if (parent_frame != nullptr) {
            buffer_manager.unfix_page(*parent_frame, false);
         }

         if (parent_frame == nullptr) {
            root_mutex.unlock();
         }

         buffer_manager.unfix_page(*current_frame, false);

         buffer_manager.unfix_page(right_frame, false);

      } else {
         leaf_node->insert(key, value);

         if (parent_frame != nullptr) {
            buffer_manager.unfix_page(*parent_frame, false);
         }

         if (parent_frame == nullptr) {
            root_mutex.unlock();
         }

         buffer_manager.unfix_page(*current_frame, true);
      }
   }
};

} // namespace moderndbs

#endif
