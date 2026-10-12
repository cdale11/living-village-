#include "living_village/port_art.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace village {
namespace {
struct File {
    int fd;
    ~File() { if (fd >= 0) ::close(fd); }
};
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read(const std::filesystem::path& path) {
    require(!std::filesystem::is_symlink(path), "Refusing symlinked gamelist");
    require(std::filesystem::file_size(path) <= 4 * 1024 * 1024, "Gamelist exceeds 4 MiB");
    std::ifstream in(path, std::ios::binary); require(bool(in), "Cannot read gamelist");
    return {(std::istreambuf_iterator<char>(in)), {}};
}
void write_atomic(const std::filesystem::path& path, const std::string& data) {
    const auto temporary = path.string() + ".living-village.tmp";
    File file{::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644)};
    require(file.fd >= 0, "Cannot create metadata temporary file; check for an interrupted registration");
    try {
        std::size_t offset = 0;
        while (offset < data.size()) {
            const auto n = ::write(file.fd, data.data() + offset, data.size() - offset);
            if (n < 0 && errno == EINTR) continue;
            require(n > 0, "Cannot write frontend metadata"); offset += static_cast<std::size_t>(n);
        }
        require(::fsync(file.fd) == 0, "Cannot flush frontend metadata");
        require(!std::filesystem::is_symlink(path), "Refusing symlinked metadata target");
        std::filesystem::rename(temporary, path);
        File directory{::open(path.parent_path().c_str(), O_RDONLY | O_DIRECTORY)};
        require(directory.fd >= 0 && ::fsync(directory.fd) == 0, "Cannot flush frontend directory");
    } catch (...) { ::unlink(temporary.c_str()); throw; }
}
struct Element { std::string name; std::size_t start, content, finish; int parent; };
// Bounded structural scan: preserve the original XML verbatim, changing only
// our entry. Quotes, comments, processing instructions and CDATA are skipped.
std::vector<Element> scan(const std::string& xml) {
    std::vector<Element> elements; std::vector<int> stack;
    std::size_t cursor = 0;
    while ((cursor = xml.find('<', cursor)) != std::string::npos) {
        auto skip = [&](const char* closing, std::size_t prefix) {
            auto end = xml.find(closing, cursor + prefix); require(end != std::string::npos, "Unterminated XML section");
            cursor = end + std::strlen(closing);
        };
        if (xml.compare(cursor, 4, "<!--") == 0) { skip("-->", 4); continue; }
        if (xml.compare(cursor, 9, "<![CDATA[") == 0) { skip("]]>", 9); continue; }
        if (xml.compare(cursor, 2, "<?") == 0) { skip("?>", 2); continue; }
        require(xml.compare(cursor, 2, "<!") != 0, "Unsupported XML declaration; metadata left unchanged");
        const auto begin = cursor; bool closing = xml.compare(cursor, 2, "</") == 0;
        cursor += closing ? 2 : 1;
        const auto name_start = cursor;
        while (cursor < xml.size() && (std::isalnum(static_cast<unsigned char>(xml[cursor])) || xml[cursor] == '_' || xml[cursor] == '-' || xml[cursor] == ':')) ++cursor;
        require(cursor > name_start, "Malformed XML element"); const auto name = xml.substr(name_start, cursor - name_start);
        char quote = 0;
        for (; cursor < xml.size(); ++cursor) {
            const char c = xml[cursor]; if (quote) { if (c == quote) quote = 0; }
            else if (c == '\'' || c == '"') quote = c;
            else if (c == '>') break;
        }
        require(cursor < xml.size(), "Unterminated XML tag");
        const bool empty = cursor && xml[cursor - 1] == '/'; ++cursor;
        if (closing) {
            require(!stack.empty() && elements[stack.back()].name == name, "Unbalanced gamelist XML");
            elements[stack.back()].finish = cursor; stack.pop_back();
        } else {
            require(elements.size() < 100000 && stack.size() < 32, "Gamelist XML exceeds structural limits");
            if (stack.empty()) require(elements.empty(), "Multiple gamelist roots");
            elements.push_back({name, begin, cursor, empty ? cursor : 0, stack.empty() ? -1 : stack.back()});
            if (!empty) stack.push_back(static_cast<int>(elements.size() - 1));
        }
    }
    require(stack.empty() && !elements.empty() && elements.front().name == "gameList", "Missing or incomplete gameList root");
    return elements;
}
std::string value(const std::string& xml, const Element& e) {
    auto end = xml.rfind("</", e.finish - 1); require(end != std::string::npos && end >= e.content, "Invalid element value");
    auto result = xml.substr(e.content, end - e.content);
    auto first = result.find_first_not_of(" \t\r\n"), last = result.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : result.substr(first, last - first + 1);
}
}
void register_port_art(const std::filesystem::path& ports) {
    require(std::filesystem::is_directory(ports), "Ports directory does not exist");
    require(std::filesystem::is_regular_file(ports / "LivingVillage.sh") && std::filesystem::is_regular_file(ports / "images/LivingVillage-image.png"), "Install launcher and cover before registration");
    File lock{::open((ports / ".living-village-art.lock").c_str(), O_WRONLY | O_CREAT | O_NOFOLLOW, 0600)};
    require(lock.fd >= 0 && ::flock(lock.fd, LOCK_EX | LOCK_NB) == 0, "Frontend registration already running or lock unavailable");
    const auto path = ports / "gamelist.xml";
    const bool exists = std::filesystem::exists(path) || std::filesystem::is_symlink(path);
    const std::string before = exists ? read(path) : "<?xml version=\"1.0\"?>\n<gameList>\n</gameList>\n";
    const auto elements = scan(before);
    int game = -1, image = -1, name = -1;
    for (std::size_t i = 1; i < elements.size(); ++i) {
        const auto& e = elements[i];
        if (e.name != "game" || e.parent != 0) continue;
        for (std::size_t j = i + 1; j < elements.size() && elements[j].start < e.finish; ++j)
            if (elements[j].parent == static_cast<int>(i) && elements[j].name == "path") {
                const auto p = value(before, elements[j]);
                if (p == "./LivingVillage.sh" || p == "LivingVillage.sh" || p == (ports / "LivingVillage.sh").string()) {
                    require(game < 0, "Duplicate Living Village entries; gamelist left unchanged"); game = static_cast<int>(i);
                }
            }
    }
    std::string after = before;
    const std::string art = "<image>./images/LivingVillage-image.png</image>";
    if (game >= 0) {
        for (std::size_t i = game + 1; i < elements.size() && elements[i].start < elements[game].finish; ++i) if (elements[i].parent == game) {
            if (elements[i].name == "image") { require(image < 0, "Duplicate image fields"); image = static_cast<int>(i); }
            if (elements[i].name == "name") name = static_cast<int>(i);
        }
        // Insert fields before replacing an earlier field, so offsets stay valid.
        auto end = after.rfind("</game", elements[game].finish - 1);
        std::string added = name < 0 ? "\n    <name>Living Village</name>" : "";
        if (image < 0) added += "\n    " + art;
        after.insert(end, added);
        if (image >= 0) after.replace(elements[image].start, elements[image].finish - elements[image].start, art);
    } else {
        const auto end = after.rfind("</gameList"); require(end != std::string::npos, "Self-closing gameList cannot be registered");
        after.insert(end, "  <game>\n    <path>./LivingVillage.sh</path>\n    <name>Living Village</name>\n    " + art + "\n  </game>\n");
    }
    scan(after);
    if (after == before) { std::cout << "Ports cover already registered\n"; return; }
    if (exists) write_atomic(ports / "gamelist.xml.living-village.bak", before);
    write_atomic(path, after);
    std::cout << "Ports cover registered. Refresh the EmulationStation game list to display it.\n";
}
}
