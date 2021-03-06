#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include <vector>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

using namespace std;

void PrintRegisters(Z80 &cpu) {
    printf("--------------------------------\n");
    printf("PC: 0x%.8x SP : 0x%.8x\n", cpu.regs.PC, cpu.regs.SP);
    printf("AF: 0x%.8x AF': 0x%.8x\n", cpu.regs.AF, cpu.regs.altAF);
    printf("BC: 0x%.8x BC': 0x%.8x\n", cpu.regs.BC, cpu.regs.altBC);
    printf("DE: 0x%.8x DE': 0x%.8x\n", cpu.regs.DE, cpu.regs.altDE);
    printf("HL: 0x%.8x HL': 0x%.8x\n", cpu.regs.HL, cpu.regs.altHL);
    printf("IX: 0x%.8x IY : 0x%.8x\n", cpu.regs.IX, cpu.regs.IY);
    printf("I : 0x%.8x R  : 0x%.8x\n", cpu.regs.I,  cpu.regs.R);
    printf("--------------------------------\n");
}

void ReadTapFile(vector<unsigned char> &data, vector<unsigned char>::iterator &block) {

    if (block == data.end()) {
        printf("Not enough data to read a block\n");
        return;
    }

    // Assuming block points to a valid block

    printf("Reading a TAP block\n");

    // First two bytes are the size of the block
    unsigned short nBytesLSB = *block++;
    unsigned short nBytesMSB = *block++;
    unsigned short nBytes = (nBytesMSB << 8) | nBytesLSB;

    // Remove the flag + the checksum to compute the total number of bytes
    unsigned short totalNBytes = nBytes - 2;

    // Type of block (A reg, 00 for headers, ff for data blocks)
    unsigned short blockType = *block++;

    printf(" - block size = %d\n", totalNBytes);

    // flag (00 for header, ff for data)
    if (blockType == 0x00) {
        printf(" - block type = header\n");

        // first block of header
        unsigned short headerType = *block++;

        // filename
        char filename[11];
        for (unsigned int i = 0; i < 10; i++) {
            filename[i] = *block++;
        }
        filename[10] = '\0';
        printf(" - filename = %s\n", filename);

        // Based on header type read the next bytes
        unsigned short lengthDataBlockLSB;
        unsigned short lengthDataBlockMSB;
        unsigned short lengthDataBlock;
        unsigned short parameter1[2];
        unsigned short parameter2[2];
        switch (headerType) {
            case 0x00: printf(" - header program block\n");
                       lengthDataBlockLSB = *block++;
                       lengthDataBlockMSB = *block++;
                       lengthDataBlock = (lengthDataBlockLSB << 8) | lengthDataBlockMSB;
                       printf(" - length of data block = %d\n", lengthDataBlock);
                       parameter1[1] = *block++;
                       parameter1[0] = *block++;
                       printf(" - autostart line number = %.2x %.2x\n", parameter1[0], parameter1[1]);
                       parameter2[1] = *block++;
                       parameter2[0] = *block++;
                       printf(" - start of the variable area relative to the start of the program = %.2x %.2x\n", parameter2[0], parameter2[1]);
                       break;
            case 0x01: printf(" - header number array block\n");
                       break;
            case 0x02: printf(" - header character array block\n");
                       break;
            case 0x03: printf(" - header code block\n");
                       lengthDataBlockLSB = *block++;
                       lengthDataBlockMSB = *block++;
                       lengthDataBlock = (lengthDataBlockLSB << 8) | lengthDataBlockMSB;
                       printf(" - length of data block = %d\n", lengthDataBlock);
                       parameter1[1] = *block++;
                       parameter1[0] = *block++;
                       printf(" - start of code block = %.2x %.2x\n", parameter1[0], parameter1[1]);
                       parameter2[1] = *block++;
                       parameter2[0] = *block++;
                       printf(" - should be 32768 = %.2x %.2x\n", parameter2[0], parameter2[1]);
                       break;
            default  : printf(" - header unknown block\n");
                       break;
        }

        // checksum
        printf(" - checksum = 0x%.2x\n", *block++);

    } else if (blockType == 0xff) {
        printf(" - block type = data\n");
        block = block + totalNBytes + 1; // skip checksum
    } else {
        printf(" - block type = unknown\n");
        block = block + totalNBytes + 1; // skip checksum
    }

    // Recursively, call again to ReadTapFile to read
    // more blocks from the tap file
    ReadTapFile(data, block);
}

