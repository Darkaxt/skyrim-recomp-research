#include "native_slice.h"
#include "lookup.h"
#include <cstring>
#include <memory>
#include <utility>
#include <iostream>

struct Guard {
    unsigned char* reservation;
    unsigned char* page;
    Guard() {
        reservation = static_cast<unsigned char*>(VirtualAlloc(nullptr, 12288, MEM_RESERVE, PAGE_NOACCESS));
        require(reservation, "Guard reserve failed");
        page = static_cast<unsigned char*>(VirtualAlloc(reservation + 4096, 4096, MEM_COMMIT, PAGE_READWRITE));
        require(page, "Guard commit failed");
    }
    ~Guard() { VirtualFree(reservation, 0, MEM_RELEASE); }
    char* put(const std::string& value, bool end) {
        require(value.size() < 4096, "String too long");
        auto p = reinterpret_cast<char*>(page + (end ? 4095 - value.size() : 0));
        std::memcpy(p, value.c_str(), value.size() + 1); return p;
    }
};
struct Info { unsigned char padding[0x50]; const char* name; };
static_assert(offsetof(Info, name) == 0x50);
std::vector<std::pair<const char*, const char*>> trace;
StringCompare real_compare;
int compare(const char* a, const char* b) { trace.emplace_back(a,b); return real_compare(a,b); }
std::uint32_t incorrect(const char*) { return 164; }

int main(int argc, char** argv) {
    try {
        require(argc >= 2, "Pack required");
        NativeSlice slice(argv[1]); automatic_set_base(slice.base);
        auto original = reinterpret_cast<std::uint32_t (*)(const char*)>(slice.base + 0x443320);
        auto crt = LoadLibraryW(L"api-ms-win-crt-string-l1-1-0.dll");
        require(crt, "CRT API set unavailable");
        real_compare = reinterpret_cast<StringCompare>(GetProcAddress(crt, "_stricmp"));
        require(real_compare, "CRT _stricmp unavailable");
        lookup_compare = compare;
        *reinterpret_cast<StringCompare*>(slice.base + 0x17C91E8) = compare;
        auto candidate = argc == 3 ? incorrect : automatic_lookup;
        Guard table_guard, query_guard;
        std::array<std::unique_ptr<Guard>,164> names;
        for (auto& n : names) n = std::make_unique<Guard>();
        std::uint64_t trials = 0;
        for (unsigned style = 0; style < 8; ++style) {
            auto table = reinterpret_cast<void**>(table_guard.page + (style & 1 ? 4096 - 165*8 : 0));
            // 164 metadata records use normal owned storage; the table and all
            // individual strings have inaccessible adjacent pages.
            std::array<Info,164> info{};
            std::array<std::string,164> text;
            table[0] = nullptr;
            for (unsigned i = 0; i < 164; ++i) {
                text[i] = "Value_" + std::to_string(i) + std::string(style * 17, char('a' + i%26));
                if (style == 6 && i == 0) text[i] = "";
                if (style == 7 && i == 163) text[i] = "DuPlicate";
                if (style == 7 && i == 37) text[i] = "duplicate";
                info[i].name = names[i]->put(text[i], (style+i)&1);
                table[i+1] = &info[i];
            }
            *reinterpret_cast<void***>(slice.base + 0x219DEC8) = table;
            for (unsigned selected = 0; selected < 167; ++selected) {
                auto wanted = selected < 164 ? text[selected] : std::string("No_such_value_") + std::to_string(selected);
                if (style & 2) for (char& ch : wanted) if (ch >= 'a' && ch <= 'z') ch -= 'a'-'A';
                const auto query = query_guard.put(wanted, selected&1);
                const auto expected = selected >= 164 ? 164u : style == 7 && selected == 163 ? 37u : selected;
                std::array<unsigned char,4096> table_before{}, query_before{};
                std::memcpy(table_before.data(), table_guard.page,4096);
                std::memcpy(query_before.data(), query_guard.page,4096);
                auto info_before = info;
                trace.clear(); auto a = original(query); auto calls = trace;
                trace.clear(); auto b = candidate(query);
                if (a != expected || b != a || trace != calls || calls.size() != (expected == 164 ? 164 : expected+1)) {
                    std::cerr << "Lookup differential failure style=" << style << " selected=" << selected
                        << " result=" << a << "/" << b << " calls=" << calls.size() << "/" << trace.size() << "\n"; return 1;
                }
                for (unsigned i=0;i<calls.size();++i)
                    require(calls[i] == std::pair(info[i].name, static_cast<const char*>(query)), "Comparison order/arguments changed");
                require(!std::memcmp(table_before.data(),table_guard.page,4096)
                        && !std::memcmp(query_before.data(),query_guard.page,4096)
                        && !std::memcmp(info_before.data(),info.data(),sizeof(info)), "Lookup changed fixture memory");
                for (unsigned i=0;i<164;++i) require(!std::memcmp(info[i].name,text[i].c_str(),text[i].size()+1),"Lookup changed a name");
                ++trials;
            }
        }
        FreeLibrary(crt);
        std::cout << "{\"status\":\"pass\",\"trials\":" << trials << ",\"table_entries\":164,\"exact_trace\":true,\"guard_pages\":true,\"real_crt\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 2; }
}
