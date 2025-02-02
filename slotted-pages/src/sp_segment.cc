#include "moderndbs/segment.h"
#include "moderndbs/slotted_page.h"
#include <iostream>

using moderndbs::Segment;
using moderndbs::SPSegment;
using moderndbs::TID;


bool SPSegment::testNotRedirectTarget(std::vector<TID> tids) {
   for(auto& tid: tids) {
      uint64_t page_id = tid.get_page_id(segment_id);
      uint16_t slot_id = tid.get_slot();

      auto& frame = buffer_manager.fix_page(page_id, false);
      SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());

      SlottedPage::Slot& slot = (sp->get_slots())[slot_id];
   }
   return true;
}


SPSegment::SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment& schema, FSISegment& fsi, schema::Table& table)
   : Segment(segment_id, buffer_manager), schema(schema), fsi(fsi), table(table) {
}

TID SPSegment::allocate(uint32_t size) {
   std::optional<uint64_t> free_page = fsi.find((size + sizeof(SlottedPage::Slot)));

   if (!free_page.has_value()) {
   a:
      auto& frame = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ table.allocated_pages, true);
      SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());

      sp->header = SlottedPage::Header(buffer_manager.get_page_size());

      std::memset(frame.get_data() + sizeof(SlottedPage), 0x00, buffer_manager.get_page_size() - sizeof(SlottedPage));

      uint16_t slot_id = sp->allocate(size, buffer_manager.get_page_size());

      fsi.update(table.allocated_pages, sp->get_free_space());

      table.allocated_pages += 1;

      buffer_manager.unfix_page(frame, true);

      return TID(table.allocated_pages - 1, slot_id);
   }

   BufferFrame& frame = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ free_page.value(), true);
   SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());


      uint16_t slot_id = sp->allocate(size, buffer_manager.get_page_size());
      fsi.update(free_page.value(), sp->get_free_space());
      buffer_manager.unfix_page(frame,true);
      return TID(free_page.value(), slot_id);
}

uint32_t SPSegment::read(TID tid, std::byte* record, uint32_t capacity) const {
   uint64_t page_id = tid.get_page_id(segment_id);
   uint16_t slot_id = tid.get_slot();

   auto& frame = buffer_manager.fix_page(page_id, false);
   SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());

   SlottedPage::Slot& slot = (sp->get_slots())[slot_id];
   std::byte* data = sp->get_data();

   if (slot.is_empty()) {
      return 0;
   } else if (slot.is_redirect()) {
      // Follow redirect
      TID redirect_target_tid = slot.as_redirect_tid();

      uint64_t page_id_red = redirect_target_tid.get_page_id(segment_id);
      uint16_t slot_id_red = redirect_target_tid.get_slot();

      auto& frame_red = buffer_manager.fix_page(page_id_red, false);
      SlottedPage* sp_red = reinterpret_cast<SlottedPage*>(frame_red.get_data());
      SlottedPage::Slot& slot_red = (sp_red->get_slots())[slot_id_red];
      std::byte* data_red = sp_red->get_data();

      std::memmove(record, &data_red[slot_red.get_offset() + 8], std::min(slot_red.get_size() - 8, capacity));

      buffer_manager.unfix_page(frame_red, false);

      return std::min(slot_red.get_size() - 8, capacity);

   } else if (slot.is_redirect_target()) {
      std::memmove(record, &data[slot.get_offset() + 8], std::min(slot.get_size() - 8, capacity));
      return std::min(slot.get_size() - 8, capacity);
   } else {
      std::memmove(record, &data[slot.get_offset()], std::min(slot.get_size(), capacity));
      return std::min(slot.get_size(), capacity);
   }

   buffer_manager.unfix_page(frame, false);
}