int LoadTrap(Z80& cpu, vector<unsigned char>& data, vector<unsigned char>::iterator &block) {

  printf("Calling to LoadTrap\n");
  printf(" DE = 0x%x (%d)\n", cpu.regs.DE, cpu.regs.DE);
  printf(" IX = 0x%x (%d)\n", cpu.regs.IX, cpu.regs.IX);

  // First byte of data contains value for the A register on return.
  // Last byte is block checksum (not using it).  
  unsigned short nBytesLSB = *block;  
  block++;
  unsigned short nBytesMSB = *block;  
  block++;  unsigned short nBytes = (nBytesMSB << 8) | nBytesLSB;
  unsigned short totalNBytes = nBytes - 2;

  // Advance pointerto skip value for A
  block++;

  printf("Num Bytes = %d, DE=%d\n", nBytes, cpu.regs.DE);

  // DE should contain the tota number of bytes
  if (totalNBytes > cpu.regs.DE) {
     printf("WARNING: totalNBytes in TAP file does not match register DE. Forcing totalNBytes to DE value\n");
     totalNBytes = cpu.regs.DE;
  }

  // We must place data read from tape at IX base address onwards
  // DE is the number of bytes to read, IX increments with each byte read
  for (; totalNBytes > 0; block++) {
    //printf("Line=%d value=%x, DE=%d\n", totalNBytes, *block, cpu.regs.DE);
    // Write block using cpu's data bus and cpu's registers
    cpu.DataBus->Write(cpu.regs.IX++, *block);
    totalNBytes--;
    cpu.regs.DE--;
  }

  // Read checksum
  block++;
  return 0;
}

