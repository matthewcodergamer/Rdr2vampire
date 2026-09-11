#include <Windows.h>
#include <atomic>
#include <exception>
#include <main.h>
#include "nightwalker/core/Runtime.h"

namespace {
HMODULE g_moduleHandle = nullptr;
std::atomic_bool g_shutdownRequested{false};

void DebugFailure(const char* message) noexcept {
    ::OutputDebugStringA("[Nightwalker] ");
    ::OutputDebugStringA(message);
    ::OutputDebugStringA("\n");
}

void ScriptMain() {
    nightwalker::core::Runtime runtime;
    try {
        if (g_moduleHandle == nullptr || !runtime.Initialize(g_moduleHandle)) {
            DebugFailure("Runtime initialization failed.");
            return;
        }
        while (!g_shutdownRequested.load(std::memory_order_acquire)) {
            runtime.Tick();
            scriptWait(0);
        }
        runtime.Shutdown();
    } catch (const std::exception& error) {
        DebugFailure(error.what());
        if (runtime.IsInitialized()) {
            try { runtime.Shutdown(); } catch (...) { DebugFailure("Shutdown failed after an exception."); }
        }
    } catch (...) {
        DebugFailure("Unhandled exception in ScriptMain.");
        if (runtime.IsInitialized()) {
            try { runtime.Shutdown(); } catch (...) { DebugFailure("Shutdown failed after an exception."); }
        }
    }
}
}

BOOL APIENTRY DllMain(HMODULE moduleHandle, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            g_moduleHandle = moduleHandle;
            g_shutdownRequested.store(false, std::memory_order_release);
            ::DisableThreadLibraryCalls(moduleHandle);
            scriptRegister(moduleHandle, &ScriptMain);
            break;
        case DLL_PROCESS_DETACH:
            g_shutdownRequested.store(true, std::memory_order_release);
            scriptUnregister(moduleHandle);
            g_moduleHandle = nullptr;
            break;
        default:
            break;
    }
    return TRUE;
}
