#include "reassembler.hh"
#include "debug.hh"
#include <iostream>
#include <algorithm>
using namespace std;
void Reassembler::append_to_map(uint64_t first_index, const string& data) {
  if(str_capacity_ == count_bytes_pending()) {
    return;
  }
  for(auto& it: strs) {
    if(first_index >= it.first && it.first + it.second.size() > first_index) {
      
      int size = it.second.size() + it.first;
      uint64_t new_index = size - first_index;
      
      strs[new_index] = data.substr(new_index, str_capacity_ - count_bytes_pending());
      return;
    }
  }
  strs[first_index] = data.substr(0, str_capacity_ - count_bytes_pending());
}
// void Reassembler::append_to_map(uint64_t first_index, const string &data) {
//     if (data.empty()) return;

//     // 如果 key 已经存在，需要合并，保证不重复覆盖
//     auto it = strs.find(first_index);
//     if (it != strs.end()) {
//         // 合并已有的数据和新数据（保留更长的）
//         if (data.size() > it->second.size()) {
//             it->second = data;
//         }
//         return;
//     }

//     // 否则，直接插入
//     strs[first_index] = data;
// }

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{

  uint64_t window_start = next_index;
  uint64_t window_end = next_index + str_capacity_;
  if(first_index >= window_end || first_index + data.size() < window_start) {
    return;
  }
  
  uint64_t start = std::max(first_index, window_start);
  uint64_t end = std::min(first_index + data.size(), window_end);
  data = data.substr(start - first_index, end - start);
  first_index = start;



  std::cerr << "before: next_index= " << next_index
     << " buffer_size= " << output_.writer().bytes_pushed() 
     << " capacity= " << output_.writer().available_capacity() 
     << " \n";
  
  std::cerr << "data is " << data << " and data.empty() is " << data.empty() << "\n";
  if(first_index == next_index) {
    uint64_t writable = min<uint64_t>(data.size(), output_.writer().available_capacity());
    output_.writer().push(data.substr(0, writable));
    next_index += writable;

    // if (writable < data.size()) {
    //     // strs[next_index] = data.substr(writable);
    //     // std::cerr << "data.empty" << data << '\n';
    //     append_to_map(next_index, data.substr(writable));
    // }
  }
  else {
    if(!data.empty())
      append_to_map(first_index, data);
  }

  for (auto &kv : strs) {
    cerr << "STRS: stored: key = " << kv.first << " val is " << kv.second <<
    " val.size = " << kv.second.size() << "\n";
}

  // while (true) {
      // auto it = strs.find(next_index);
      for(auto& it: strs) {
        if(next_index >= it.first && (it.first + it.second.size() > next_index)) {
          uint64_t cur_size = it.second.size() - next_index;
          size_t writable = std::min(cur_size , output_.writer().available_capacity());
          if (writable == 0) break;

          output_.writer().push(it.second.substr(0, writable));
          next_index += writable;
          
        }
      // }
      // if (it == strs.end()) break;
      // size_t writable = min(it->second.size(), output_.writer().available_capacity());
      // if (writable == 0) break;

      // output_.writer().push(it->second.substr(0, writable));
      // next_index += writable;

      // if (writable < it->second.size()) {
      //     // 只写了一部分，剩下的还要存回去
      //     strs[next_index] = it->second.substr(writable);
      //     strs.erase(it);
      //     break;
      // } else {
      //     strs.erase(it);
      // }
  }
  
//  // 记录终点
//   if (is_last_substring) {
//       last_index_ = first_index + data.size();
//   }

//   // 检查是否到达终点
//   if (last_index_.has_value() && next_index == *last_index_) {
//       output_.writer().close();
//   }
  if (is_last_substring) { 
    std::cerr << "str.empty() " << strs.empty()
    << "str_capacity_ = " << str_capacity_ << " " << output_.writer().available_capacity() << "\n";
    if(strs.empty() ) 
      output_.writer().close(); 
    // final_index = next_index + data.size();
    // size_t end_index = first_index + data.size();
    // final_index_ = end_index;
    // if(strs.empty() && (*final_index_ == output_.reader().bytes_popped())) 
    //   output_.writer().close(); 
  }


}
// #include "reassembler.hh"
// #include <iostream>
// #include <algorithm>
// #include <optional>