void ReadKeyboard(ALLEGRO_EVENT_QUEUE *event_queue, ULA* ula) {
  bool keyEvent = false;
  bool keyDown  = false;
  ALLEGRO_EVENT ev;
  al_wait_for_event(event_queue, &ev);

  if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
    keyEvent = true;
    keyDown  = true;
  } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
    keyEvent = true;
  }

  if (keyEvent) {
    switch (ev.keyboard.keycode) {
      // Row 0
      case ALLEGRO_KEY_0: ula->PressKey(0, 9, keyDown); break;
      case ALLEGRO_KEY_1: ula->PressKey(0, 0, keyDown); break;
      case ALLEGRO_KEY_2: ula->PressKey(0, 1, keyDown); break;
      case ALLEGRO_KEY_3: ula->PressKey(0, 2, keyDown); break;
      case ALLEGRO_KEY_4: ula->PressKey(0, 3, keyDown); break;
      case ALLEGRO_KEY_5: ula->PressKey(0, 4, keyDown); break;
      case ALLEGRO_KEY_6: ula->PressKey(0, 5, keyDown); break;
      case ALLEGRO_KEY_7: ula->PressKey(0, 6, keyDown); break;
      case ALLEGRO_KEY_8: ula->PressKey(0, 7, keyDown); break;
      case ALLEGRO_KEY_9: ula->PressKey(0, 8, keyDown); break;
      // Row 1
      case ALLEGRO_KEY_Q: ula->PressKey(1, 0, keyDown); break;
      case ALLEGRO_KEY_W: ula->PressKey(1, 1, keyDown); break;
      case ALLEGRO_KEY_E: ula->PressKey(1, 2, keyDown); break;
      case ALLEGRO_KEY_R: ula->PressKey(1, 3, keyDown); break;
      case ALLEGRO_KEY_T: ula->PressKey(1, 4, keyDown); break;
      case ALLEGRO_KEY_Y: ula->PressKey(1, 5, keyDown); break;
      case ALLEGRO_KEY_U: ula->PressKey(1, 6, keyDown); break;
      case ALLEGRO_KEY_I: ula->PressKey(1, 7, keyDown); break;
      case ALLEGRO_KEY_O: ula->PressKey(1, 8, keyDown); break;
      case ALLEGRO_KEY_P: ula->PressKey(1, 9, keyDown); break;
      // Row 2
      case ALLEGRO_KEY_A: ula->PressKey(2, 0, keyDown); break;
      case ALLEGRO_KEY_S: ula->PressKey(2, 1, keyDown); break;
      case ALLEGRO_KEY_D: ula->PressKey(2, 2, keyDown); break;
      case ALLEGRO_KEY_F: ula->PressKey(2, 3, keyDown); break;
      case ALLEGRO_KEY_G: ula->PressKey(2, 4, keyDown); break;
      case ALLEGRO_KEY_H: ula->PressKey(2, 5, keyDown); break;
      case ALLEGRO_KEY_J: ula->PressKey(2, 6, keyDown); break;
      case ALLEGRO_KEY_K: ula->PressKey(2, 7, keyDown); break;
      case ALLEGRO_KEY_L: ula->PressKey(2, 8, keyDown); break;
      case ALLEGRO_KEY_ENTER: ula->PressKey(2, 9, keyDown); break;
      // Row 2
      case ALLEGRO_KEY_LSHIFT: ula->PressKey(3, 0, keyDown); break;
      case ALLEGRO_KEY_Z: ula->PressKey(3, 1, keyDown); break;
      case ALLEGRO_KEY_X: ula->PressKey(3, 2, keyDown); break;
      case ALLEGRO_KEY_C: ula->PressKey(3, 3, keyDown); break;
      case ALLEGRO_KEY_V: ula->PressKey(3, 4, keyDown); break;
      case ALLEGRO_KEY_B: ula->PressKey(3, 5, keyDown); break;
      case ALLEGRO_KEY_N: ula->PressKey(3, 6, keyDown); break;
      case ALLEGRO_KEY_M: ula->PressKey(3, 7, keyDown); break;
      case ALLEGRO_KEY_RSHIFT: ula->PressKey(3, 8, keyDown); break;
      case ALLEGRO_KEY_SPACE:  ula->PressKey(3, 9, keyDown); break;
    }
  }
}

