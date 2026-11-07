#include <cassert>
#include <cstdio>
#include "../emulator.h"
#include "../joystick_input.h"

int main() {
  assert(al_init());std::string error;
  for(const char *rom:{"48.rom","128-0.rom","plus2-0.rom","plus3-0.rom"}) {
    SpectrumMachine machine;assert(machine.LoadROM(FindROM(rom),error));
    assert(machine.io.Read(0x1f)==machine.ula.IORead(0x1f));
    machine.io.kempstonEnabled=true;
    assert(machine.io.Read(0x1f)==0);
    machine.io.kempstonState=JoystickInput::Up|JoystickInput::Right|JoystickInput::Fire;
    assert(machine.io.Read(0x1f)==25 && machine.io.Read(0xab1f)==25);
    for(unsigned port:{0x1fu,0x5fu,0x9fu,0xdfu,0x01dfu,0xabdfu})assert(machine.io.Read(port)==25);
    assert(machine.io.Read(0x1e)==(machine.ula.IORead(0x1e)&25));
    // Navy Moves reads IN A,(C) with BC=0x01DF / 0x02DF. Neutral must
    // return zero rather than floating-bus bits that trigger jump and fire.
    machine.bus.Write(0x8100,0xed);machine.bus.Write(0x8101,0x78);
    machine.io.kempstonState=0;machine.cpu.regs.PC=0x8100;machine.cpu.regs.BC=0x01df;
    machine.StepInstruction();assert(machine.cpu.regs.A==0);
    machine.io.kempstonState=25;machine.cpu.regs.PC=0x8100;machine.cpu.regs.BC=0x02df;
    machine.StepInstruction();assert(machine.cpu.regs.A==25);
    machine.bus.Write(0x8000,0xdb);machine.bus.Write(0x8001,0x1f);
    machine.cpu.regs.PC=0x8000;machine.cpu.regs.A=0x91;machine.StepInstruction();
    assert(machine.cpu.regs.A==25);
    assert(machine.io.Read(0xfefe)==machine.ula.IORead(0xfefe));
    assert(machine.io.Read(0xff)==machine.ula.IORead(0xff));
    machine.io.kempstonState=255;assert(machine.io.Read(0x1f)==31);
    machine.io.kempstonEnabled=false;assert(machine.io.Read(0x1f)==machine.ula.IORead(0x1f));
  }
  ULA ula;JoystickInput input;
  const unsigned states[]={JoystickInput::Left,JoystickInput::Right,JoystickInput::Down,JoystickInput::Up,JoystickInput::Fire};
  for(auto mode:{JoystickMode::Sinclair1,JoystickMode::Sinclair2})for(unsigned n=0;n<5;++n) {
    input.Release();input.mode=mode;input.Gamepad(0);input.Gamepad(states[n]);input.ApplyKeyboard(ula);
    unsigned row=mode==JoystickMode::Sinclair1?0xeffe:0xf7fe;
    unsigned bit=mode==JoystickMode::Sinclair1?4-n:n;
    assert((~ula.IORead(row)&31)==(1u<<bit));
    assert((ula.IORead(0xfefe)&1)==1); // Sinclair does not press CAPS SHIFT.
    input.Gamepad(0);input.ApplyKeyboard(ula);assert((ula.IORead(row)&31)==31);
  }
  input.mode=JoystickMode::Sinclair1;input.Gamepad(JoystickInput::Left);
  input.keys[ALLEGRO_KEY_6]=true;input.ApplyKeyboard(ula);
  input.Gamepad(0);input.ApplyKeyboard(ula);assert((ula.IORead(0xeffe)&16)==0);
  input.Gamepad(JoystickInput::Left);input.keys[ALLEGRO_KEY_6]=false;input.ApplyKeyboard(ula);
  assert((ula.IORead(0xeffe)&16)==0);
  input.Gamepad(0);input.ApplyKeyboard(ula);assert((ula.IORead(0xeffe)&31)==31);

  input.mode=JoystickMode::Kempston;input.keyboard=true;
  input.keys[ALLEGRO_KEY_UP]=input.keys[ALLEGRO_KEY_RIGHT]=input.keys[ALLEGRO_KEY_RCTRL]=true;
  assert(input.State()==25);input.ApplyKeyboard(ula);
  assert((ula.IORead(0xfefe)&1)==1 && (ula.IORead(0xeffe)&31)==31);
  input.keys[ALLEGRO_KEY_LEFT]=input.keys[ALLEGRO_KEY_DOWN]=true;assert(input.State()==JoystickInput::Fire);
  input.Release();input.Gamepad(25);assert(input.State()==0); // Wait for neutral after focus loss.
  input.SuspendGamepad();input.Gamepad(25);assert(input.State()==0);
  input.Gamepad(0);input.Gamepad(25);assert(input.State()==25);
  input.Release();input.mode=JoystickMode::Disabled;input.keys[ALLEGRO_KEY_LEFT]=true;input.ApplyKeyboard(ula);
  assert((ula.IORead(0xfefe)&1)==0 && (ula.IORead(0xf7fe)&16)==0); // Original Spectrum cursor.
  input.Release();input.ApplyKeyboard(ula);assert((ula.IORead(0xfefe)&1)==1);
  assert(JoystickInput::Directions(.2f,-.2f)==0);
  assert(JoystickInput::Directions(.8f,-.8f)==9);
  // DirectInput sets BOTH capability flags on analogue sticks. An idle right
  // trackpad/stick and trigger values must not drive the Spectrum joystick.
  ALLEGRO_JOYSTICK_STATE pad{};
  JoystickHost::Stick layout[]={
    {3,ALLEGRO_JOYFLAG_DIGITAL|ALLEGRO_JOYFLAG_ANALOGUE},
    {3,ALLEGRO_JOYFLAG_DIGITAL|ALLEGRO_JOYFLAG_ANALOGUE},
    {2,ALLEGRO_JOYFLAG_DIGITAL|ALLEGRO_JOYFLAG_ANALOGUE,true},
    {2,ALLEGRO_JOYFLAG_DIGITAL}
  };
  pad.stick[0].axis[2]=-1; // A combined trigger in the primary stick's Z axis.
  pad.stick[1].axis[0]=.9f;pad.stick[1].axis[1]=-.9f;
  pad.stick[2].axis[0]=-1;pad.stick[2].axis[1]=-1;
  assert(JoystickHost::Movement(pad,layout,4)==0);
  pad.stick[3].axis[0]=-1;assert(JoystickHost::Movement(pad,layout,4)==JoystickInput::Left);
  pad.stick[3].axis[0]=0;pad.stick[0].axis[0]=.8f;
  assert(JoystickHost::Movement(pad,layout,4)==JoystickInput::Right);
  pad.stick[3].axis[1]=-1;assert(JoystickHost::Movement(pad,layout,4)==9);
  assert(JoystickHost::DpadButton("RIGHT DPAD")==JoystickInput::Right);
  assert(JoystickHost::DpadButton("LEFT DPAD")==JoystickInput::Left);
  assert(JoystickHost::DpadButton("DOWN DPAD")==JoystickInput::Down);
  assert(JoystickHost::DpadButton("D-pad Up")==JoystickInput::Up);
  assert(JoystickHost::DpadButton("A")==0 && JoystickHost::DpadButton(nullptr)==0);
  puts("Joystick: Kempston CPU/ports on four models, Sinclair matrix, keyboard merging, diagonals, dead zone and neutral-after-focus passed");
}
