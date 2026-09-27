// FirmwareImage.h
// 固件加载器: 支持原始二进制 (.bin)、Intel HEX (.hex) 与 ELF32 (.elf/.axf)
// 三种格式统一解析为「若干段需要写入模拟内存的数据」，供 main.cpp 使用。
#pragma once

#include <cstdint>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 一段需要写入模拟内存的数据
// -----------------------------------------------------------------------------
struct FirmwareChunk {
  uint32_t address = 0;       // 目标绝对地址 (模拟内存地址)
  std::vector<uint8_t> data;  // 数据内容
};

// -----------------------------------------------------------------------------
// 固件加载结果
// -----------------------------------------------------------------------------
struct FirmwareImage {
  std::string path;                   // 实际成功打开的文件路径
  std::string format;                 // "bin" / "hex" / "elf"
  std::vector<FirmwareChunk> chunks;  // 按地址升序排列、相邻已合并
  bool has_entry = false;             // 是否含确定的入口地址 (ELF 头 / HEX 起始记录)
  uint32_t entry = 0;                 // 入口地址 (可能不含 Thumb 位)
  uint32_t load_base = 0;             // 最低加载地址 (向量表所在处)
  uint64_t total_bytes = 0;           // 数据总字节数

  [[nodiscard]] bool empty() const { return chunks.empty(); }
};

enum class ImageFormat { Auto, Bin, Hex, Elf };

// 按扩展名 / 文件头魔数识别固件格式
ImageFormat detectFormat(const std::string &path);

// 格式名称 ("bin" / "hex" / "elf")
const char *formatName(ImageFormat fmt);

// 在常见位置查找固件文件, 依次尝试:
//   给定路径 -> 可执行文件同级目录 -> <exe>/firmware -> <exe>/../firmware
//   -> ./firmware -> ../firmware  (绝对路径只尝试其本身)
// 找到返回可读路径; 找不到返回空串, tried 记录所有尝试过的候选路径
std::string resolveFirmwarePath(const std::string &requested,
                                std::vector<std::string> *tried);

// 加载固件。binBase 仅对 .bin 生效 (bin 文件自身不含地址信息)。
// 失败时返回 false 并填写 error。
bool loadFirmware(const std::string &path,
                  ImageFormat fmt,
                  uint32_t binBase,
                  FirmwareImage &out,
                  std::string &error);
