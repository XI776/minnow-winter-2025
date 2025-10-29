#include "reassembler.hh"
#include "debug.hh"
#include <iostream>
#include <algorithm>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  // std::cerr << "first_index = " << first_index <<" expected_idx = " << next_assembled_idx_
  //  << " data = " << data << std::endl;

  auto pos_iter = unassemble_strs_.upper_bound(first_index);
  // 尝试获取一个小于等于 index 的迭代器指针
  if (pos_iter != unassemble_strs_.begin())
      pos_iter--;
  /**
   *  此时迭代器有三种情况，
   *  1. 一种是为 end_iter，表示不存在任何数据
   *  2. 一种是为非 end_iter，
   *      a. 一种表示确实存在一个小于等于 idx 的迭代器指针
   *      b. 一种表示当前指向的是大于 idx 的迭代器指针
   */


  size_t new_idx = first_index;
  // 如果前面确实有子串
  if(pos_iter != unassemble_strs_.end() && pos_iter->first <= first_index) {
    const size_t up_idx = pos_iter->first;

    // 如果当前子串前面出现了重叠
    if(first_index < up_idx + pos_iter->second.size()) {
      new_idx = up_idx + pos_iter->second.size();
    }
  }
  else if(first_index < next_assembled_idx_) {
    new_idx = next_assembled_idx_;
  }

  const size_t data_start_pos = new_idx - first_index;
  // 当前子串将保存的 data 的长度
    /**
     *  NOTE: 注意，若上一个子串将当前子串完全包含，则 data_size 不为正数
     *  NOTE: 这里由于 unsigned 的结果向 signed 的变量写入，因此可能存在上溢的可能
     *        但即便上溢，最多只会造成当前传入子串丢弃，不会产生任何安全隐患
     *  PS: 而且哪个数据包会这么大，大到超过 signed long 的范围
     */
  ssize_t data_size = data.size() - data_start_pos;


  pos_iter = unassemble_strs_.lower_bound(new_idx);





  while(pos_iter != unassemble_strs_.end() && new_idx <= pos_iter->first){
    const size_t data_end_pos = new_idx + data_size;

    if(pos_iter->first < data_end_pos) {
      
      if(data_end_pos < pos_iter->first + pos_iter->second.size()) {
        data_size = pos_iter->first - new_idx;
        break;
      }
      // 全部重叠
      else {
        unassembled_bytes_num_ -= pos_iter->second.size();
        pos_iter = unassemble_strs_.erase(pos_iter);
        continue;
      }
    }
    else 
      break;
  }






  size_t first_unacceptable_idx = next_assembled_idx_ + output_.writer().available_capacity();
  // if(first_unacceptable_idx <= new_idx)
  //   return;

  
  if (new_idx + data_size > first_unacceptable_idx) {
      // 整个段都不可接受，直接返回
      if (new_idx >= first_unacceptable_idx) {
          return;
      }
      // 只有部分可接受，截断 data_size
      data_size = first_unacceptable_idx - new_idx; 
  }
  if(data_size > 0) {
    const string new_data = data.substr(data_start_pos, data_size);
    // std::cerr << "new_data = " << new_data << " index = " << first_index << "\n";
    // std::cerr << "new_idx = " << new_idx << " next_index = " << next_assembled_idx_ << "\n";

    if(new_idx == next_assembled_idx_) {
      size_t write_byte = std::min(output_.writer().available_capacity(), static_cast<size_t>(data_size));
      output_.writer().push(new_data);
      // write_byte -= output_.writer().available_capacity();
      next_assembled_idx_ += write_byte;
      // std::cerr << "write_byte = " << write_byte << " next_assembled_idx_ = " << next_assembled_idx_ << "\n";
      // if(write_byte < data_size) {
      // // 确保 data_size 是非负数，然后进行安全比较
      // if (data_size > 0 && write_byte < static_cast<size_t>(data_size)) {
      //   const string data_to_store = new_data.substr(write_byte, data_size - write_byte);
      //   unassembled_bytes_num_ += data_to_store.size();
      //   unassemble_strs_.insert(make_pair(next_assembled_idx_, std::move(data_to_store)));
        
      // }
      

    }
    else {
        const string data_to_store = new_data.substr(0, new_data.size());
        unassembled_bytes_num_ += data_to_store.size();
        unassemble_strs_.insert(make_pair(new_idx, std::move(data_to_store)));
      }
  }
  // std::cerr << "unassemble_strs_ size = " << unassemble_strs_.size() << "\n";
  for(auto iter = unassemble_strs_.begin(); iter != unassemble_strs_.end();) {
    assert(next_assembled_idx_ <= iter->first);
    // std::cerr << "iter->first = " << iter->first << " next_idx = " << next_assembled_idx_ << "\n";

    if(iter->first == next_assembled_idx_) {
      const size_t write_num = min(output_.writer().available_capacity(), iter->second.size());
      output_.writer().push(iter->second);
      next_assembled_idx_ += write_num;

      if(write_num < iter->second.size()) {
        unassembled_bytes_num_ += iter->second.size() - write_num;
        unassemble_strs_.insert(make_pair(next_assembled_idx_, std::move(iter->second.substr(write_num))));

        unassembled_bytes_num_ -= iter->second.size();
        unassemble_strs_.erase(iter);
        break;
      }
      // std::cerr << "unassembled_bytes_num = " <<  unassembled_bytes_num_ << " str.size() = " << iter->second.size() << "\n";

      unassembled_bytes_num_ -= iter->second.size();
      iter = unassemble_strs_.erase(iter);
    }
    else 
      break;
  }

  if(is_last_substring) {
    eof_idx_ = first_index + data.size();
  }
  if(eof_idx_ <= next_assembled_idx_) {
    output_.writer().close();
  }
}
uint64_t Reassembler::count_bytes_pending() const {
  return unassembled_bytes_num_;
}
