#include <stdbool.h>
#include <stdint.h>

struct CPU {
  uint8_t V[16];
  uint16_t I;

  uint16_t PC;
  uint8_t SP;

  uint8_t display[64 * 32];
  uint8_t memory[4096];
  uint16_t stack[16];

  uint8_t delay_timer;
  uint8_t sound_timer;

  uint16_t keys;

  // bindings for host
  bool _rerender_window;
};

int cpu_load_rom(struct CPU *cpu, const char *filename);
void cpu_tick(struct CPU *cpu);