uint32_t SPSegment::write(TID tid, std::byte* record, uint32_t record_size) {
   uint64_t page_id = tid.get_page_id(segment_id);
   uint16_t slot_id = tid.get_slot();
   auto& frame = buffer_manager.fix_page(page_id, true);
   SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());
   SlottedPage::Slot& slot = (sp->get_slots())[slot_id];
   std::byte* data = sp->get_data();

   if (slot.is_empty()) {
      return 0;
   } else if (slot.is_redirect()) {
      TID redirect_target_tid = slot.as_redirect_tid();

      uint64_t page_id_red = redirect_target_tid.get_page_id(segment_id);
      uint16_t slot_id_red = redirect_target_tid.get_slot();
      auto& frame_red = buffer_manager.fix_page(page_id_red, true);
      SlottedPage* sp_red = reinterpret_cast<SlottedPage*>(frame_red.get_data());
      SlottedPage::Slot& slot_red = (sp_red->get_slots())[slot_id_red];
      std::byte* data_red = sp_red->get_data();

      std::memmove(&data_red[slot_red.get_offset() + 8], record, std::min(slot.get_size() - 8, record_size));

      buffer_manager.unfix_page(frame_red, true);
      return std::min(slot.get_size() - 8, record_size);
   } else if (slot.is_redirect_target()) {
      std::memmove(&data[slot.get_offset() + 8], record, std::min(slot.get_size() - 8, record_size));
      return std::min(slot.get_size() - 8, record_size);
   } else {
      assert(record_size <= slot.get_size());

      std::memmove(&data[slot.get_offset()], record, std::min(slot.get_size(), record_size));
      return std::min(slot.get_size(), record_size);
   }

   buffer_manager.unfix_page(frame, true);
}

void SPSegment::resize(TID tid, uint32_t new_length) {
   uint64_t page_id = tid.get_page_id(segment_id);
   uint16_t slot_id = tid.get_slot();
   auto& frame = buffer_manager.fix_page(page_id, false);
   SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());
   SlottedPage::Slot& slot = (sp->get_slots())[slot_id];
   std::byte* data = sp->get_data();


   if (slot.is_redirect()) {
      TID redirect_target_tid = slot.as_redirect_tid();

      uint64_t page_id_red = redirect_target_tid.get_page_id(segment_id);
      uint16_t slot_id_red = redirect_target_tid.get_slot();
      auto& frame_red = buffer_manager.fix_page(page_id_red, true);
      SlottedPage* sp_red = reinterpret_cast<SlottedPage*>(frame_red.get_data());
      SlottedPage::Slot& slot_red = (sp_red->get_slots())[slot_id_red];
      std::byte* data_red = sp_red->get_data();


      if (slot_red.get_size() >= (new_length+8)) {
         sp_red->relocate(slot_id_red, new_length + 8, buffer_manager.get_page_size());
         fsi.update((page_id_red & 0x0000FFFFFFFFFFFF), sp_red->get_free_space());
      } else if ((int) sp_red->get_free_space() >= (((int) new_length + 8) - (int) slot_red.get_size())) {
         sp_red->relocate(slot_id_red, new_length + 8, buffer_manager.get_page_size());
         fsi.update((page_id_red & 0x0000FFFFFFFFFFFF), sp_red->get_free_space());
      } else {
         // set up the new redirect target
         TID redirect_target_tid_2 = allocate(new_length + 8);
         slot.set_redirect_tid(redirect_target_tid_2);

         uint64_t page_id_red_2 = redirect_target_tid_2.get_page_id(segment_id);
         uint16_t slot_id_red_2 = redirect_target_tid_2.get_slot();
         auto& frame_red_2 = buffer_manager.fix_page(page_id_red_2, false);
         SlottedPage* sp_red_2 = reinterpret_cast<SlottedPage*>(frame_red_2.get_data());
         SlottedPage::Slot& slot_red_2 = (sp_red_2->get_slots())[slot_id_red_2];
         slot_red_2.mark_as_redirect_target(true);
         std::byte* data_red_2 = sp_red_2->get_data();

         // move from other redirected slot to this one
         std::memmove(&data_red_2[slot_red_2.get_offset()], &data_red[slot_red.get_offset()], slot_red.get_size());

         erase(redirect_target_tid);

         buffer_manager.unfix_page(frame_red_2, true);
      }

      buffer_manager.unfix_page(frame_red, true);
   } else if (slot.is_redirect_target()) {
      assert(false);
   } else {
      if (slot.get_size() >= new_length) {
         sp->relocate(slot_id, new_length, buffer_manager.get_page_size());
         fsi.update((page_id & 0x0000FFFFFFFFFFFF), sp->get_free_space());
      } else if ((int) sp->get_free_space() >= ((int) new_length - (int) slot.get_size())) {
         sp->relocate(slot_id, new_length, buffer_manager.get_page_size());
         fsi.update((page_id & 0x0000FFFFFFFFFFFF), sp->get_free_space());
      } else {
         assert(slot.get_size() < new_length);

         TID redirect_target_tid = allocate(new_length + 8);

         uint64_t page_id_red = redirect_target_tid.get_page_id(segment_id);
         uint16_t slot_id_red = redirect_target_tid.get_slot();
         auto& frame_red = buffer_manager.fix_page(page_id_red, true);
         SlottedPage* sp_red = reinterpret_cast<SlottedPage*>(frame_red.get_data());
         SlottedPage::Slot& slot_red = (sp_red->get_slots())[slot_id_red];
         std::byte* data_red = sp_red->get_data();

         //  mark new slot as redirect target
         slot_red.mark_as_redirect_target(true);

         std::byte* data_copy = (std::byte*) malloc(8 + new_length);

         uint64_t raw = tid.get_value();

         // move original TID into the first 8 bytes
         std::memmove(data_copy, &raw, 8);

         // assert  tid not correctly copied into buffer
         uint64_t* raw_after = reinterpret_cast<uint64_t*>(data_copy);
         TID t(*raw_after);

         assert(t.get_page_id(segment_id) == tid.get_page_id(segment_id));
         assert(t.get_slot() == tid.get_slot());

         // move the data into data_copy
         std::memmove(data_copy + 8, &data[slot.get_offset()], slot.get_size());
         std::memmove(&data_red[slot_red.get_offset()], data_copy, 8 + slot.get_size());

         free(data_copy);

         slot.set_redirect_tid(redirect_target_tid);

         buffer_manager.unfix_page(frame_red, true);
      }
   }

   buffer_manager.unfix_page(frame, true);
}

