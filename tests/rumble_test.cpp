#include "living_village/rumble.hpp"
#include <array>
#include <cstdarg>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
namespace { int uploads=0,starts=0,stops=0,releases=0,closes=0; }
int fake_open(const char* path,int) { return std::strcmp(path,"/dev/input/event3")==0 ? 103 : -1; }
int fake_close(int fd) { if(fd!=103) return -1; ++closes;return 0; }
std::ptrdiff_t fake_write(int fd,const void* data,std::size_t size) {
    if(fd!=103) return -1;
    const auto& event=*static_cast<const input_event*>(data);
    if(size!=sizeof(input_event)||event.type!=EV_FF||event.code!=7) return -1;
    if(event.value==1) ++starts; else if(event.value==0) ++stops; else return -1;
    return static_cast<std::ptrdiff_t>(size);
}
int fake_ioctl(int fd,unsigned long request,void* data) {
    if(fd!=103) return -1;
    if(request==EVIOCRMFF) { if(reinterpret_cast<std::intptr_t>(data)==7) { ++releases;return 0; } return -1; }
    if(_IOC_NR(request)==0x20+EV_FF) { auto* bits=static_cast<unsigned long*>(data); bits[FF_RUMBLE/(sizeof(unsigned long)*8)]|=1UL<<(FF_RUMBLE%(sizeof(unsigned long)*8));return 0; }
    if(_IOC_NR(request)==0x06) { std::strcpy(static_cast<char*>(data),"gpio-vibrator");return 0; }
    if(request==EVIOCSFF) { auto* effect=static_cast<ff_effect*>(data); if(effect->type!=FF_RUMBLE||effect->replay.length!=2000||effect->u.rumble.strong_magnitude!=42000) return -1;effect->id=7;++uploads;return 0; }
    return -1;
}
int main() {
    const village::RumbleIO io{fake_open,fake_ioctl,fake_write,fake_close};
    { village::LinuxRumble rumble(&io); if(!rumble.play(42000,9000)||rumble.status.find("gpio-vibrator")==std::string::npos||!rumble.play(0,0)) return 1; }
    if(uploads!=1||starts!=1||stops!=2||releases!=1||closes!=1) return 1;
    std::cout<<"PASS: Linux FF device discovery, bounded effect upload/start/stop/removal and descriptor cleanup (simulated kernel)\n";
}
