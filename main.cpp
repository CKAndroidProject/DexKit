#include <iostream>
#include <fstream>
#include <chrono>
#include <stack>
#include <map>
#include <set>
#include "byte_code_util.h"
#include "dex_kit.h"

int main() {
    std::map<std::string_view, std::set<std::string_view>> obfuscate = {
            {"Lcom/tencent/mobileqq/activity/ChatActivityFacade;",               {"reSendEmo"}},
            {"Lcooperation/qzone/PlatformInfor;",                                {"52b7f2", "qimei"}},
            {"Lcom/tencent/mobileqq/troop/clockin/handler/TroopClockInHandler;", {"TroopClockInHandler"}},
            {"test",                                                             {"mark_uin_upload"}},
    };

    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    dexkit::DexKit dexKit("../dex/qq-8.9.2/qq-8.9.2.apk");

    // 返回混淆map中包含所有字符串的类
    auto res = dexKit.LocationClasses(obfuscate);
    for (auto &[key, value]: res) {
        std::cout << key << " -> \n";
        for (auto &v: value) {
            std::cout << "\t" << v << "\n";
        }
    }

    auto now1 = std::chrono::system_clock::now();
    auto now_ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(now1.time_since_epoch());
    std::cout << "used time: " << now_ms1.count() - now_ms.count() << " ms\n";
//    while (true) sleep(1);
    return 0;
}