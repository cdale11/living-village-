# Third-party headers

`SDL2/` contains unmodified public headers from SDL **2.30.11**, downloaded from
<https://www.libsdl.org/release/SDL2-2.30.11.tar.gz>. The zlib license is retained
as `SDL2/LICENSE.txt`. Only headers are vendored; the OS provides SDL2 at runtime.
All game art and bitmap lettering are original procedural graphics.

`licenses/` retains the LLVM Apache 2.0 license with exceptions for libc++/libc++abi
linked into the ARM64 binary, and the Zig MIT license for its compiler runtime.
The development compiler itself is not shipped.
