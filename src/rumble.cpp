#include "living_village/rumble.hpp"
#include <algorithm>
#include <array>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
namespace village {
const RumbleIO& LinuxRumble::ops() const {
    static const RumbleIO system{
        [](const char* p,int f) { return ::open(p,f); },
        [](int fd,unsigned long request,void* data) { return request == EVIOCRMFF ? ::ioctl(fd,request,static_cast<int>(reinterpret_cast<std::intptr_t>(data))) : ::ioctl(fd,request,data); },
        [](int fd,const void* p,std::size_t n) -> std::ptrdiff_t { return ::write(fd,p,n); },
        [](int fd) { return ::close(fd); }
    };
    return io ? *io : system;
}
void LinuxRumble::discover() {
    searched = true;
    for (int index = 0; index < 64; ++index) {
        const auto path = "/dev/input/event" + std::to_string(index);
        const int candidate = ops().open_device(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (candidate < 0) continue;
        std::array<unsigned long, (FF_MAX + 1 + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)> bits{};
        if (ops().ioctl_device(candidate, EVIOCGBIT(EV_FF, sizeof(bits)), bits.data()) >= 0
            && (bits[FF_RUMBLE / (sizeof(unsigned long) * 8)] & (1UL << (FF_RUMBLE % (sizeof(unsigned long) * 8))))) {
            fd = candidate; char name[128]{}; ops().ioctl_device(fd, EVIOCGNAME(sizeof(name)), name);
            status = std::string("Linux FF: ") + name; return;
        }
        ops().close_device(candidate);
    }
}
LinuxRumble::~LinuxRumble() {
    if (fd >= 0) { play(0, 0); if (effect_id >= 0) ops().ioctl_device(fd, EVIOCRMFF, reinterpret_cast<void*>(static_cast<std::intptr_t>(effect_id))); ops().close_device(fd); }
}
bool LinuxRumble::play(std::uint16_t strength, std::uint32_t duration) {
    if (!searched && strength) discover();
    if (fd < 0) return false;
    if (!strength || !duration) {
        if (effect_id < 0) return true;
        input_event stop{}; stop.type = EV_FF; stop.code = static_cast<__u16>(effect_id);
        return ops().write_device(fd, &stop, sizeof(stop)) == sizeof(stop);
    }
    ff_effect effect{}; effect.type = FF_RUMBLE; effect.id = static_cast<__s16>(effect_id);
    effect.u.rumble.strong_magnitude = strength; effect.u.rumble.weak_magnitude = strength;
    effect.replay.length = static_cast<__u16>(std::min<std::uint32_t>(duration, 2000));
    if (ops().ioctl_device(fd, EVIOCSFF, &effect) < 0) { status = "Linux FF effect upload failed"; return false; }
    effect_id = effect.id;
    input_event start{}; start.type = EV_FF; start.code = static_cast<__u16>(effect_id); start.value = 1;
    if (ops().write_device(fd, &start, sizeof(start)) != sizeof(start)) { status = "Linux FF trigger failed"; return false; }
    return true;
}
}
