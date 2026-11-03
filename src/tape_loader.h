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
inline bool Supported(const std::string &path) {
  return Extension(path,".tap") || Extension(path,".tzx") || Extension(path,".sna") || Extension(path,".z80") || Extension(path,".szx") || Extension(path,".slt") || Extension(path,".dsk");
}
inline std::string Normalize(std::string path) {
  std::replace(path.begin(),path.end(),'\\','/');std::vector<std::string> parts;size_t start=0;
  while(start<path.size()){size_t end=path.find('/',start);if(end==std::string::npos)end=path.size();auto part=path.substr(start,end-start);
    if(part==".."){if(!parts.empty())parts.pop_back();}else if(!part.empty() && part!=".")parts.push_back(part);start=end+1;}
  std::string out;for(const auto &part:parts){if(!out.empty())out+='/';out+=part;}return out;
}
inline bool ReadMedia(const std::string &path,std::vector<unsigned char> &out,std::string &name,std::string &error,const std::string &requested="") {
  std::vector<unsigned char> bytes;std::string chosen=path;
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
      const char *entryName=archive_entry_pathname(entry);
      if(!entryName || archive_entry_filetype(entry)!=AE_IFREG || (!Supported(entryName) || (!requested.empty() && Normalize(entryName)!=Normalize(requested)))) {
        if(archive_read_data_skip(input.get())!=ARCHIVE_OK) return fail("Cannot skip ZIP entry");
        continue;
      }
      if(archive_entry_is_encrypted(entry)>0) {error="Encrypted media entries are not supported";return false;}
      if(archive_entry_size_is_set(entry) && (archive_entry_size(entry)<0 ||
          static_cast<unsigned long long>(archive_entry_size(entry))>MaxBytes)) {
        error="Media inside ZIP exceeds the 16 MiB limit";return false;
      }
      unsigned char buffer[16384];la_ssize_t size;
      while((size=archive_read_data(input.get(),buffer,sizeof(buffer)))>0) {
        if(static_cast<size_t>(size)>MaxBytes-bytes.size()) {error="Media inside ZIP exceeds the 16 MiB limit";return false;}
        bytes.insert(bytes.end(),buffer,buffer+size);
      }
      if(size<0) return fail("Cannot decompress media entry");
      chosen=entryName;found=true;break; // First supported entry in archive order.
    }
    if(!found) {error="ZIP contains no matching supported media";return false;}
  } else {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input) {error="Cannot open media file: "+path;return false;}
    auto size=input.tellg();
    if(size<0 || size>std::streamoff(MaxBytes)) {error="Media exceeds the 16 MiB limit";return false;}
    bytes.resize(static_cast<size_t>(size));input.seekg(0);
    if(!bytes.empty()) input.read(reinterpret_cast<char *>(bytes.data()),bytes.size());
    if(!input) {error="Cannot read media file: "+path;return false;}
  }
  if(bytes.empty()) {error="Empty media file: "+path;return false;}
  out.swap(bytes);name=chosen;error.clear();return true;
}
inline bool Read(const std::string &path,std::vector<unsigned char> &out,std::string &error) {
  std::vector<unsigned char> bytes;std::string name;
  if(!ReadMedia(path,bytes,name,error))return false;
  if(!Extension(name,".tap") && !Extension(name,".tzx")) {error="Expected TAP or TZX tape";return false;}
  if(!ValidateTape(bytes)) {error="Invalid TAP/TZX file: "+name;return false;}
  out.swap(bytes);return true;
}
}
#endif
