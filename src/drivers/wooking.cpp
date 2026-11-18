// Wooking PM25, tested with receiver 1EA7:0064, product "2.4G Mouse".
// Vendor collection FFB5:0001, report B5, 8-byte input/output, no features.
// Status query: B5 01 00 00 00 00 00 00; reply: B5 01 01 <percent> ...
// Related protocol research (different receiver, used as a starting point):
// https://github.com/ksanjeev284/acer-popgo-companion/blob/main/docs/PROTOCOL_MAP.md
// The remaining bytes and charging state are not decoded for this model.
#include "drivers.h"

namespace {
bool QueryBattery(HidDev& d, int& percent) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        d.Flush();
        if (!d.Write({ 0xB5, 0x01, 0, 0, 0, 0, 0, 0 }, 300)) return false;
        const ULONGLONG end = GetTickCount64() + 500;
        while (GetTickCount64() < end) {
            std::vector<BYTE> reply;
            const ULONGLONG now = GetTickCount64();
            if (now >= end) break;
            if (d.Read(reply, (int)(end - now)) < 0) break;
            if (reply.size() != 8 || reply[0] != 0xB5 || reply[1] != 0x01 || reply[2] != 0x01) continue;
            // Reject unknown/sentinel values instead of clamping them to a fabricated 100%.
            if (reply[3] > 100) continue;
            percent = reply[3];
            return true;
        }
    }
    return false;
}
}  // namespace

void PollWooking(PollContext& ctx) {
    for (const auto& c : ctx.colls) {
        // This USB ID is shared by other OEM mice. Never probe unrelated collections/products.
        if (c.vid != 0x1EA7 || c.pid != 0x0064 || ctx.IsClaimed(c.vid, c.pid) ||
            c.page() != 0xFFB5 || c.usage() != 0x0001 || c.inLen() != 8 || c.outLen() != 8 || c.featLen() != 0)
            continue;
        HidDev d;
        if (!d.Open(c)) continue;
        int percent = -1;
        if (!QueryBattery(d, percent)) continue;
        DeviceStatus s;
        s.key = L"wooking:" + c.path;
        s.name = L"Wooking PM25";
        s.via = L"2.4 GHz receiver";
        s.kind = DeviceKind::Mouse;
        s.percent = percent;
        // The MCU's percentage is used directly. No inferred voltage or charging indicator.
        ctx.Add(s, c.vid, c.pid);
    }
}
