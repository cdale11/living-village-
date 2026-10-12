#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
namespace village {
struct RumbleIO {
    int (*open_device)(const char*,int);
    int (*ioctl_device)(int,unsigned long,void*);
    std::ptrdiff_t (*write_device)(int,const void*,std::size_t);
    int (*close_device)(int);
};
class LinuxRumble {
public:
    explicit LinuxRumble(const RumbleIO* operations = nullptr) : io(operations) {}
    LinuxRumble(const LinuxRumble&) = delete;
    LinuxRumble& operator=(const LinuxRumble&) = delete;
    ~LinuxRumble();
    bool play(std::uint16_t strength, std::uint32_t duration);
    std::string status = "No supported force-feedback device";
private:
    int fd = -1, effect_id = -1;
    bool searched = false;
    const RumbleIO* io;
    const RumbleIO& ops() const;
    void discover();
};
}
