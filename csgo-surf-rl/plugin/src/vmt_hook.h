// Tiny virtual-method-table hook helper: swap one vtable slot for our
// function and hand back the original as a trampoline. This is the standard,
// non-obfuscated way a Source plugin intercepts an engine interface method;
// it does nothing to hide itself from anything.
#ifndef SURF_VMT_HOOK_H
#define SURF_VMT_HOOK_H

#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace surf {

class VmtHook {
public:
    // Install a hook at vtable slot `index` of the object at `instance`,
    // redirecting to `detour`. Returns false if memory could not be made
    // writable. The original pointer is retained for Original().
    bool Install(void* instance, int index, void* detour) {
        if (!instance) return false;
        vtable_ = *reinterpret_cast<void***>(instance);
        index_ = index;
        original_ = vtable_[index];
        if (!MakeWritable(&vtable_[index], sizeof(void*))) return false;
        vtable_[index] = detour;
        installed_ = true;
        return true;
    }

    void Remove() {
        if (installed_) {
            vtable_[index_] = original_;
            installed_ = false;
        }
    }

    template <typename Fn>
    Fn Original() const {
        return reinterpret_cast<Fn>(original_);
    }

    ~VmtHook() { Remove(); }

private:
    static bool MakeWritable(void* addr, size_t size) {
#if defined(_WIN32)
        DWORD old;
        return VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old) != 0;
#else
        long page = sysconf(_SC_PAGESIZE);
        uintptr_t start = reinterpret_cast<uintptr_t>(addr) & ~(uintptr_t)(page - 1);
        uintptr_t end = reinterpret_cast<uintptr_t>(addr) + size;
        size_t len = end - start;
        return mprotect(reinterpret_cast<void*>(start), len,
                        PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
#endif
    }

    void** vtable_ = nullptr;
    int index_ = -1;
    void* original_ = nullptr;
    bool installed_ = false;
};

}  // namespace surf

#endif  // SURF_VMT_HOOK_H
