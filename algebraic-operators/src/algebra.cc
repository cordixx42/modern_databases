#include "moderndbs/algebra.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace moderndbs::iterator_model {

// /// This can be used to store registers in an `std::unordered_map` or
// /// `std::unordered_set`. Examples:
// ///
// /// std::unordered_map<Register, int, RegisterHasher> map_from_reg_to_int;
// /// std::unordered_set<Register, RegisterHasher> set_of_regs;
// struct RegisterHasher {
//    uint64_t operator()(const Register& r) const {
//       return r.get_hash();
//    }
// };

// /// This can be used to store vectors of registers (which is how tuples are
// /// represented) in an `std::unordered_map` or `std::unordered_set`. Examples:
// ///
// /// std::unordered_map<std::vector<Register>, int, RegisterVectorHasher> map_from_tuple_to_int;
// /// std::unordered_set<std::vector<Register>, RegisterVectorHasher> set_of_tuples;
// struct RegisterVectorHasher {
//    uint64_t operator()(const std::vector<Register>& registers) const {
//       std::string hash_values;
//       for (auto& reg : registers) {
//          uint64_t hash = reg.get_hash();
//          hash_values.append(reinterpret_cast<char*>(&hash), sizeof(hash));
//       }
//       return std::hash<std::string>{}(hash_values);
//    }
// };

Register Register::from_int(int64_t value) {
   Register reg{};
   reg.type = Type::INT64;
   reg.value_int = value;
   return reg;
}

Register Register::from_string(const std::string& value) {
   assert(value.size() >= 16);
   Register reg{};
   reg.type = Type::CHAR16;
   reg.value_string = value;
   reg.value_string.resize(16);
   return reg;
}

Register::Type Register::get_type() const {
   return type;
}

int64_t Register::as_int() const {
   assert(type == Type::INT64);
   return value_int;
}

std::string Register::as_string() const {
   assert(type == Type::CHAR16);
   return value_string;
}

void Register::add_int(uint64_t val) {
   assert(get_type() == Type::INT64);
   value_int += val;
}

uint64_t Register::get_hash() const {
   if (type == Type::INT64) {
      return value_int;
   } else {
      return std::hash<std::string>{}(value_string);
   }
}

bool operator==(const Register& r1, const Register& r2) {
   if (r1.type == r2.type) {
      if (r1.type == Register::Type::INT64) {
         if (r1.value_int == r2.value_int) {
            return true;
         }
      } else {
         if (r1.value_string == r2.value_string) {
            return true;
         }
      }
   }
   return false;
}

bool operator!=(const Register& r1, const Register& r2) {
   return !(operator==(r1, r2));
}

bool operator<(const Register& r1, const Register& r2) {
   assert(r1.get_type() == r2.get_type());
   if (r1.type == Register::Type::INT64) {
      return r1.value_int < r2.value_int;
   } else {
      return r1.value_string < r2.value_string;
   }
}

bool operator<=(const Register& r1, const Register& r2) {
   assert(r1.get_type() == r2.get_type());
   if (r1.type == Register::Type::INT64) {
      return r1.value_int <= r2.value_int;
   } else {
      return r1.value_string <= r2.value_string;
   }
}

bool operator>(const Register& r1, const Register& r2) {
   assert(r1.get_type() == r2.get_type());
   if (r1.type == Register::Type::INT64) {
      return r1.value_int > r2.value_int;
   } else {
      return r1.value_string > r2.value_string;
   }
}

bool operator>=(const Register& r1, const Register& r2) {
   assert(r1.get_type() == r2.get_type());
   if (r1.type == Register::Type::INT64) {
      return r1.value_int >= r2.value_int;
   } else {
      return r1.value_string >= r2.value_string;
   }
}

Print::Print(Operator& input, std::ostream& stream) : UnaryOperator(input), stream(stream) {
}

Print::~Print() = default;

void Print::open() {
   input->open();
}

