#include "moderndbs/segment.h"
#include <math.h>

using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, schema::Table& table)
   : Segment(segment_id, buffer_manager), table(table) {
}

uint8_t FSISegment::encode_free_space(uint32_t free_space) {
   // linear encoding with 4 bits
   return (free_space / (buffer_manager.get_page_size() / 16));
}

uint32_t FSISegment::decode_free_space(uint8_t free_space) {
   uint32_t fs = free_space;
   return (fs * (buffer_manager.get_page_size() / 16));
}

void printUpperNibble(uint8_t val) {
   std::cout << "upper nibble " << unsigned(val >> 4) << "\n";
}

void printLowerNibble(uint8_t val) {
   std::cout << "lower nibble " << unsigned(val & 0x0F) << "\n";
}

uint8_t getUpperNibble(uint8_t val) {
   return (val >> 4);
}

uint8_t getLowerNibble(uint8_t val) {
   return (val & 0x0F);
}

void FSISegment::update(uint64_t target_page, uint32_t free_space) {
   auto page_size = buffer_manager.get_page_size();

   uint64_t target = target_page & 0x0000FFFFFFFFFFFF;
   uint64_t seg_id = segment_id;
   uint32_t i = target / (2 * page_size);
   uint32_t j = target % (2 * page_size);
   auto& frame = buffer_manager.fix_page(i ^ (seg_id << 48), true);
   uint8_t* byte_ptr = (uint8_t*) frame.get_data();
   uint8_t nibble_2 = byte_ptr[j / 2];
   uint8_t newfs = encode_free_space(free_space);

   if (j % 2 == 0) {
      byte_ptr[j / 2] = ((getLowerNibble(newfs) << 4) | getLowerNibble(nibble_2));
   } else {
      byte_ptr[j / 2] = ((getUpperNibble(nibble_2) << 4) | getLowerNibble(newfs));
   }

   buffer_manager.unfix_page(frame, true);
}

std::optional<uint64_t> FSISegment::find(uint32_t required_space) {
   uint32_t i = 0;
   uint32_t j = 0;
   uint64_t seg_id = segment_id;
   auto page_size = buffer_manager.get_page_size();
   while ((i * 2 * page_size + j) < table.allocated_pages) {
      auto& frame = buffer_manager.fix_page(i ^ (seg_id << 48), false);
      uint8_t* byte_ptr = (uint8_t*) frame.get_data();
      for (j = 0; j < buffer_manager.get_page_size(); j++) {
         assert(j <= buffer_manager.get_page_size());
         uint8_t nibble_2 = byte_ptr[j / 2];


         uint32_t fs = 0;

         if (j % 2 == 0) {
            fs = decode_free_space(getUpperNibble(nibble_2));
         } else {
            fs = decode_free_space(getLowerNibble(nibble_2));
         }

         if (fs > (required_space + sizeof(SlottedPage::Slot))) {
            buffer_manager.unfix_page(frame, false);
            return std::make_optional(i * 2 * page_size + j);
         }
      }
      i++;
      buffer_manager.unfix_page(frame, false);
   }

   return std::nullopt;
}
