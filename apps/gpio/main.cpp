#include <iostream>
#include <dirent.h>
#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <vector>

struct FileDescriptor {
  int fd;
};

void PrintChipInfo(const std::string& chip);
void PrintLineInfo(FileDescriptor chip, int line);
std::string GetCh341ChipFilename();
std::vector<std::string> GetChipChipNames();

int main() {
  auto chips = GetChipChipNames();
  std::cout << "Number of GPIO chips :  " << chips.size() << std::endl;

  for(const auto& chip : chips) {
    PrintChipInfo(chip);
  }

  try {
    auto ch341Index = GetCh341ChipFilename();
    std::cout << std::endl << std::endl << " --> CH341 GPIO chip detected at index " << ch341Index << std::endl;
  } catch(const std::runtime_error& error) {
    std::cerr << "Error : " << error.what();
  }

  return 0;
}

std::vector<std::string> GetChipChipNames() {
  auto gpioFilenameFilter = [](const struct dirent *entry) -> int {
    std::string filename = entry->d_name;
    return filename.rfind("gpiochip", 0) == 0;
  };
  static const char* devString = "/dev/";

  std::vector<std::string> filenames;
  struct dirent **entries;
  auto nb = scandir(devString, &entries, gpioFilenameFilter, alphasort);
  for(int i = 0; i < nb; i++) {
    std::string name = std::string{devString} + std::string{entries[i]->d_name};
    filenames.push_back(name);
  }
  return filenames;
}

void PrintChipInfo(const std::string& chip) {
  struct gpiochip_info info;
  auto fd = open(chip.c_str(), O_RDWR);

  auto res = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
  if(res == 0) {
    std::cout << chip << " : " << info.name << " (" << info.label
              << ") : " << info.lines << " lines" << std::endl;
    for(int i = 0; i < info.lines; i++) {
      PrintLineInfo({fd}, i);
    }
  } else {
    std::cerr << "Error while getting chip info for " << chip << std::endl;
  }
}

void PrintLineInfo(FileDescriptor fileDescriptor, int line) {
  struct gpioline_info info;
  info.line_offset = line;

  auto res = ioctl(fileDescriptor.fd, GPIO_GET_LINEINFO_IOCTL, &info);
  if(res == 0) {
    std::cout << "\t[" << line << "] " << info.name << " - " << info.consumer << " - " << info.flags << std::endl;
  } else {
    std::cerr << "Error while getting line info for line " << line;
  }
}

bool GetChipInfo(const std::string chip, struct gpiochip_info& info) {
  auto fd = open(chip.c_str(), O_RDWR);
  auto res = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
  return res == 0;
}

std::string GetCh341ChipFilename() {
  static const char* ch341ChipLabel = "ch341";
  static const size_t ch341NbLines = 16;

  auto chips = GetChipChipNames();
  for(auto& chip : chips) {
    struct gpiochip_info info;
    if(GetChipInfo(chip, info)) {
      if(std::string{info.label} == std::string{ch341ChipLabel} && info.lines == ch341NbLines)
        return chip;
    }
  }
  throw std::runtime_error("Cannot find a GPIO chip corresponding to the CH341");
}
