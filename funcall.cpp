#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

using namespace std;

struct MemPage {
  uint8_t *mem;
  size_t page_size;
  size_t pages = 0;
  size_t position = 0;

  MemPage(size_t requested_pages = 1) {
    page_size = sysconf(_SC_PAGE_SIZE);
    mem = (uint8_t*) mmap(NULL, page_size * requested_pages, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS ,-1, 0);
    if(mem == MAP_FAILED) {
      throw runtime_error("Can't allocate enough executable memory!");
    }
    pages = requested_pages;
  }

  ~MemPage() {
    munmap(mem, pages * page_size);
  }

  void push(uint8_t data) {
    check_available_space(sizeof data);
    mem[position] = data;
    position++;
  }

  void push(void (*fn)()) {
    size_t fn_address = reinterpret_cast<size_t>(fn);
    check_available_space(sizeof fn_address);

    memcpy((mem + position), &fn_address, sizeof fn_address);
    position += sizeof fn_address;
  }

  void push(const vector<uint8_t> &data) {
    check_available_space(data.size());
    memcpy((mem + position), &data[0], data.size());
    position += data.size();
  }

  void check_available_space(size_t data_size) {
    if(position + data_size > pages * page_size) {
      throw runtime_error("Not enough virtual memory allocated!");
    }
  }

  void show_memory() {
    cout << "Mem: " << position << "/" << pages * page_size << " bytes used" << endl;
    cout << hex;
    for(size_t i = 0; i < position; ++i) {
      cout << "0x" << (int) mem[i] << " ";
      if(i % 16 == 0 && i > 0) {
        cout << endl;
      }
    }
    cout << dec << endl;
  }
};

namespace AssemblyChunks {
  vector<uint8_t>function_prologue {
    0x55,               // push rbp
    0x48, 0x89, 0xe5,   // mov	rbp, rsp
  };

  vector<uint8_t>function_epilogue {
    0x5d,   // pop	rbp
    0xc3    // ret
  };
}

vector<int> a{1, 2, 3};

void test() {
    printf("Toto je suis toto\n");
    for(auto &e : a) {
        e -= 5;
    }
}

int main() {
  MemPage mp;

  mp.push(AssemblyChunks::function_prologue);

  mp.push(0x48); mp.push(0xb8); mp.push(test);    // movabs rax, <function_address>
  mp.push(0xff); mp.push(0xd0);                   // call rax

  mp.push(AssemblyChunks::function_epilogue);
  mp.show_memory();

  void (*func)() = reinterpret_cast<void (*)()>(mp.mem);
  func();
}
