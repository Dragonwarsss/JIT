// hello_3.cpp
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>

using namespace std;

void append_message_size(vector<uint8_t> &machine_code, const string &data) {
  size_t message_size = data.length();

  machine_code[24] = (message_size & 0xFF) >> 0;
  machine_code[25] = (message_size & 0xFF00) >> 8;
  machine_code[26] = (message_size & 0xFF0000) >> 16;
  machine_code[27] = (message_size & 0xFF000000) >> 24;
}

void print_debug_hex(const vector<uint8_t> &machine_code) {
  int count = 0;
  
  cout << "DBG :" << endl;
  cout << hex;
  for(auto e : machine_code) {
    cout << (int)e << " ";
    count++;
    if(count % 7 == 0)
      cout << '\n';
  }
  cout << dec;
  cout << "\n\n";
}

size_t estimate_memory_size(size_t machine_code_size) {
    size_t page_size_multiple = sysconf(_SC_PAGE_SIZE); // machine page size
    size_t factor = 1;
    size_t required_memory_size;

    while {
        required_memory_size = factor * page_size_multiple;
        if (machine_code_size <= required_memory_size)
          break;
        factor++;
    }
    return required_memory_size;
}

int main() {
  string data = "Hello World!\n";

  // Store the machine code in memory
  vector<uint8_t> machine_code {
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,
    0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00, // stdin fd 0x01
    0x48, 0x8d, 0x35, 0x0a, 0x00, 0x00, 0x00, // string location
    0x48, 0xc7, 0xc2, 0x00, 0x00, 0x00, 0x00, // string length (default 0)
    0x0f, 0x05,                               // execute
    0xc3                                      // return
  };

  append_message_size(machine_code, data);

  for(auto c : data) {
    machine_code.push_back(c);
  }

  print_debug_hex(machine_code);
  cout << "-------------------" << endl;

  size_t required_memory_size = estimate_memory_size(machine_code.size());

  uint8_t *mem = (uint8_t*) mmap(NULL, required_memory_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS ,-1, 0);
  if(mem == MAP_FAILED) {
    cerr << "Can't allocate memory\n"; exit(1);
  }

	for(size_t i = 0; i < machine_code.size(); ++i) {
	  mem[i] = machine_code[i];
	}

  void (*func)();
  func = (void (*)()) mem;
  func();
  munmap(mem, required_memory_size); // free
}