int main(int argc, char *argv[]) {

  ALLEGRO_DISPLAY*       display     = NULL;
  ALLEGRO_BITMAP*        bitmap      = NULL;
  ALLEGRO_EVENT_QUEUE*   event_queue = NULL;
  ALLEGRO_SAMPLE*        sample      = NULL;
  ALLEGRO_LOCKED_REGION* lock;

  // Components
  Z80 cpu;
  ULA ula;
  Bus16 bus;
  ROM<0, 16384> rom;
  RAM<32768, 32*1024> ram;

  // Check arguments
  if(argc != 2) {
    printf("Error: wrong parameters. Usage: gomas tap_file\n");
    exit(-1);
  }

  // Open tap file
  ifstream input(argv[1], std::ios::binary);
  if(!input) {
    printf("Error: rom does not exists!\n");
    exit(-1);
  }

  // allegro initializations
  if(!al_init()) {
    printf("Error: failed to initialize allegro!\n");
    return -1;
  }

  if(!al_install_keyboard()) {
    printf("Error: failed to initialize keyboard!\n");
    return -1;
  }

  if(!al_init_primitives_addon()) {
    printf("Error: failed to initialize allegro primitives!\n");
    return -1;
  }

  al_set_new_display_flags(ALLEGRO_WINDOWED);
  display = al_create_display(640, 480);
  if(!display) {
    printf("Error: failed to create display!\n");
    return -1;
  }

  bitmap = al_create_bitmap(320, 240);
  if(!bitmap) {
    printf("Error: failed to create bitmap!\n");
    al_destroy_display(display);
    return -1;
  }

  if(!al_install_audio()) {
    printf("Error: failed to initialize audio!\n");
    return -1;
  }

  if(!al_init_acodec_addon()) {
    printf("Error: failed to initialize audio codecs!\n");
    return -1;
  }

  if(!al_reserve_samples(1)) {
    printf("Error: failed to reserve samples!\n");
    return -1;
  }

  event_queue = al_create_event_queue();
  if(!event_queue) {
    printf("Error: failted to create event_queue!\n");
    al_destroy_bitmap(bitmap);
    al_destroy_display(display);
    return -1;
  }

  sample = al_create_sample(&ula.FrameAudio,
                            SAMPLES_PER_FRAME,
                            AUDIO_SAMPLE_RATE,
                            ALLEGRO_AUDIO_DEPTH_INT16,
                            ALLEGRO_CHANNEL_CONF_1,
                            false);

  if(!sample) {
    printf("Audio clip sample not loaded!\n");
    return -1;
  }

  ALLEGRO_SAMPLE_INSTANCE* audioInst = al_create_sample_instance(sample);
  al_attach_sample_instance_to_mixer(audioInst, al_get_default_mixer());

  al_register_event_source(event_queue, al_get_keyboard_event_source());

  al_set_target_bitmap(bitmap);
  al_clear_to_color(al_map_rgb(0xD7, 0xD7, 0xD7));
  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_bitmap(bitmap, 0, 0, 0);
  al_flip_display();

  // Load ROM contents
  if(rom.Load("48.rom")) {
    printf("Error: Unable to load rom\n");
    return(-1);
  }
  
  // Populate busses
  bus.AddBusComponent(&rom);
  bus.AddBusComponent(&ram);
  bus.AddBusComponent((ULAMemory*)&ula);

  // And attach busses to cpu core
  cpu.DataBus = &bus;
  cpu.IOBus = (BusComponent<0xFE, 1>*)((ULAIO*)&ula);

  // Add bitmap to ULA
  ula.SetBitmap(bitmap);

  cpu.regs.PC = 0;

  // Set bitmap before main loop for first iteration
  lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
  al_set_target_bitmap(bitmap);

  // Read the next tape block
  vector<unsigned char>::iterator block;
  vector<unsigned char> data((std::istreambuf_iterator<char>(input)),
                             (std::istreambuf_iterator<char>()));

  block = data.begin();

  vector<unsigned char>::iterator block_copy = block;
  ReadTapFile(data, block_copy);

  // Main loop
  do {
    // Keyboard routine. Needs to be moved elsewhere
    if(!al_is_event_queue_empty(event_queue)) {
      ReadKeyboard(event_queue, &ula);
    }

    cpu.tStates = 0;
    
    if ((cpu.regs.PC == 0x056B)) {      
      if (LoadTrap(cpu, data, block) == 0) {
          printf("LoadTrap successful\n");
          // set up register for success
          cpu.regs.BC = 0xB001;
          cpu.regs.altAF = 0x0145;
          cpu.regs.CF = 1;
      } else {
        // set up registers for failure        
        cpu.regs.CF = 0;
      }
      // Return from the table block load routine
      cpu.regs.PC = 0x05e2;
    }

    // Emulate instructions
    cpu.EmulateOne();

    //PrintRegisters(cpu);

    bool irq = false;
    ula.AddCycles(cpu.tStates, irq);

    // 20ms has passed. Vertical screen sync triggered
    if (irq) {
      cpu.INT();

      if (ula.GetIsDirty()) {        

        al_unlock_bitmap(bitmap);
        al_set_target_bitmap(al_get_backbuffer(display));
        al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 0, 640, 480, 0);
       
        al_flip_display();
        
        lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
        al_set_target_bitmap(bitmap);
      }

      // Play audio but first wait for first sample to complete
      while(al_get_sample_instance_playing(audioInst));
      al_set_sample_instance_playing(audioInst, true);      
    }

  } while(true);

  al_destroy_display(display);
}

