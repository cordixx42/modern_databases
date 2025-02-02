#include "moderndbs/slotted_page.h"
#include <cstring>

using moderndbs::SlottedPage;

SlottedPage::Header::Header(uint32_t page_size)
   : slot_count(0),
     first_free_slot(0),
     data_start(page_size),
     free_space(page_size - sizeof(Header)) {}

SlottedPage::SlottedPage(uint32_t page_size)
   : header(page_size) {
   std::memset(get_data() + sizeof(SlottedPage), 0x00, page_size - sizeof(SlottedPage));
}

std::byte* SlottedPage::get_data() {
   return reinterpret_cast<std::byte*>(this);
}

const std::byte* SlottedPage::get_data() const {
   return reinterpret_cast<const std::byte*>(this);
}

SlottedPage::Slot* SlottedPage::get_slots() {
   return reinterpret_cast<SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

const SlottedPage::Slot* SlottedPage::get_slots() const {
   return reinterpret_cast<const SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

uint32_t SlottedPage::get_fragmented_free_space() {
   // space between slots and data ?
   return header.data_start - (sizeof(Header) + header.slot_count * (sizeof(Slot)));
}

uint16_t SlottedPage::allocate(uint32_t data_size, uint32_t page_size) {
   // check if full ? then compactify

   if (data_size + sizeof(Slot) > get_free_space()) {

      throw std::invalid_argument("no space sp allocate");
   }
   if (data_size + sizeof(Slot) > get_fragmented_free_space()) {
      compactify(page_size);
   }

   // first free slot

   Slot* slots = get_slots();
   auto& ffs = slots[header.first_free_slot];
   if (header.first_free_slot != header.slot_count && ffs.get_size() >= data_size) {
         uint16_t result = header.first_free_slot;
         ffs.set_slot(ffs.get_offset(), data_size, false);
         header.free_space -= data_size;
         header.first_free_slot = header.slot_count;
         return result;
   }
   else {
         if(header.first_free_slot == header.slot_count) {
            header.first_free_slot += 1;
         }

         header.free_space -= (sizeof(Slot));
         header.slot_count += 1;
         header.data_start -= data_size;
         header.free_space -= (data_size);


         Slot newSlot = Slot();
         newSlot.set_slot(header.data_start, data_size, false);

         Slot* slots = get_slots();
         slots[header.slot_count - 1] = newSlot;

         // index as slot_id
         return header.slot_count - 1;

   }
}

void SlottedPage::relocate(uint16_t slot_id, uint32_t data_size, uint32_t page_size) {
   Slot* slots = get_slots();

   if (data_size > (get_free_space() + slots[slot_id].get_size())) {
      throw std::invalid_argument("no space");
   }

   Slot& to_relocate = slots[slot_id];
   // case relocate with smaller size
   if (data_size <= to_relocate.get_size()) {
      header.free_space += (to_relocate.get_size() - data_size);
      to_relocate.set_slot(to_relocate.get_offset(), data_size, to_relocate.is_redirect_target());
      return;
   }

   if (data_size > get_fragmented_free_space()) {
      uint32_t slot_size = to_relocate.get_size();
      std::byte* buf = (std::byte*) malloc(std::min(data_size, slot_size));

      bool is_redirect_target = to_relocate.is_redirect_target();

      std::memmove(buf, &get_data()[to_relocate.get_offset()], std::min(data_size, slot_size));

      to_relocate.clear();

      compactify(page_size);

      header.data_start -= data_size;
      header.free_space -= data_size;
      header.free_space += slot_size;

      std::memmove(&get_data()[header.data_start], buf, std::min(data_size, slot_size));

      to_relocate.set_slot(header.data_start, data_size, is_redirect_target);

      free(buf);

      return;
   }

   header.data_start -= data_size;
   header.free_space -= data_size;
   header.free_space += to_relocate.get_size();

   // copying the data
   std::memmove(&get_data()[header.data_start], &get_data()[to_relocate.get_offset()], std::min(data_size,to_relocate.get_size()));

   to_relocate.set_slot(header.data_start, data_size, to_relocate.is_redirect_target());

}

void SlottedPage::erase(uint16_t slot_id) {
   Slot* slots = get_slots();
   Slot& to_erase = slots[slot_id];

   bool last = (to_erase.get_offset() == header.data_start);

   // if this was last allocated slot move data start to the right
   if (last) {
      header.slot_count -= 1;
      header.data_start += to_erase.get_size();
      header.free_space += (to_erase.get_size() + sizeof(Slot));


      int slot_id_iter = slot_id;
      slot_id_iter--;
      // iterate to the left and
      while (slot_id_iter >= 0 && slots[slot_id_iter].is_empty()) {
         header.free_space += sizeof(Slot);
         //don't change data_start
         // header.data_start += slots[slot_id_iter].get_size();
         header.slot_count -= 1;
         slot_id_iter--;
      }

      slot_id = slot_id_iter + 1;
   } else {
      if(!to_erase.is_redirect()) {
         header.free_space += (to_erase.get_size());
      }
   }

   to_erase.value = 0;

   header.first_free_slot = slot_id;

   if( header.first_free_slot > header.slot_count) {
      header.first_free_slot = header.slot_count;
   }

   assert(header.first_free_slot <= header.slot_count);
}

void SlottedPage::compactify(uint32_t page_size) {
   Slot* slots = get_slots();

   std::byte* data_copy = (std::byte*) malloc(page_size);

   std::memmove(data_copy, get_data(), page_size);

   std::byte* data = get_data();
   header.data_start = page_size;

   // for all non-empty and non-redirect slots, copy content to other offset
   // not compactifying slots but only payload
   for (uint16_t i = 0; i < header.slot_count; i++) {
      if (!slots[i].is_empty() && !slots[i].is_redirect()) {
         // move to correct position
         header.data_start -= slots[i].get_size();
         std::memmove(&data[header.data_start], &data_copy[slots[i].get_offset()], slots[i].get_size());
         slots[i].set_slot(header.data_start, slots[i].get_size(), slots[i].is_redirect_target());
      }
   }

   free(data_copy);

}
