#include "living_village/port_art.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
std::string read(const std::filesystem::path& p) { std::ifstream in(p); return {(std::istreambuf_iterator<char>(in)), {}}; }
void write(const std::filesystem::path& p, const std::string& s) { std::ofstream out(p); out << s; }
int main() {
    char tmp[] = "/tmp/lv-art-XXXXXX"; auto* dir = mkdtemp(tmp); if (!dir) return 1;
    const std::filesystem::path root(dir), list = root / "gamelist.xml";
    try {
        std::filesystem::create_directories(root / "livingvillage/images"); write(root / "LivingVillage.sh", "launcher"); write(root / "livingvillage/images/cover.png", "test image");
        const std::string other = "  <game id=\"123\"><path>./Other.sh</path><name>Other &amp; Port</name><image>./other.png</image><desc><![CDATA[A <game> tag]]></desc></game>\n";
        const std::string before = "<?xml version=\"1.0\"?><gameList>\n<!-- <game> ignore -->\n" + other + "</gameList>";
        write(list, before); register_port_art(root); const auto first = read(list);
        check(first.find(other) != std::string::npos && first.find("./livingvillage/images/cover.png") != std::string::npos, "Art registration lost unrelated metadata");
        check(read(root / "gamelist.xml.living-village.bak") == before, "Metadata backup was not the original");
        register_port_art(root); check(read(list) == first && read(root / "gamelist.xml.living-village.bak") == before, "Registration not idempotent");
        write(list, "<gameList><game><path>LivingVillage.sh</path><name>My Village</name><image>old.png</image><rating>0.8</rating></game></gameList>");
        register_port_art(root); check(read(list).find("<rating>0.8</rating>") != std::string::npos && read(list).find("<name>My Village</name>") != std::string::npos && read(list).find("old.png") == std::string::npos, "Existing entry fields were not preserved");
        for (const std::string bad : {"<gameList><game></gameList>", "<!DOCTYPE gameList><gameList></gameList>", "<gameList><game><path>./LivingVillage.sh</path></game><game><path>LivingVillage.sh</path></game></gameList>"}) {
            write(list, bad); bool rejected = false; try { register_port_art(root); } catch (...) { rejected = true; }
            check(rejected && read(list) == bad, "Invalid/ambiguous XML was overwritten");
        }
        std::filesystem::remove(list); register_port_art(root); check(read(list).find("./LivingVillage.sh") != std::string::npos, "Empty Ports install not registered");
        std::filesystem::remove(list); write(root / "protected.xml", "protected"); std::filesystem::create_symlink(root / "protected.xml", list);
        bool rejected = false; try { register_port_art(root); } catch (...) { rejected = true; }
        check(rejected && read(root / "protected.xml") == "protected", "Symlink metadata target changed");
        std::filesystem::remove_all(root); std::cout << "PASS: cover registration, preserved XML, idempotency, backup, malformed/duplicate/symlink rejection\n";
    } catch (const std::exception& e) { std::filesystem::remove_all(root); std::cerr << e.what() << '\n'; return 1; }
}