bool Print::next() {
   if (input->next()) {
      auto tuple = input->get_output();
      for (int i = 0; i < tuple.size(); i++) {
         Register* reg_ptr = tuple[i];
         if (reg_ptr->get_type() == Register::Type::INT64) {
            stream << reg_ptr->as_int();
         } else {
            assert(reg_ptr->get_type() == Register::Type::CHAR16);

            stream << reg_ptr->as_string();
         }
         if (i < tuple.size() - 1) {
            stream << ",";
         }
      }
      stream << "\n";
      return true;
   }
   return false;
}

void Print::close() {
   input->close();
}

std::vector<Register*> Print::get_output() {
   // Print has no output
   return {};
}

Projection::Projection(Operator& input, std::vector<size_t> attr_indexes)
   : UnaryOperator(input), attr_indexes(attr_indexes.begin(), attr_indexes.end()) {
}

Projection::~Projection() = default;

void Projection::open() {
   input->open();
}

bool Projection::next() {
   regs.clear();
   if (input->next()) {
      auto input_regs = input->get_output();
      for (int i = 0; i < input_regs.size(); i++) {
         if (attr_indexes.contains(i)) {
            regs.push_back(input_regs[i]);
         }
      }
      return true;
   }
   return false;
}

void Projection::close() {
   input->close();
}

std::vector<Register*> Projection::get_output() {
   return regs;
}

Select::Select(Operator& input, PredicateAttributeInt64 predicate)
   : UnaryOperator(input), pred(predicate) {
}

Select::Select(Operator& input, PredicateAttributeChar16 predicate)
   : UnaryOperator(input), pred(predicate) {
}

Select::Select(Operator& input, PredicateAttributeAttribute predicate)
   : UnaryOperator(input), pred(predicate) {
}

Select::~Select() = default;

void Select::open() {
   input->open();
}

