#pragma once

#include <cstddef>
#include <cstdint>

using HMODULE = void*;
using DWORD = std::uint32_t;

DWORD GetModuleFileNameW(HMODULE module, wchar_t* buffer, DWORD size);
short GetAsyncKeyState(int key);
