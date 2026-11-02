#ifndef TAPE_LOADER_H
#define TAPE_LOADER_H

#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "tape.h"

// Read compressed media directly into memory; archive paths are never extracted.
namespace TapeFiles {
constexpr size_t MaxBytes = 16 * 1024 * 1024;
inline bool Extension(const std::string &path,const std::string &extension) {
  if(path.size()<extension.size()) return false;
  std::string suffix=path.substr(path.size()-extension.size());
  std::transform(suffix.begin(),suffix.end(),suffix.begin(),[](unsigned char c){return std::tolower(c);});
  return suffix==extension;
}
inline bool Read(const std::string &path,std::vector<unsigned char> &out,std::string &error) {
  std::vector<unsigned char> bytes;
  if(Extension(path,".zip")) {
    std::unique_ptr<archive,decltype(&archive_read_free)> input(archive_read_new(),archive_read_free);
    if(!input) {error="Cannot allocate ZIP reader";return false;}
    archive_read_support_format_zip(input.get());
    auto fail=[&](const std::string &message) {
      const char *detail=archive_error_string(input.get());
      error=message+(detail?": "+std::string(detail):"");return false;
    };
    if(archive_read_open_filename(input.get(),path.c_str(),16384)!=ARCHIVE_OK) return fail("Cannot open ZIP file");
    bool found=false;archive_entry *entry=nullptr;int status;
    unsigned count=0;
    while((status=archive_read_next_header(input.get(),&entry))!=ARCHIVE_EOF) {
      if(status!=ARCHIVE_OK) return fail("Cannot read ZIP entry");
      if(++count>4096) {error="ZIP contains too many entries";return false;}
      const char *name=archive_entry_pathname(entry);
      if(!name || archive_entry_filetype(entry)!=AE_IFREG || !Extension(name,".tap")) {
        if(archive_read_data_skip(input.get())!=ARCHIVE_OK) return fail("Cannot skip ZIP entry");
        continue;
      }
      if(archive_entry_is_encrypted(entry)>0) {error="Encrypted TAP entries are not supported";return false;}
      if(archive_entry_size_is_set(entry) && (archive_entry_size(entry)<0 ||
          static_cast<unsigned long long>(archive_entry_size(entry))>MaxBytes)) {
        error="TAP inside ZIP exceeds the 16 MiB limit";return false;
      }
      unsigned char buffer[16384];la_ssize_t size;
      while((size=archive_read_data(input.get(),buffer,sizeof(buffer)))>0) {
        if(static_cast<size_t>(size)>MaxBytes-bytes.size()) {error="TAP inside ZIP exceeds the 16 MiB limit";return false;}
        bytes.insert(bytes.end(),buffer,buffer+size);
      }
      if(size<0) return fail("Cannot decompress TAP entry");
      found=true;break; // First TAP in archive order; sidecar files are ignored.
    }
    if(!found) {error="ZIP contains no TAP files";return false;}
  } else {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input) {error="Cannot open TAP file: "+path;return false;}
    auto size=input.tellg();
    if(size<0 || size>std::streamoff(MaxBytes)) {error="TAP exceeds the 16 MiB limit";return false;}
    bytes.resize(static_cast<size_t>(size));input.seekg(0);
    if(!bytes.empty()) input.read(reinterpret_cast<char *>(bytes.data()),bytes.size());
    if(!input) {error="Cannot read TAP file: "+path;return false;}
  }
  if(bytes.empty() || !ValidateTape(bytes)) {error="Invalid or empty TAP file: "+path;return false;}
  out.swap(bytes);error.clear();return true;
}
}
#endif