bool Select::next() {
   regs.clear();
   while (input->next()) {
      bool pred_holds = true;
      auto input_regs = input->get_output();
      if (std::holds_alternative<PredicateAttributeInt64>(pred)) {
         PredicateAttributeInt64 pred_int = std::get<PredicateAttributeInt64>(pred);

         switch (pred_int.predicate_type) {
            case PredicateType::EQ:
               (!(input_regs[pred_int.attr_index]->as_int() == pred_int.constant)) && (pred_holds = false);
               break;
            case PredicateType::NE:
               (!(input_regs[pred_int.attr_index]->as_int() != pred_int.constant)) && (pred_holds = false);
               break;
            case PredicateType::LT:
               (!(input_regs[pred_int.attr_index]->as_int() < pred_int.constant)) && (pred_holds = false);
               break;
            case PredicateType::LE:
               (!(input_regs[pred_int.attr_index]->as_int() <= pred_int.constant)) && (pred_holds = false);
               break;
            case PredicateType::GT:
               (!(input_regs[pred_int.attr_index]->as_int() > pred_int.constant)) && (pred_holds = false);
               break;
            case PredicateType::GE:
               (!(input_regs[pred_int.attr_index]->as_int() >= pred_int.constant)) && (pred_holds = false);
               break;
            default:
               break;
         }

      } else if (std::holds_alternative<PredicateAttributeChar16>(pred)) {
         PredicateAttributeChar16 pred_str = std::get<PredicateAttributeChar16>(pred);

         switch (pred_str.predicate_type) {
            case PredicateType::EQ:
               (!(input_regs[pred_str.attr_index]->as_string() == pred_str.constant)) && (pred_holds = false);
               break;
            case PredicateType::NE:
               (!(input_regs[pred_str.attr_index]->as_string() != pred_str.constant)) && (pred_holds = false);
               break;
            case PredicateType::LT:
               (!(input_regs[pred_str.attr_index]->as_string() < pred_str.constant)) && (pred_holds = false);
               break;
            case PredicateType::LE:
               (!(input_regs[pred_str.attr_index]->as_string() <= pred_str.constant)) && (pred_holds = false);
               break;
            case PredicateType::GT:
               (!(input_regs[pred_str.attr_index]->as_string() > pred_str.constant)) && (pred_holds = false);
               break;
            case PredicateType::GE:
               (!(input_regs[pred_str.attr_index]->as_string() >= pred_str.constant)) && (pred_holds = false);
               break;
            default:
               break;
         }
      } else {
         PredicateAttributeAttribute pred_attr = std::get<PredicateAttributeAttribute>(pred);
         switch (pred_attr.predicate_type) {
            case PredicateType::EQ:
               (!(*input_regs[pred_attr.attr_left_index] == *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            case PredicateType::NE:
               (!(*input_regs[pred_attr.attr_left_index] != *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            case PredicateType::LT:
               (!(*input_regs[pred_attr.attr_left_index] < *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            case PredicateType::LE:
               (!(*input_regs[pred_attr.attr_left_index] <= *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            case PredicateType::GT:
               (!(*input_regs[pred_attr.attr_left_index] > *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            case PredicateType::GE:
               (!(*input_regs[pred_attr.attr_left_index] >= *input_regs[pred_attr.attr_right_index])) && (pred_holds = false);
               break;
            default:
               break;
         }
      }
      if (pred_holds) {
         regs = input_regs;
         return true;
      }
   }
   return false;
}

void Select::close() {
   input->close();
}

std::vector<Register*> Select::get_output() {
   return regs;
}

Sort::Sort(Operator& input, std::vector<Criterion> criteria)
   : UnaryOperator(input), criteria(criteria) {
}

Sort::~Sort() = default;

void Sort::open() {
   input->open();
   while (input->next()) {
      std::vector<Register*> newvec{};

      for (auto reg : input->get_output()) {
         Register* r = new Register(*reg);
         newvec.push_back(r);
      }

      all_materialized.push_back(newvec);
      // all_materialized.push_back(input->get_output());
   }

   // WHY CAN'T THIS BE HERE
   // input->close();

   int prev_idx = -1;
   bool prev_desc;

   for (auto& crit : criteria) {
      if (crit.desc) {
         std::sort(all_materialized.begin(), all_materialized.end(), [&](std::vector<Register*> a, std::vector<Register*> b) {
            if (prev_idx == -1) {
               return *a[crit.attr_index] > *b[crit.attr_index];
            }
            if (*a[prev_idx] == *b[prev_idx]) {
               return *a[crit.attr_index] > *b[crit.attr_index];
            } else {
               return prev_desc ? (*a[prev_idx] > *b[prev_idx]) : (*a[prev_idx] < *b[prev_idx]);
            }
         });
      } else {
         std::sort(all_materialized.begin(), all_materialized.end(), [&](std::vector<Register*> a, std::vector<Register*> b) {
            if (prev_idx == -1) {
               return *a[crit.attr_index] < *b[crit.attr_index];
            }
            if (*a[prev_idx] == *b[prev_idx]) {
               return *a[crit.attr_index] < *b[crit.attr_index];
            } else {
               return prev_desc ? (*a[prev_idx] > *b[prev_idx]) : (*a[prev_idx] < *b[prev_idx]);
            }
         });
      }

      prev_idx = crit.attr_index;
      prev_desc = crit.desc;
   }
}

bool Sort::next() {
   output.clear();
   if (output_idx < all_materialized.size()) {
      output = all_materialized[output_idx];
      output_idx++;
      return true;
   }
   return false;
}

std::vector<Register*> Sort::get_output() {
   return output;
}

void Sort::close() {
   input->close();

   for (auto& vec : all_materialized) {
      for (auto reg : vec) {
         delete reg;
      }
   }
}

HashJoin::HashJoin(
   Operator& input_left,
   Operator& input_right,
   size_t attr_index_left,
   size_t attr_index_right) : BinaryOperator(input_left, input_right), attr_index_left(attr_index_left), attr_index_right(attr_index_right) {
   // TODO: add your implementation here
}

HashJoin::~HashJoin() = default;

void HashJoin::open() {
   input_left->open();
   while (input_left->next()) {
      Register& reg = *(input_left->get_output()[attr_index_left]);

      //TODO: new Registers
      std::vector<Register*> newvec{};

      for (auto rg : input_left->get_output()) {
         Register* r = new Register(*rg);
         newvec.push_back(r);
      }

      registersLeft.emplace(reg, newvec);
   }
   input_right->open();
}

bool HashJoin::next() {
   output.clear();
   if (input_right->next()) {
      Register& reg = *(input_right->get_output()[attr_index_right]);
      if (registersLeft.contains(reg)) {
         output = registersLeft[reg];

         std::vector<Register*> newvec{};

         for (auto rg : input_right->get_output()) {
            Register* r = new Register(*rg);
            registersRight.push_back(r);
            newvec.push_back(r);
         }

         output.insert(output.end(), newvec.begin(), newvec.end());
         return true;
      } else {
         return false;
      }
   }
   return false;
}

void HashJoin::close() {
   input_left->close();
   input_right->close();

   for (auto& [k, v] : registersLeft) {
      for (auto reg : v) {
         delete reg;
      }
   }

   for (auto reg : registersRight) {
      delete reg;
   }
}

std::vector<Register*> HashJoin::get_output() {
   return output;
}

HashAggregation::HashAggregation(
   Operator& input,
   std::vector<size_t>
      group_by_attrs,
   std::vector<AggrFunc>
      aggr_funcs) : UnaryOperator(input), group_by_attrs(group_by_attrs), aggr_funcs(aggr_funcs) {
}

HashAggregation::~HashAggregation() = default;

void HashAggregation::open() {
   input->open();

   while (input->next()) {
      std::vector<Register*> newvec{};

      for (auto reg : input->get_output()) {
         Register* r = new Register(*reg);
         assert(r->get_type() == reg->get_type());
         newvec.push_back(r);
      }

      all_materialized.push_back(newvec);
   }

   // aggregation over everything
   if (group_by_attrs.empty()) {
      for (auto fun : aggr_funcs) {
         switch (fun.func) {
            case AggrFunc::Func::COUNT: {
               Register* newreg_count = new Register(Register::from_int(all_materialized.size()));
               new_aggregate_registers.push_back(newreg_count);
               output.push_back(newreg_count);
               break;
            }
            case AggrFunc::Func::MAX: {
               if (all_materialized.empty()) {
                  break;
               }
               switch (all_materialized[0].at(fun.attr_index)->get_type()) {
                  case Register::Type::INT64: {
                     uint64_t max = all_materialized[0].at(fun.attr_index)->as_int();
                     for (auto& tuple : all_materialized) {
                        // max = std::max(max, tuple[fun.attr_index]->as_int());
                        if (tuple[fun.attr_index]->as_int() > max) {
                           max = tuple[fun.attr_index]->as_int();
                        }
                     }
                     Register* newreg_max = new Register(Register::from_int(max));
                     new_aggregate_registers.push_back(newreg_max);
                     output.push_back(newreg_max);
                     break;
                  }

                  case Register::Type::CHAR16: {
                     std::string maxx = all_materialized[0].at(fun.attr_index)->as_string();
                     for (auto& tuple : all_materialized) {
                        if (tuple[fun.attr_index]->as_string() > maxx) {
                           maxx = tuple[fun.attr_index]->as_string();
                        }
                     }
                     Register* newreg_maxx = new Register(Register::from_string(maxx));
                     new_aggregate_registers.push_back(newreg_maxx);
                     output.push_back(newreg_maxx);
                     break;
                  } break;
               }
               break;
            }
            case AggrFunc::Func::MIN: {
               if (all_materialized.empty()) {
                  break;
               }
               switch (all_materialized[0].at(fun.attr_index)->get_type()) {
                  case Register::Type::INT64: {
                     uint64_t min = all_materialized[0].at(fun.attr_index)->as_int();
                     for (auto& tuple : all_materialized) {
                        // max = std::max(max, tuple[fun.attr_index]->as_int());
                        if (tuple[fun.attr_index]->as_int() < min) {
                           min = tuple[fun.attr_index]->as_int();
                        }
                     }
                     Register* newreg_min = new Register(Register::from_int(min));
                     new_aggregate_registers.push_back(newreg_min);
                     output.push_back(newreg_min);
                     break;
                  }
                  case Register::Type::CHAR16: {
                     std::string minn = all_materialized[0].at(fun.attr_index)->as_string();
                     for (auto& tuple : all_materialized) {
                        if (tuple[fun.attr_index]->as_string() < minn) {
                           minn = tuple[fun.attr_index]->as_string();
                        }
                     }
                     Register* newreg_minn = new Register(Register::from_string(minn));
                     new_aggregate_registers.push_back(newreg_minn);
                     output.push_back(newreg_minn);
                     break;
                  } break;
               }
               break;
            }
            case AggrFunc::Func::SUM: {
               uint64_t sum = 0;
               for (auto& tuple : all_materialized) {
                  sum += tuple[fun.attr_index]->as_int();
               }
               Register* newreg_sum = new Register(Register::from_int(sum));
               new_aggregate_registers.push_back(newreg_sum);

               output.push_back(newreg_sum);
               break;
            }
            default:
               break;
         }
      }

      res_general.insert(std::make_pair(std::vector<Register>{}, output));

      return;
   }

   std::unordered_map<std::vector<Register>, std::vector<Register*>, RegisterVectorHasher> ress_general;

   for (auto& tuple : all_materialized) {
      std::vector<Register> all_gb_a{};
      for (auto& gb_a : group_by_attrs) {
         all_gb_a.push_back(*tuple[gb_a]);
      }

      if (ress_general.contains(all_gb_a)) {
         auto& existing_vec = ress_general.at(all_gb_a);

         // int idx = 1;
         int idx = group_by_attrs.size();
         for (auto fun : aggr_funcs) {
            switch (fun.func) {
               case AggrFunc::Func::COUNT:
                  if (idx < existing_vec.size()) {
                     existing_vec[idx]->add_int(1);
                  } else {
                     assert(existing_vec.size() == idx);
                     Register* newreg = new Register(Register::from_int(1));
                     new_aggregate_registers.push_back(newreg);
                     assert(newreg->get_type() == Register::Type::INT64);
                     existing_vec.push_back(newreg);
                  }
                  break;
               case AggrFunc::Func::MAX:
                  break;
               case AggrFunc::Func::MIN:
                  break;
               case AggrFunc::Func::SUM:
                  if (idx < existing_vec.size()) {
                     existing_vec[idx]->add_int(tuple[fun.attr_index]->as_int());
                  } else {
                     assert(existing_vec.size() == idx);
                     Register* newreg = new Register(Register::from_int(tuple[fun.attr_index]->as_int()));
                     new_aggregate_registers.push_back(newreg);
                     assert(newreg->get_type() == Register::Type::INT64);
                     existing_vec.push_back(newreg);
                  }
                  break;
               default:
                  break;
            }
            idx++;
         }

      } else {
         std::vector<Register*> output;
         for (auto& gb_a : group_by_attrs) {
            output.push_back(tuple[gb_a]);
         }

         ress_general.insert(std::make_pair(all_gb_a, output));

         auto& new_inserted_vec = ress_general.at(all_gb_a);

         int idx = group_by_attrs.size();
         for (auto fun : aggr_funcs) {
            switch (fun.func) {
               case AggrFunc::Func::COUNT:
                  if (idx < new_inserted_vec.size()) {
                     new_inserted_vec[idx]->add_int(1);
                  } else {
                     assert(new_inserted_vec.size() == idx);
                     Register* newreg = new Register(Register::from_int(1));
                     new_aggregate_registers.push_back(newreg);
                     assert(newreg->get_type() == Register::Type::INT64);
                     new_inserted_vec.push_back(newreg);
                  }
                  break;
               case AggrFunc::Func::MAX:
                  break;
               case AggrFunc::Func::MIN:
                  break;
               case AggrFunc::Func::SUM:
                  if (idx < new_inserted_vec.size()) {
                     new_inserted_vec[idx]->add_int(tuple[fun.attr_index]->as_int());
                  } else {
                     assert(new_inserted_vec.size() == idx);
                     Register* newreg = new Register(Register::from_int(tuple[fun.attr_index]->as_int()));
                     new_aggregate_registers.push_back(newreg);
                     assert(newreg->get_type() == Register::Type::INT64);
                     new_inserted_vec.push_back(newreg);
                  }
                  break;
               default:
                  break;
            }
            idx++;
         }
      }
   }

   res_general = ress_general;
}

bool HashAggregation::next() {
   if (!res_general.empty()) {
      output = (*res_general.begin()).second;
      res_general.erase(res_general.begin());
      return true;
   }
   return false;
}

void HashAggregation::close() {
   input->close();
   for (auto& vec : all_materialized) {
      for (auto reg : vec) {
         delete reg;
      }
   }

   for (auto reg : new_aggregate_registers) {
      delete reg;
   }
}

std::vector<Register*> HashAggregation::get_output() {
   return output;
}

Union::Union(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
}

Union::~Union() = default;

void Union::open() {
   input_left->open();
   input_right->open();
}

bool Union::next() {
   while (input_left->next()) {
      auto left = input_left->get_output();
      std::vector<Register> newvec{};
      for (auto reg : left) {
         newvec.push_back(*reg);
      }
      if (encountered.contains(newvec)) {
         continue;
      } else {
         encountered.insert(newvec);
         output = left;
         return true;
      }
   }

   while (input_right->next()) {
      auto right = input_right->get_output();
      std::vector<Register> newvec{};
      for (auto reg : right) {
         newvec.push_back(*reg);
      }
      if (encountered.contains(newvec)) {
         continue;
      } else {
         encountered.insert(newvec);
         output = right;
         return true;
      }
   }
   return false;
}

std::vector<Register*> Union::get_output() {
   return output;
}

void Union::close() {
   input_left->close();
   input_right->close();
}

UnionAll::UnionAll(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
}

UnionAll::~UnionAll() = default;

void UnionAll::open() {
   input_left->open();
   input_right->open();
}

bool UnionAll::next() {
   if (input_left->next()) {
      output = input_left->get_output();
      return true;
   }

   if (input_right->next()) {
      output = input_right->get_output();
      return true;
   }

   return false;
}

std::vector<Register*> UnionAll::get_output() {
   return output;
}

void UnionAll::close() {
   input_left->close();
   input_right->close();
}

Intersect::Intersect(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
   // TODO: add your implementation here
}

Intersect::~Intersect() = default;

void Intersect::open() {
   input_left->open();
   input_right->open();

   while (input_left->next()) {
      std::vector<Register> newvec{};

      for (auto reg : input_left->get_output()) {
         Register* r = new Register(*reg);
         to_free.push_back(r);
         newvec.push_back(*r);
      }

      encountered.insert(newvec);
   }
}

bool Intersect::next() {
   while (input_right->next()) {
      std::vector<Register> newvec{};
      for (auto reg : input_right->get_output()) {
         newvec.push_back(*reg);
      }
      if (encountered.contains(newvec)) {
         output = input_right->get_output();
         encountered.erase(newvec);
         return true;
      }
   }
   return false;
}

std::vector<Register*> Intersect::get_output() {
   return output;
}

void Intersect::close() {
   input_left->close();
   input_right->close();
   for (auto reg : to_free) {
      delete reg;
   }
}

IntersectAll::IntersectAll(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
}

IntersectAll::~IntersectAll() = default;

void IntersectAll::open() {
   input_left->open();
   input_right->open();

   while (input_left->next()) {
      std::vector<Register> newvec{};

      for (auto reg : input_left->get_output()) {
         Register* r = new Register(*reg);
         to_free.push_back(r);
         newvec.push_back(*r);
      }

      if (encountered_left_to_count.contains(newvec)) {
         encountered_left_to_count.at(newvec) += 1;
      } else {
         encountered_left_to_count.insert(std::make_pair(newvec, 1));
      }
   }
}

bool IntersectAll::next() {
   while (input_right->next()) {
      std::vector<Register> newvec{};
      for (auto reg : input_right->get_output()) {
         newvec.push_back(*reg);
      }
      if (encountered_left_to_count.contains(newvec) && encountered_left_to_count.at(newvec) > 0) {
         output = input_right->get_output();
         encountered_left_to_count.at(newvec) -= 1;
         return true;
      }
   }
   return false;
}

std::vector<Register*> IntersectAll::get_output() {
   return output;
}

void IntersectAll::close() {
   input_left->close();
   input_right->close();

   for (auto reg : to_free) {
      delete reg;
   }
}

Except::Except(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
}

Except::~Except() = default;

void Except::open() {
   input_left->open();
   input_right->open();

   while (input_right->next()) {
      std::vector<Register> newvec{};

      for (auto reg : input_right->get_output()) {
         Register* r = new Register(*reg);
         to_free.push_back(r);
         newvec.push_back(*r);
      }

      encountered_right.insert(newvec);
   }
}

bool Except::next() {
   while (input_left->next()) {
      std::vector<Register> newvec{};
      for (auto reg : input_left->get_output()) {
         newvec.push_back(*reg);
      }
      if (!encountered_right.contains(newvec)) {
         output = input_left->get_output();
         // to guarantee distinctness ?
         encountered_right.insert(newvec);
         return true;
      }
   }
   return false;
}

std::vector<Register*> Except::get_output() {
   return output;
}

void Except::close() {
   input_left->close();
   input_right->close();
   for (auto reg : to_free) {
      delete reg;
   }
}

ExceptAll::ExceptAll(Operator& input_left, Operator& input_right)
   : BinaryOperator(input_left, input_right) {
}

ExceptAll::~ExceptAll() = default;

void ExceptAll::open() {
   input_left->open();
   input_right->open();

   while (input_right->next()) {
      std::vector<Register> newvec{};

      for (auto reg : input_right->get_output()) {
         Register* r = new Register(*reg);
         to_free.push_back(r);
         newvec.push_back(*r);
      }

      if (encountered_right_to_count.contains(newvec)) {
         encountered_right_to_count.at(newvec) += 1;
      } else {
         encountered_right_to_count.insert(std::make_pair(newvec, 1));
      }
   }
}

bool ExceptAll::next() {
   while (input_left->next()) {
      std::vector<Register> newvec{};
      for (auto reg : input_left->get_output()) {
         newvec.push_back(*reg);
      }

      if (!encountered_right_to_count.contains(newvec)) {
         output = input_left->get_output();
         return true;
      } else if (encountered_right_to_count.contains(newvec)) {
         if (encountered_right_to_count.at(newvec) > 0) {
            encountered_right_to_count.at(newvec) -= 1;
         } else {
            output = input_left->get_output();
            return true;
         }
      }
   }
   return false;
}

std::vector<Register*> ExceptAll::get_output() {
   return output;
}

void ExceptAll::close() {
   input_left->close();
   input_right->close();
   for (auto reg : to_free) {
      delete reg;
   }
}

} // namespace moderndbs