void SPSegment::erase(TID tid) {
   uint64_t page_id = tid.get_page_id(segment_id);
   uint16_t slot_id = tid.get_slot();
   auto& frame = buffer_manager.fix_page(page_id, false);
   SlottedPage* sp = reinterpret_cast<SlottedPage*>(frame.get_data());
   SlottedPage::Slot& slot = (sp->get_slots())[slot_id];
   std::byte* data = sp->get_data();

   if (slot.is_redirect()) {
      TID redirect_target_tid = slot.as_redirect_tid();

      uint64_t page_id_red = redirect_target_tid.get_page_id(segment_id);
      uint16_t slot_id_red = redirect_target_tid.get_slot();
      auto& frame_red = buffer_manager.fix_page(page_id_red, true);
      SlottedPage* sp_red = reinterpret_cast<SlottedPage*>(frame_red.get_data());

      sp_red->erase(slot_id_red);
      sp->erase(slot_id);
      fsi.update((page_id_red & 0x0000FFFFFFFFFFFF), sp_red->get_free_space());
      fsi.update((page_id & 0x0000FFFFFFFFFFFF), sp->get_free_space());

      buffer_manager.unfix_page(frame_red, true);

   } else if (slot.is_redirect_target()) {
      sp->erase(slot_id);
      fsi.update((page_id & 0x0000FFFFFFFFFFFF), sp->get_free_space());
   } else {
      sp->erase(slot_id);
      fsi.update((page_id & 0x0000FFFFFFFFFFFF), sp->get_free_space());
   }

   buffer_manager.unfix_page(frame, true);
}
