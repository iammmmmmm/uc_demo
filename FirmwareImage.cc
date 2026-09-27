// FirmwareImage.cc
// 固件加载器实现: .bin (裸二进制) / .hex (Intel HEX) / .elf (ELF32 可执行)
#include "FirmwareImage.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace {

// -----------------------------------------------------------------------------
// 通用小工具
// -----------------------------------------------------------------------------
std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string extensionOf(const std::string &path) {
  const auto slash = path.find_last_of("/\\");
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
    return {};
  }
  return toLower(path.substr(dot));
}

std::string fileNameOf(const std::string &path) {
  const auto slash = path.find_last_of("/\\");
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool isAbsolutePath(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  if (path[0] == '/' || path[0] == '\\') {
    return true;
  }
  // Windows 盘符: C:\ 或 C:/
  return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':';
}

// 可执行文件所在目录 (用于从任意工作目录运行都能找到 firmware/)
std::string executableDir() {
#if defined(_WIN32)
  char buffer[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return {};
  }
  const std::string path(buffer, len);
  const auto pos = path.find_last_of("/\\");
  return pos == std::string::npos ? std::string{} : path.substr(0, pos);
#elif defined(__linux__)
  char buffer[4096] = {};
  const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (len <= 0) {
    return {};
  }
  const std::string path(buffer, static_cast<size_t>(len));
  const auto pos = path.find_last_of('/');
  return pos == std::string::npos ? std::string{} : path.substr(0, pos);
#else
  return {};
#endif
}

bool readWholeFile(const std::string &path, std::vector<uint8_t> &out, std::string &error) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    error = "无法打开固件文件: " + path;
    return false;
  }
  const std::streamoff size = file.tellg();
  if (size < 0) {
    error = "无法获取固件文件大小: " + path;
    return false;
  }
  file.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(size));
  if (size > 0 && !file.read(reinterpret_cast<char *>(out.data()), size)) {
    error = "读取固件文件失败: " + path;
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// 分段的追加 / 排序 / 合并
// -----------------------------------------------------------------------------
void appendChunk(FirmwareImage &img, uint32_t address, const uint8_t *data, size_t len) {
  if (len == 0) {
    return;
  }
  if (!img.chunks.empty()) {
    FirmwareChunk &last = img.chunks.back();
    if (static_cast<uint64_t>(last.address) + last.data.size() == address) {
      last.data.insert(last.data.end(), data, data + len);
      return;
    }
  }
  FirmwareChunk chunk;
  chunk.address = address;
  chunk.data.assign(data, data + len);
  img.chunks.push_back(std::move(chunk));
}

// 按地址排序, 并合并地址连续的分段
void normalizeChunks(FirmwareImage &img) {
  std::stable_sort(img.chunks.begin(), img.chunks.end(),
                   [](const FirmwareChunk &a, const FirmwareChunk &b) {
                     return a.address < b.address;
                   });
  std::vector<FirmwareChunk> merged;
  for (auto &chunk : img.chunks) {
    if (!merged.empty()) {
      FirmwareChunk &last = merged.back();
      if (static_cast<uint64_t>(last.address) + last.data.size() == chunk.address) {
        last.data.insert(last.data.end(), chunk.data.begin(), chunk.data.end());
        continue;
      }
    }
    merged.push_back(std::move(chunk));
  }
  img.chunks = std::move(merged);

  img.total_bytes = 0;
  for (const auto &chunk : img.chunks) {
    img.total_bytes += chunk.data.size();
  }
  if (!img.chunks.empty()) {
    img.load_base = img.chunks.front().address;
  }
}

// -----------------------------------------------------------------------------
// Intel HEX 解析
// -----------------------------------------------------------------------------
int hexDigit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

bool parseIntelHex(const std::string &path, FirmwareImage &img, std::string &error) {
  std::ifstream file(path);
  if (!file.is_open()) {
    error = "无法打开固件文件: " + path;
    return false;
  }

  uint32_t base = 0;       // 由 02/04 记录提供的高位地址
  size_t lineNo = 0;
  bool sawEof = false;
  std::string line;

  while (std::getline(file, line)) {
    ++lineNo;
    // 去掉 CR/LF 与首尾空白
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
      continue;  // 空行
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    line = line.substr(first, last - first + 1);
    if (line.empty() || line[0] != ':') {
      error = "HEX 格式错误 (第 " + std::to_string(lineNo) + " 行): 记录必须以 ':' 开头";
      return false;
    }
    if ((line.size() - 1) % 2 != 0) {
      error = "HEX 格式错误 (第 " + std::to_string(lineNo) + " 行): 字符数为奇数";
      return false;
    }

    std::vector<uint8_t> bytes;
    bytes.reserve((line.size() - 1) / 2);
    for (size_t i = 1; i + 1 < line.size(); i += 2) {
      const int hi = hexDigit(line[i]);
      const int lo = hexDigit(line[i + 1]);
      if (hi < 0 || lo < 0) {
        error = "HEX 格式错误 (第 " + std::to_string(lineNo) + " 行): 含非法十六进制字符";
        return false;
      }
      bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    if (bytes.size() < 5) {
      error = "HEX 格式错误 (第 " + std::to_string(lineNo) + " 行): 记录过短";
      return false;
    }

    // 校验和: 所有字节(含校验和)之和应为 0
    uint8_t sum = 0;
    for (const uint8_t b : bytes) {
      sum = static_cast<uint8_t>(sum + b);
    }
    if (sum != 0) {
      std::ostringstream oss;
      oss << "HEX 校验和错误 (第 " << lineNo << " 行)";
      error = oss.str();
      return false;
    }

    const uint8_t len = bytes[0];
    const uint32_t addr = (static_cast<uint32_t>(bytes[1]) << 8) | bytes[2];
    const uint8_t type = bytes[3];
    if (bytes.size() != static_cast<size_t>(len) + 5) {
      error = "HEX 格式错误 (第 " + std::to_string(lineNo) + " 行): 长度字段与实际数据不符";
      return false;
    }
    const uint8_t *data = bytes.data() + 4;

    switch (type) {
      case 0x00:  // 数据记录
        appendChunk(img, base + addr, data, len);
        break;
      case 0x01:  // 文件结束
        sawEof = true;
        break;
      case 0x02:  // 扩展段地址 (<<4)
        if (len != 2) {
          error = "HEX 记录 02 长度非法 (第 " + std::to_string(lineNo) + " 行)";
          return false;
        }
        base = ((static_cast<uint32_t>(data[0]) << 8) | data[1]) << 4;
        break;
      case 0x03:  // 起始段地址 (CS:IP)
        if (len == 4) {
          img.entry = (((static_cast<uint32_t>(data[0]) << 8) | data[1]) << 4) +
                      ((static_cast<uint32_t>(data[2]) << 8) | data[3]);
          img.has_entry = true;
        }
        break;
      case 0x04:  // 扩展线性地址 (<<16)
        if (len != 2) {
          error = "HEX 记录 04 长度非法 (第 " + std::to_string(lineNo) + " 行)";
          return false;
        }
        base = ((static_cast<uint32_t>(data[0]) << 8) | data[1]) << 16;
        break;
      case 0x05:  // 起始线性地址 (入口点)
        if (len == 4) {
          img.entry = (static_cast<uint32_t>(data[0]) << 24) |
                      (static_cast<uint32_t>(data[1]) << 16) |
                      (static_cast<uint32_t>(data[2]) << 8) | data[3];
          img.has_entry = true;
        }
        break;
      default:
        error = "HEX 含不支持的记录类型 0x" + std::to_string(type) + " (第 " +
                std::to_string(lineNo) + " 行)";
        return false;
    }

    if (sawEof) {
      break;
    }
  }

  if (img.empty()) {
    error = "HEX 文件中没有任何数据记录";
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// ELF32 解析 (仅使用程序头 PT_LOAD 段)
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct Elf32_Ehdr {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct Elf32_Phdr {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};
#pragma pack(pop)

static_assert(sizeof(Elf32_Ehdr) == 52, "ELF32 头大小应为 52 字节");
static_assert(sizeof(Elf32_Phdr) == 32, "ELF32 程序头大小应为 32 字节");

bool parseElf32(const std::string &path,
                const std::vector<uint8_t> &raw,
                FirmwareImage &img,
                std::string &error) {
  if (raw.size() < sizeof(Elf32_Ehdr)) {
    error = "ELF 文件过小: " + path;
    return false;
  }
  Elf32_Ehdr ehdr{};
  std::memcpy(&ehdr, raw.data(), sizeof(ehdr));

  if (std::memcmp(ehdr.e_ident, "\x7f" "ELF", 4) != 0) {
    error = "不是 ELF 文件 (魔数不匹配): " + path;
    return false;
  }
  if (ehdr.e_ident[4] != 1) {
    error = "仅支持 ELF32 (32 位) 固件，当前文件为 64 位";
    return false;
  }
  if (ehdr.e_ident[5] != 1) {
    error = "仅支持小端 (little-endian) ELF 固件";
    return false;
  }
  if (ehdr.e_phoff == 0 || ehdr.e_phnum == 0) {
    error = "ELF 文件不含程序头, 无法确定加载段";
    return false;
  }
  if (ehdr.e_phoff + static_cast<uint64_t>(ehdr.e_phnum) * ehdr.e_phentsize > raw.size()) {
    error = "ELF 程序头表越界, 文件可能已损坏";
    return false;
  }

  constexpr uint32_t PT_LOAD = 1;
  for (uint16_t i = 0; i < ehdr.e_phnum; ++i) {
    Elf32_Phdr phdr{};
    std::memcpy(&phdr, raw.data() + ehdr.e_phoff + static_cast<uint64_t>(i) * ehdr.e_phentsize,
                sizeof(phdr));
    if (phdr.p_type != PT_LOAD || phdr.p_filesz == 0) {
      continue;  // 非加载段, 或仅有 .bss (无文件内容, SRAM 初始即为 0)
    }
    if (static_cast<uint64_t>(phdr.p_offset) + phdr.p_filesz > raw.size()) {
      error = "ELF 段 " + std::to_string(i) + " 的数据越界, 文件可能已损坏";
      return false;
    }
    // 裸机镜像按物理地址加载; paddr 为 0 时退回 vaddr
    const uint32_t target = phdr.p_paddr != 0 ? phdr.p_paddr : phdr.p_vaddr;
    appendChunk(img, target, raw.data() + phdr.p_offset, phdr.p_filesz);
  }

  if (img.empty()) {
    error = "ELF 文件中没有可加载的 PT_LOAD 段";
    return false;
  }
  img.entry = ehdr.e_entry;
  img.has_entry = true;
  return true;
}

// 读取文件头 4 字节用于格式嗅探
bool sniffMagic(const std::string &path, uint8_t magic[4]) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.read(reinterpret_cast<char *>(magic), 4);
  return file.gcount() == 4;
}

}  // namespace

// -----------------------------------------------------------------------------
// 对外接口
// -----------------------------------------------------------------------------
ImageFormat detectFormat(const std::string &path) {
  const std::string ext = extensionOf(path);
  if (ext == ".hex" || ext == ".ihex" || ext == ".ihx") {
    return ImageFormat::Hex;
  }
  if (ext == ".elf" || ext == ".axf" || ext == ".out") {
    return ImageFormat::Elf;
  }
  if (ext == ".bin") {
    return ImageFormat::Bin;
  }
  // 扩展名未知: 按文件头魔数嗅探
  uint8_t magic[4] = {};
  if (sniffMagic(path, magic)) {
    if (std::memcmp(magic, "\x7f" "ELF", 4) == 0) {
      return ImageFormat::Elf;
    }
    if (magic[0] == ':') {
      return ImageFormat::Hex;
    }
  }
  return ImageFormat::Bin;
}

const char *formatName(ImageFormat fmt) {
  switch (fmt) {
    case ImageFormat::Hex:
      return "hex";
    case ImageFormat::Elf:
      return "elf";
    case ImageFormat::Bin:
    case ImageFormat::Auto:
    default:
      return "bin";
  }
}

std::string resolveFirmwarePath(const std::string &requested, std::vector<std::string> *tried) {
  std::vector<std::string> candidates;
  auto push = [&candidates](const std::string &p) {
    if (p.empty()) {
      return;
    }
    if (std::find(candidates.begin(), candidates.end(), p) == candidates.end()) {
      candidates.push_back(p);
    }
  };

  push(requested);
  // 绝对路径不再做任何猜测
  if (!isAbsolutePath(requested)) {
    const std::string exeDir = executableDir();
    const std::string name = fileNameOf(requested);
    if (!exeDir.empty()) {
      push(exeDir + "/" + requested);
      push(exeDir + "/" + name);
      push(exeDir + "/../firmware/" + name);
    }
    push("firmware/" + name);
    push("../firmware/" + name);
  }

  for (const std::string &candidate : candidates) {
    if (tried != nullptr) {
      tried->push_back(candidate);
    }
    std::ifstream file(candidate, std::ios::binary);
    if (file.is_open()) {
      return candidate;
    }
  }
  return {};
}

bool loadFirmware(const std::string &path,
                  ImageFormat fmt,
                  uint32_t binBase,
                  FirmwareImage &out,
                  std::string &error) {
  out = FirmwareImage{};
  out.path = path;

  if (fmt == ImageFormat::Auto) {
    fmt = detectFormat(path);
  }
  out.format = formatName(fmt);

  if (fmt == ImageFormat::Hex) {
    if (!parseIntelHex(path, out, error)) {
      return false;
    }
  } else if (fmt == ImageFormat::Elf) {
    std::vector<uint8_t> raw;
    if (!readWholeFile(path, raw, error)) {
      return false;
    }
    if (!parseElf32(path, raw, out, error)) {
      return false;
    }
  } else {
    std::vector<uint8_t> raw;
    if (!readWholeFile(path, raw, error)) {
      return false;
    }
    if (raw.empty()) {
      error = "固件文件为空: " + path;
      return false;
    }
    appendChunk(out, binBase, raw.data(), raw.size());
  }

  normalizeChunks(out);
  if (out.empty()) {
    error = "固件文件未包含任何数据: " + path;
    return false;
  }
  return true;
}
