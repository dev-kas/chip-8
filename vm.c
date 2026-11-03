#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

int cpu_load_rom(struct CPU *cpu, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "cannot open rom file: %s\n", filename);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size > (4096 - 0x200)) {
    fprintf(stderr, "rom file size (%ld bytes) too big for memory\n",
            file_size);
    fclose(file);
    return -1;
  }

  size_t bytes_read = fread(&cpu->memory[0x200], 1, file_size, file);
  fclose(file);

  if (bytes_read != (size_t)file_size) {
    fprintf(stderr, "cannot read full rom content into memory\n");
    return -1;
  }

  printf("loaded rom (%zu bytes) to 0x200\n", bytes_read);
  return 0;
}

void cpu_tick(struct CPU *cpu) {
  // fetch
  uint16_t opcode = (cpu->memory[cpu->PC] << 8) | cpu->memory[cpu->PC + 1];
  cpu->PC += 2;

  // decode
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;
  uint8_t n = opcode & 0x000F;
  uint8_t nn = opcode & 0x00FF;
  uint16_t nnn = opcode & 0x0FFF;

  // dispatch
  switch (opcode & 0xF000) {
  case 0x0000:
    if (nn == 0xE0) {
      memset(cpu->display, 0, sizeof(cpu->display));
      cpu->_rerender_window = true;
    }
    if (nn == 0xEE)
      cpu->PC = cpu->stack[--cpu->SP];
    break;
  case 0x1000:
    cpu->PC = nnn;
    break;
  case 0x2000:
    cpu->stack[cpu->SP++] = cpu->PC;
    cpu->PC = nnn;
    break;
  case 0x3000:
    if (cpu->V[x] == nn)
      cpu->PC += 2;
    break;
  case 0x4000:
    if (cpu->V[x] != nn)
      cpu->PC += 2;
    break;
  case 0x5000:
    if (n == 0 && cpu->V[x] == cpu->V[y])
      cpu->PC += 2;
    break;
  case 0x6000:
    cpu->V[x] = nn;
    break;
  case 0x7000:
    cpu->V[x] += nn;
    break;
  case 0x8000:
    switch (n) {
    case 0x0:
      cpu->V[x] = cpu->V[y];
      break;

    case 0x1:
      cpu->V[x] |= cpu->V[y];
      break;

    case 0x2:
      cpu->V[x] &= cpu->V[y];
      break;

    case 0x3:
      cpu->V[x] ^= cpu->V[y];
      break;

    case 0x4: {
      uint16_t sum = (uint16_t)cpu->V[x] + cpu->V[y];
      uint8_t carry = (sum > 0xFF) ? 1 : 0;
      cpu->V[x] = (uint8_t)(sum & 0xFF);
      cpu->V[0xF] = carry;
      break;
    }

    case 0x5: {
      uint8_t not_borrow = (cpu->V[x] >= cpu->V[y]) ? 1 : 0;
      cpu->V[x] = cpu->V[x] - cpu->V[y];
      cpu->V[0xF] = not_borrow;
      break;
    }

    case 0x6: {
      uint8_t lsb = cpu->V[x] & 0x1;
      cpu->V[x] >>= 1;
      cpu->V[0xF] = lsb;
      break;
    }

    case 0x7: {
      uint8_t not_borrow = (cpu->V[y] >= cpu->V[x]) ? 1 : 0;
      cpu->V[x] = cpu->V[y] - cpu->V[x];
      cpu->V[0xF] = not_borrow;
      break;
    }

    case 0xE: {
      uint8_t msb = (cpu->V[x] & 0x80) >> 7;
      cpu->V[x] <<= 1;
      cpu->V[0xF] = msb;
      break;
    }

    default:
      break;
    }
    break;
  case 0x9000:
    if (cpu->V[x] != cpu->V[y])
      cpu->PC += 2;
    break;
  case 0xA000:
    cpu->I = nnn;
    break;
  case 0xB000:
    cpu->PC = nnn + cpu->V[0];
    break;
  case 0xC000:
    cpu->V[x] = (uint8_t)(rand() & 0xFF) & nn;
    break;
  case 0xD000: {
    uint8_t xc = cpu->V[x] % 64;
    uint8_t yc = cpu->V[y] % 32;
    uint8_t height = n;

    cpu->V[0xF] = 0;

    for (int yl = 0; yl < height; yl++) {
      int yp = yc + yl;
      if (yp >= 32)
        break;

      uint8_t pixelByte = cpu->memory[cpu->I + yl];
      for (int xPixel = 0; xPixel < 8; xPixel++) {
        int xp = xc + xPixel;
        if (xp >= 64)
          break;

        if ((pixelByte & (0x80 >> xPixel)) != 0) {
          int index = yp * 64 + xp;
          if (cpu->display[index] == 1) {
            cpu->V[0xF] = 1;
          }
          cpu->display[index] ^= 1;
        }
      }
    }

    cpu->_rerender_window = true;
    break;
  }
  case 0xE000:
    if (nn == 0x9E) {
      if (cpu->keys & (1u << cpu->V[x]))
        cpu->PC += 2;
    } else if (nn == 0xA1) {
      if (!(cpu->keys & (1u << cpu->V[x])))
        cpu->PC += 2;
    }
    break;
  case 0xF000:
    switch (nn) {
    case 0x07:
      cpu->V[x] = cpu->delay_timer;
      break;

    case 0x0A: {
      if (cpu->keys == 0) {
        cpu->PC -= 2;
        break;
      }

      for (int i = 0; i < 16; i++) {
        if (cpu->keys & (1u << i)) {
          cpu->V[x] = i;
          break;
        }
      }

      break;
    }

    case 0x15:
      cpu->delay_timer = cpu->V[x];
      break;

    case 0x18:
      cpu->sound_timer = cpu->V[x];
      break;

    case 0x1E:
      cpu->I += cpu->V[x];
      break;

    case 0x29: {
      uint8_t vx = (opcode & 0x0F00) >> 8;
      uint8_t digit = cpu->V[vx] & 0x0F;
      cpu->I = 0x050 + (digit * 5);
      break;
    }

    case 0x33: {
      uint8_t vx = (opcode & 0x0F00) >> 8;
      uint8_t value = cpu->V[vx];

      cpu->memory[cpu->I] = value / 100;
      cpu->memory[cpu->I + 1] = (value / 10) % 10;
      cpu->memory[cpu->I + 2] = value % 10;
      break;
    }

    case 0x55:
      for (int i = 0; i <= x; i++)
        cpu->memory[cpu->I + i] = cpu->V[i];
      break;

    case 0x65:
      for (int i = 0; i <= x; i++)
        cpu->V[i] = cpu->memory[cpu->I + i];
      break;
    }
    break;
  }
}
