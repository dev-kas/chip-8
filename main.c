#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vm.h"

#define WIDTH 1280
#define HEIGHT 640

const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream,
                                   int additional_amount, int total_amount) {
  (void)total_amount;
  struct CPU *cpu = (struct CPU *)userdata;
  static int sample_index = 0;

  int samples_needed = additional_amount / (int)sizeof(int16_t);
  int16_t buffer[512];

  while (samples_needed > 0) {
    int chunk = (samples_needed < 512) ? samples_needed : 512;
    for (int i = 0; i < chunk; i++) {
      if (cpu->sound_timer > 0) {
        buffer[i] = ((sample_index / 50) % 2 == 0) ? 3000 : -3000;
        sample_index++;
      } else {
        buffer[i] = 0;
        sample_index = 0;
      }
    }
    SDL_PutAudioStreamData(stream, buffer, chunk * sizeof(int16_t));
    samples_needed -= chunk;
  }
}

static void handle_input(struct CPU *cpu, bool *running) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      *running = false;
      return;
    }
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
      bool down = (event.type == SDL_EVENT_KEY_DOWN);
      int key = -1;
      switch (event.key.key) {
      case SDLK_1:
        key = 0x1;
        break;
      case SDLK_2:
        key = 0x2;
        break;
      case SDLK_3:
        key = 0x3;
        break;
      case SDLK_4:
        key = 0xC;
        break;
      case SDLK_Q:
        key = 0x4;
        break;
      case SDLK_W:
        key = 0x5;
        break;
      case SDLK_E:
        key = 0x6;
        break;
      case SDLK_R:
        key = 0xD;
        break;
      case SDLK_A:
        key = 0x7;
        break;
      case SDLK_S:
        key = 0x8;
        break;
      case SDLK_D:
        key = 0x9;
        break;
      case SDLK_F:
        key = 0xE;
        break;
      case SDLK_Z:
        key = 0xA;
        break;
      case SDLK_X:
        key = 0x0;
        break;
      case SDLK_C:
        key = 0xB;
        break;
      case SDLK_V:
        key = 0xF;
        break;
      }
      if (key != -1) {
        if (down)
          cpu->keys |= (1u << key);
        else
          cpu->keys &= ~(1u << key);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: %s <path_to_rom>\n", argv[0]);
    return 1;
  }

  srand((unsigned)time(NULL));

  struct CPU cpu = {0};
  cpu.PC = 0x200;

  memcpy(&cpu.memory[0x050], fontset, sizeof(fontset));
  if (cpu_load_rom(&cpu, argv[1]) != 0) {
    return 1;
  }

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  SDL_Window *win = SDL_CreateWindow("CHIP-8", WIDTH, HEIGHT, 0);
  SDL_Renderer *renderer = SDL_CreateRenderer(win, NULL);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, 64, 32);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  SDL_AudioSpec audio_spec = {
      .format = SDL_AUDIO_S16LE, .channels = 1, .freq = 44100};
  SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, audio_callback, &cpu);
  if (stream) {
    SDL_ResumeAudioStreamDevice(stream);
  }

  uint32_t pixels[64 * 32];
  bool running = true;

  while (running) {
    handle_input(&cpu, &running);

    for (int i = 0; i < 10; i++) {
      cpu_tick(&cpu);
    }

    if (cpu.delay_timer > 0)
      cpu.delay_timer--;
    if (cpu.sound_timer > 0)
      cpu.sound_timer--;

    if (cpu._rerender_window) {
      cpu._rerender_window = false;

      for (int i = 0; i < 64 * 32; i++) {
        pixels[i] = cpu.display[i] ? 0xFFFFFFFF : 0xFF000000;
      }

      SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
      SDL_RenderClear(renderer);
      SDL_RenderTexture(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
    }

    SDL_Delay(1000 / 60);
  }

  if (stream) {
    SDL_DestroyAudioStream(stream);
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
