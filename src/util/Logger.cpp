#include "nightwalker/util/Logger.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <string>
namespace nightwalker::util { namespace {
const char* ToString(LogLevel level) noexcept { switch(level){ case LogLevel::Debug:return "DEBUG"; case LogLevel::Info:return "INFO"; case LogLevel::Warning:return "WARN"; case LogLevel::Error:return "ERROR"; default:return "UNKNOWN"; } }
std::string Timestamp(){ SYSTEMTIME v{}; ::GetLocalTime(&v); std::array<char,32>b{}; std::snprintf(b.data(),b.size(),"%04u-%02u-%02u %02u:%02u:%02u.%03u",(unsigned)v.wYear,(unsigned)v.wMonth,(unsigned)v.wDay,(unsigned)v.wHour,(unsigned)v.wMinute,(unsigned)v.wSecond,(unsigned)v.wMilliseconds); return b.data(); }
}
bool Logger::Initialize(const std::filesystem::path& p){ std::scoped_lock lock(mutex_); if(stream_.is_open())stream_.close(); stream_.clear(); stream_.open(p,std::ios::out|std::ios::app); return stream_.is_open(); }
void Logger::Shutdown() noexcept { try{ std::scoped_lock lock(mutex_); if(stream_.is_open()){stream_.flush();stream_.close();} }catch(...){ } }
void Logger::Write(LogLevel level,std::string_view message) noexcept { if((int)level<(int)minimumLevel_)return; try{ const std::string line="["+Timestamp()+"] ["+ToString(level)+"] "+std::string(message); ::OutputDebugStringA((line+"\n").c_str()); std::scoped_lock lock(mutex_); if(stream_.is_open()){stream_<<line<<'\n';stream_.flush();} }catch(...){ ::OutputDebugStringA("[Nightwalker] logger failure\n"); } }
}
