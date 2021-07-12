#pragma once
#include <time.h>
#include <cstdint>

class TSCNS
{
public:
  // If you haven't calibrated tsc_ghz on this machine, set tsc_ghz as 0.0 and it will auto wait 10 ms and calibrate.
  // Of course you can calibrate again later(e.g. after system init is done) and the longer you wait the more precise
  // tsc_ghz calibrate can get. It's a good idea that user waits as long as possible(more than 1 min) once, and save the
  // resultant tsc_ghz returned from calibrate() somewhere(e.g. config file) on this machine for future use. Or you can
  // cheat, see README and cheat.cc for details.
  //
  // If you have calibrated/cheated before on this machine as above, set tsc_ghz and skip calibration.
  //
  // One more thing: you can re-init and calibrate TSCNS at later times if you want to re-sync with
  // system time in case of NTP or manual time changes.
  double init(double tsc_ghz = 0.0);

  double calibrate(uint64_t min_wait_ns = 10000000);

  static uint64_t rdtsc();

  uint64_t tsc2ns(uint64_t tsc) const;

  uint64_t rdns() const;

  // If you want cross-platform, use std::chrono as below which incurs one more function call:
  // return std::chrono::high_resolution_clock::now().time_since_epoch().count();
  static uint64_t rdsysns();

  // For checking purposes, see test.cc
  uint64_t rdoffset() const;

private:
  // Linux kernel sync time by finding the first try with tsc diff < 50000
  // We do better: we find the try with the mininum tsc diff
  void syncTime(uint64_t& tsc, uint64_t& ns);

  void adjustOffset();

  alignas(64) double tsc_ghz_inv = 1.0; // make sure tsc_ghz_inv and ns_offset are on the same cache line
  uint64_t ns_offset = 0;
  uint64_t base_tsc = 0;
  uint64_t base_ns = 0;
};