// using namespace std;

// // Reassembler 的构造函数和成员变量
// // 假设 Reassembler 类有以下成员：
// // private:
// //     map<uint64_t, string> strs;
// //     uint64_t next_index = 0;
// //     size_t str_capacity_;
// //     ByteStream& output_;
// //     optional<uint64_t> final_index_ = nullopt;

// void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
//     // 1. 裁剪超出窗口的数据
//     uint64_t window_start = next_index;
//     uint64_t window_end = next_index + output_.writer().available_capacity();

//     // 如果数据完全超出窗口，直接返回
//     if (first_index >= window_end || first_index + data.size() <= window_start) {
//         if (is_last_substring) {
//             // 如果最后一个数据包完全在窗口外，并且窗口内没有数据，说明流结束
//             if (strs.empty() && next_index == final_index_.value_or(next_index)) {
//                 output_.writer().close();
//             }
//         }
//         return;
//     }

//     // 2. 裁剪重叠部分并更新数据
//     uint64_t data_start_in_window = max(first_index, window_start);
//     uint64_t data_end_in_window = min(first_index + data.size(), window_end);

//     string trimmed_data = data.substr(data_start_in_window - first_index, data_end_in_window - data_start_in_window);
//     uint64_t trimmed_first_index = data_start_in_window;
    
//     // 3. 记录 final_index_
//     if (is_last_substring) {
//         final_index_ = first_index + data.size();
//     }

//     // 4. 将数据合并到 map 中
//     // 检查是否与现有数据块重叠或连续
//     // 这里我们遍历 map，看看新数据是否能与已有的数据合并
//     auto it = strs.lower_bound(trimmed_first_index);
//     if (it != strs.begin()) {
//         auto prev_it = prev(it);
//         if (prev_it->first + prev_it->second.size() >= trimmed_first_index) {
//             // 新数据与前一个数据块重叠或连续，进行合并
//             trimmed_first_index = prev_it->first;
//             trimmed_data = prev_it->second.append(trimmed_data.substr(max((uint64_t)0, prev_it->first + prev_it->second.size() - data_start_in_window)));
//             strs.erase(prev_it);
//         }
//     }

//     while (it != strs.end() && it->first <= trimmed_first_index + trimmed_data.size()) {
//         // 新数据与后续数据块重叠或连续
//         if (it->first + it->second.size() > trimmed_first_index + trimmed_data.size()) {
//             trimmed_data.append(it->second.substr(trimmed_first_index + trimmed_data.size() - it->first));
//         }
//         it = strs.erase(it);
//     }
    
//     strs[trimmed_first_index] = trimmed_data;

//     // 5. 尝试将连续的数据推入输出流
//     push_and_trim();

//     // 6. 检查是否可以关闭
//     check_and_close();
// }

// void Reassembler::push_and_trim() {
//     while (!strs.empty()) {
//         auto it = strs.begin();
//         // 只有当第一个数据块的索引与 next_index 匹配时，才能推入
//         if (it->first != next_index) {
//             break;
//         }

//         size_t writable = min(it->second.size(), output_.writer().available_capacity());
//         if (writable == 0) {
//             break;
//         }

//         output_.writer().push(it->second.substr(0, writable));
//         next_index += writable;

//         if (writable < it->second.size()) {
//             // 只推入了一部分，更新 map 中的数据
//             string remaining_data = it->second.substr(writable);
//             strs.erase(it);
//             strs[next_index] = remaining_data;
//             break; // 推入后缓冲区可能满，退出循环
//         } else {
//             // 整个数据块都推入了
//             strs.erase(it);
//         }
//     }
// }

// void Reassembler::check_and_close() {
//     if (final_index_.has_value() && next_index == *final_index_) {
//         output_.writer().close();
//     }
// }
// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  // return {};
  uint64_t size = 0;
  for(auto it: strs) {
    size += it.second.size();
  }
  return size;
}
// // #include "reassembler.hh"
// // #include <iostream>
// // #include <algorithm>
// // #include <map>

// // using namespace std;

