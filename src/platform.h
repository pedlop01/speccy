#ifndef PLATFORM_H
#define PLATFORM_H
#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <allegro5/allegro.h>
#include <cstdio>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>
#include <atomic>
#include <random>
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#endif

namespace Platform {
#ifdef _WIN32
inline std::wstring Wide(const std::string &text) {
  int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.c_str(),-1,nullptr,0);
  if(!count)return {};
  std::vector<wchar_t> buffer(count);
  if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.c_str(),-1,buffer.data(),count))return {};
  return buffer.data();
}
#endif
// Exclusive creation, in the destination directory, also for suffixed ZIPs.
inline int Temporary(char *pattern,int suffix=0) {
#ifdef _WIN32
  size_t length=std::strlen(pattern);
  if(suffix<0 || length<size_t(suffix)+6)return -1;
  size_t start=length-suffix-6;
  if(std::string(pattern+start,6)!="XXXXXX")return -1;
  static std::atomic<unsigned> sequence{0};
  unsigned random=GetTickCount()^GetCurrentProcessId()^++sequence;
  try{random^=std::random_device{}();}catch(...){}
  for(unsigned attempt=0;attempt<128;++attempt) {
    unsigned value=random+attempt*2654435761u;
    for(unsigned i=0;i<6;++i){pattern[start+i]="0123456789abcdefghijklmnopqrstuvwxyz"[value%36];value/=36;}
    auto path=Wide(pattern);if(path.empty())return -1;
    int fd=_wopen(path.c_str(),_O_CREAT|_O_EXCL|_O_RDWR|_O_BINARY,_S_IREAD|_S_IWRITE);
    if(fd>=0)return fd;
  }
  return -1;
#else
  return suffix?mkstemps(pattern,suffix):mkstemp(pattern);
#endif
}
inline int Sync(int fd) {
#ifdef _WIN32
  return _commit(fd);
#else
  return fsync(fd);
#endif
}
inline int Close(int fd) {
#ifdef _WIN32
  return _close(fd);
#else
  return close(fd);
#endif
}
inline FILE *File(int fd,const char *mode) {
#ifdef _WIN32
  return _fdopen(fd,mode);
#else
  return fdopen(fd,mode);
#endif
}
inline int Descriptor(FILE *file) {
#ifdef _WIN32
  return _fileno(file);
#else
  return fileno(file);
#endif
}
// Windows CRT rename cannot replace an existing destination. Keep the previous
// save/settings/cache file until the new file has been written and closed.
inline int Replace(const std::string &source,const std::string &destination) {
#ifdef _WIN32
  auto from=Wide(source),to=Wide(destination);
  if(from.empty() || to.empty())return -1;
  return MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)?0:-1;
#else
  return std::rename(source.c_str(),destination.c_str());
#endif
}
inline int RemoveDirectory(const char *path) {
#ifdef _WIN32
  auto wide=Wide(path);return wide.empty()?-1:_wrmdir(wide.c_str());
#else
  return rmdir(path);
#endif
}
inline bool Writable(const std::string &path) {
#ifdef _WIN32
  auto wide=Wide(path);return !wide.empty() && _waccess(wide.c_str(),2)==0;
#else
  return access(path.c_str(),W_OK)==0;
#endif
}
inline std::string Executable() {
  ALLEGRO_PATH *path=al_get_standard_path(ALLEGRO_EXENAME_PATH);
  if(!path)return "";
  std::string result=al_path_cstr(path,ALLEGRO_NATIVE_PATH_SEP);al_destroy_path(path);return result;
}
inline bool OpenURL(const std::string &url) {
#ifdef _WIN32
  auto text=Wide(url);return !text.empty() && reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"open",text.c_str(),nullptr,nullptr,SW_SHOWNORMAL))>32;
#else
  pid_t pid;char command[]="xdg-open";char *arguments[]={command,const_cast<char *>(url.c_str()),nullptr};
  int result=posix_spawnp(&pid,command,nullptr,nullptr,arguments,environ);
  if(result)return false;
  std::thread([pid](){int status;while(waitpid(pid,&status,0)<0 && errno==EINTR){};}).detach();return true;
#endif
}
inline bool LocalTime(const std::time_t *stamp,std::tm *result) {
#ifdef _WIN32
  return localtime_s(result,stamp)==0;
#else
  return localtime_r(stamp,result)!=nullptr;
#endif
}
inline bool Absolute(const std::string &path) {
  return !path.empty() && (path[0]=='/' || path[0]=='\\' ||
      (path.size()>=3 && path[1]==':' && (path[2]=='/' || path[2]=='\\')));
}
}
#endif
