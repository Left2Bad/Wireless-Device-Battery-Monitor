// Captured PM25 status and transport/filtering regressions, without hardware writes.
#include "../src/drivers/drivers.h"
#include <cassert>
#include <cstdio>

namespace {
std::vector<std::vector<BYTE>> reports;
int opens = 0, writes = 0;
bool writeOk = true;
HidColl Receiver() {
    HidColl c;
    c.path = L"test-receiver";
    c.vid = 0x1EA7; c.pid = 0x0064;
    c.caps.UsagePage = 0xFFB5; c.caps.Usage = 1;
    c.caps.InputReportByteLength = c.caps.OutputReportByteLength = 8;
    return c;
}
PollContext Poll(const HidColl& c, std::vector<std::vector<BYTE>> packets) {
    reports = std::move(packets);
    PollContext ctx; ctx.colls = { c }; PollWooking(ctx); return ctx;
}
}
bool HidDev::Open(const HidColl& c) { c_ = c; h_ = reinterpret_cast<HANDLE>(1); ++opens; return true; }
void HidDev::Close() { h_ = INVALID_HANDLE_VALUE; }
void HidDev::Flush() {}
bool HidDev::Write(const std::vector<BYTE>& data, int timeout) {
    assert(data == (std::vector<BYTE>{0xB5, 1, 0, 0, 0, 0, 0, 0}));
    assert(timeout > 0 && timeout <= 300); ++writes; return writeOk;
}
int HidDev::Read(std::vector<BYTE>& out, int timeout) {
    assert(timeout > 0 && timeout <= 500);
    if (reports.empty()) return -1;
    out = reports.front(); reports.erase(reports.begin()); return (int)out.size();
}
int main() {
    auto receiver = Receiver();
    const std::vector<BYTE> captured = {0xB5, 1, 1, 0x38, 0x62, 0x3C, 0, 0x72};
    auto ctx = Poll(receiver, {captured});
    assert(ctx.out.size() == 1 && ctx.out[0].percent == 56);
    assert(ctx.out[0].kind == DeviceKind::Mouse && ctx.out[0].name == L"Wooking PM25");
    assert(!ctx.out[0].charging && ctx.out[0].millivolts == 0);
    assert(ctx.IsClaimed(receiver.vid, receiver.pid));
    for (BYTE level : {0, 100}) {
        auto frame = captured; frame[3] = level;
        ctx = Poll(receiver, {frame}); assert(ctx.out.size() == 1 && ctx.out[0].percent == level);
    }
    for (int field : {0, 1, 2, 3}) {
        auto frame = captured; frame[field] = 0xFF;
        ctx = Poll(receiver, {frame}); assert(ctx.out.empty() && ctx.claimed.empty());
    }
    ctx = Poll(receiver, {{0xB5, 1, 1}, captured}); assert(ctx.out.size() == 1);
    ctx = Poll(receiver, {}); assert(ctx.out.empty() && ctx.claimed.empty());
    writeOk = false; ctx = Poll(receiver, {captured}); assert(ctx.out.empty()); writeOk = true;
    for (int field = 0; field < 7; ++field) {
        auto wrong = receiver;
        switch(field) {
        case 0: wrong.vid = 0x1234; break;
        case 1: wrong.pid = 0x5678; break;
        case 2: wrong.caps.UsagePage = 1; break;
        case 3: wrong.caps.Usage = 2; break;
        case 4: wrong.caps.InputReportByteLength = 9; break;
        case 5: wrong.caps.OutputReportByteLength = 9; break;
        case 6: wrong.caps.FeatureReportByteLength = 8; break;
        }
        opens = writes = 0; ctx = Poll(wrong, {captured});
        assert(opens == 0 && writes == 0 && ctx.out.empty());
    }
    opens = writes = 0;
    ctx = {}; ctx.colls = {receiver}; ctx.Claim(receiver.vid, receiver.pid); PollWooking(ctx);
    assert(opens == 0 && writes == 0);
    std::puts("Wooking tests passed: captured status, boundaries, invalid packets, timeout, transport and device filters.");
}