// // // This helper function merges two overlapping or adjacent fragments.
// // // It returns the merged data and updates the start and end indices.
// // string merge_fragments(uint64_t& new_start, uint64_t& new_end, uint64_t old_start, const string& old_data, uint64_t new_start_data, const string& new_data) {
// //     uint64_t old_end = old_start + old_data.size();
// //     uint64_t new_end_data = new_start_data + new_data.size();

// //     uint64_t merged_start = min(old_start, new_start_data);
// //     uint64_t merged_end = max(old_end, new_end_data);
// //     new_start = merged_start;
// //     new_end = merged_end;

// //     string result(merged_end - merged_start, 0);

// //     // Copy old data
// //     for (size_t i = 0; i < old_data.size(); ++i) {
// //         result[old_start - merged_start + i] = old_data[i];
// //     }
// //     // Copy new data
// //     for (size_t i = 0; i < new_data.size(); ++i) {
// //         result[new_start_data - merged_start + i] = new_data[i];
// //     }
// //     return result;
// // }

// // void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
// // {
// //     // 1. Check if the reassembler is already closed.
// //     if (output_.writer().is_closed()) {
// //         return;
// //     }

// //     // 2. Adjust input based on already pushed data.
// //     if (first_index < next_index) {
// //         uint64_t overlap = next_index - first_index;
// //         if (overlap >= data.size()) {
// //             if (is_last_substring) {
// //                 output_.writer().close();
// //             }
// //             return;
// //         }
// //         data = data.substr(overlap);
// //         first_index = next_index;
// //     }

// //     // 3. Trim data that falls outside the available buffer capacity.
// //     uint64_t available_capacity = output_.writer().available_capacity();
// //     uint64_t win_right = next_index + available_capacity;
// //     if (first_index >= win_right) {
// //         if (is_last_substring && first_index <= win_right) { // If the last segment fits exactly at the end
// //              output_.writer().close();
// //         }
// //         return;
// //     }
// //     if (first_index + data.size() > win_right) {
// //         data.resize(win_right - first_index);
// //     }
// //     if (data.empty()) {
// //         return;
// //     }

// //     // 4. Merge new fragment with existing ones.
// //     uint64_t new_start = first_index;
// //     uint64_t new_end = first_index + data.size();
    
// //     // Find fragments that overlap with the new one.
// //     // Use lower_bound to find the first fragment with a key >= new_start.
// //     auto it = strs.lower_bound(new_start);
// //     if (it != strs.begin()) {
// //         --it;
// //     }

// //     while (it != strs.end()) {
// //         uint64_t seg_start = it->first;
// //         uint64_t seg_end = it->first + it->second.size();
        
// //         // If the new segment is completely past the current stored segment,
// //         // we can break the loop.
// //         if (seg_start > new_end) {
// //             break;
// //         }

// //         // Check for overlap
// //         if (seg_end > new_start) {
// //             // Overlap detected. Merge the two segments.
// //             string merged_data = merge_fragments(new_start, new_end, seg_start, it->second, first_index, data);
// //             data = merged_data;
// //             it = strs.erase(it);
// //         } else {
// //             ++it;
// //         }
// //     }
    
// //     // Insert the merged fragment.
// //     strs[new_start] = data;

// //     // 5. Try to push continuous data to the output.
// //     while (true) {
// //         auto it_to_push = strs.find(next_index);
// //         if (it_to_push == strs.end()) {
// //             break; // No continuous data available
// //         }
        
// //         output_.writer().push(it_to_push->second);
// //         next_index += it_to_push->second.size();
// //         strs.erase(it_to_push);
// //     }

// //     // 6. Handle the `is_last_substring` flag.
// //     if (is_last_substring) {
// //         // uint64_t end_index = first_index + data.size();
// //         // if (next_index >= end_index) {
// //              output_.writer().close();
// //         // }
// //     }
// // }

// // // How many bytes are stored in the Reassembler itself?
// // uint64_t Reassembler::count_bytes_pending() const
// // {
// //     uint64_t size = 0;
// //     for (const auto& pair : strs) {
// //         size += pair.second.size();
// //     }
// //     return size;
// // }