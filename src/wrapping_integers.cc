#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );

  return (zero_point + n % (1ULL << 32));
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    // printf("zero_point = %d checkpoint = %ld\n", zero_point.raw_value_, checkpoint);
    const uint64_t MOD = 1ULL << 32;

    uint64_t offset = ((uint64_t)this->raw_value_ + MOD - (uint64_t)zero_point.raw_value_) & (MOD - 1);
    // printf("offset = %ld\n", offset);
    uint64_t candidate = (checkpoint & ~(MOD - 1)) + offset;

    // 比较三个候选：同一圈、上一圈、下一圈
    uint64_t before = candidate - MOD;
    uint64_t after  = candidate + MOD;

    uint64_t diff_c = (candidate > checkpoint) ? candidate - checkpoint : checkpoint - candidate;
    uint64_t diff_b = (before > checkpoint) ? before - checkpoint : checkpoint - before;
    uint64_t diff_a = (after > checkpoint) ? after - checkpoint : checkpoint - after;

    uint64_t ans = candidate;
    if (diff_b < diff_c) ans = before;
    if (diff_a < diff_c && diff_a < diff_b) ans = after;

    return ans;
}
