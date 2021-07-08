#include "TSCNS.h"

double TSCNS::init(double tsc_ghz) {
    syncTime(base_tsc, base_ns);
    if (tsc_ghz > 0) {
        tsc_ghz_inv = 1.0 / tsc_ghz;
        adjustOffset();
        return tsc_ghz;
    }
    else {
        return calibrate();
    }
}

double TSCNS::calibrate(uint64_t min_wait_ns) {
    uint64_t delayed_tsc, delayed_ns;
    do {
        syncTime(delayed_tsc, delayed_ns);
    } while ((delayed_ns - base_ns) < min_wait_ns);
    tsc_ghz_inv = (double)(int64_t)(delayed_ns - base_ns) / (int64_t)(delayed_tsc - base_tsc);
    adjustOffset();
    return 1.0 / tsc_ghz_inv;
}

uint64_t TSCNS::rdtsc() {
    return __builtin_ia32_rdtsc();
}

uint64_t TSCNS::tsc2ns(uint64_t tsc) const {
    return ns_offset + (int64_t)((int64_t)tsc * tsc_ghz_inv);
}

uint64_t TSCNS::rdns() const {
    return tsc2ns(rdtsc());
}

uint64_t TSCNS::rdsysns() {
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

uint64_t TSCNS::rdoffset() const {
    return ns_offset;
}

void TSCNS::syncTime(uint64_t& tsc, uint64_t& ns)  {
    const int N = 10;
    uint64_t tscs[N + 1];
    uint64_t nses[N + 1];

    tscs[0] = rdtsc();
    for (int i = 1; i <= N; i++) {
        nses[i] = rdsysns();
        tscs[i] = rdtsc();
    }

    int best = 1;
    for (int i = 2; i <= N; i++) {
        if (tscs[i] - tscs[i - 1] < tscs[best] - tscs[best - 1]) best = i;
    }
    tsc = (tscs[best] + tscs[best - 1]) >> 1;
    ns = nses[best];
}

void TSCNS::adjustOffset() {
    ns_offset = base_ns - (int64_t)((int64_t)base_tsc * tsc_ghz_inv);
}

