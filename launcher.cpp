#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <xinput.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "engine/libretro/libretro-common/include/libretro.h"
#include "coop_hud.h"
#include "debug_menu.h"


struct Core {
 unsigned (*ready)(),(*paused)(),(*debug_flags)(),(*level)(),(*menu_get)();
 int (*debug)(unsigned),(*debug_stage)(unsigned);void (*menu_set)(unsigned);
 unsigned (*state_size)(); int (*save)(void*,unsigned),(*restore)(const void*,unsigned);
 HMODULE dll;
 void (*init)(),(*run)(),(*deinit)();
 void (*set_environment)(retro_environment_t);
 void (*set_video_refresh)(retro_video_refresh_t);
 void (*set_audio_sample)(retro_audio_sample_t);
 void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
 void (*set_input_poll)(retro_input_poll_t);
 void (*set_input_state)(retro_input_state_t);
 void (*set_controller_port_device)(unsigned,unsigned);
 bool (*load_game)(const retro_game_info*);
 void (*get_system_av_info)(retro_system_av_info*);
 bool (*unserialize)(const void*,size_t);
 size_t (*serialize_size)();bool (*serialize)(void*,size_t);
 void (*enable)(unsigned),(*status)(unsigned*);
} core;
std::map<std::string,std::string> options;
std::vector<uint32_t> pixels;
std::vector<int16_t> sound;
std::vector<unsigned char> startState;
unsigned width=320,height=224,format=0,pads[2]={},details[10]={};
bool running=true,paused=false;int testFrames=0,frameNumber=0;

DebugMenu menu;
HWND windowHandle;
typedef DWORD (WINAPI *GetPad)(DWORD,XINPUT_STATE*);
GetPad getPad=nullptr;
HWAVEOUT audioDevice;
WAVEHDR headers[12]={};std::vector<int16_t> audioBuffers[12];int audioSlot=0;
struct Action {int first,last,player,button;};std::vector<Action> actions;

bool environment(unsigned cmd,void *data) {
 switch(cmd) {
 case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
 case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char**)data=".";return true;
 case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: format=*(int*)data;return format<=2;
 case RETRO_ENVIRONMENT_GET_VARIABLE: {
  auto *v=(retro_variable*)data;auto i=options.find(v->key);
  v->value=i==options.end()?nullptr:i->second.c_str();return v->value!=nullptr;
 }
 case RETRO_ENVIRONMENT_SET_VARIABLES:
  for(auto *v=(retro_variable*)data;v->key;v++) {
   std::string s=v->value;auto pos=s.find(';');if(pos==std::string::npos)continue;
   s=s.substr(pos+1);while(!s.empty()&&s[0]==' ')s.erase(0,1);
   options.emplace(v->key,s.substr(0,s.find('|')));
  }return true;
 case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool*)data=false;return true;
 case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool*)data=true;return true;
 case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return true;
 case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned*)data=0;return true;
 case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
 case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
 case RETRO_ENVIRONMENT_SET_GEOMETRY:
 case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: return true;
 default:return false;
 }
}
void video(const void *data,unsigned w,unsigned h,size_t pitch) {
 if(!data)return;width=w;height=h;pixels.resize(w*h);
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
  uint32_t rgb;
  if(format==RETRO_PIXEL_FORMAT_XRGB8888)rgb=((const uint32_t*)((const char*)data+y*pitch))[x];
  else {
   unsigned v=((const uint16_t*)((const char*)data+y*pitch))[x];unsigned r,g,b;
   if(format==RETRO_PIXEL_FORMAT_RGB565){r=v>>11;g=((v>>5)&63)*255/63;}
   else {r=(v>>10)&31;g=((v>>5)&31)*255/31;}
   b=v&31;rgb=((r*255/31)<<16)|(g<<8)|(b*255/31);
  }pixels[y*w+x]=rgb;
 }
}
size_t audioBatch(const int16_t *p,size_t n){sound.insert(sound.end(),p,p+n*2);return n;}
void audioSample(int16_t l,int16_t r){int16_t p[]={l,r};audioBatch(p,1);}
void inputPoll(){}
int16_t inputState(unsigned port,unsigned device,unsigned index,unsigned id) {
 if(port>1||device!=RETRO_DEVICE_JOYPAD)return 0;
 return id==RETRO_DEVICE_ID_JOYPAD_MASK?pads[port]:((pads[port]>>id)&1);
}
void readInputs() {
 pads[0]=pads[1]=0;
 const int keys[2][16]={{'F','G',0,VK_RETURN,'W','S','A','D',VK_SPACE,0,'X',0,0,0,0,0},
                       {'J','K',0,0,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,'L',0,'O',0,0,0,0,0}};
 bool focused=GetForegroundWindow()==windowHandle;
 if(!testFrames&&focused)for(int p=0;p<2;p++)for(int b=0;b<16;b++)
  if(keys[p][b]&&(GetAsyncKeyState(keys[p][b])&0x8000))pads[p]|=1<<b;
 if(!testFrames&&focused&&getPad)for(int p=0;p<2;p++) {
  XINPUT_STATE s={};if(getPad(p,&s)!=ERROR_SUCCESS)continue;
  WORD b=s.Gamepad.wButtons;auto set=[&](bool v,int n){if(v)pads[p]|=1<<n;};
  set((b&XINPUT_GAMEPAD_DPAD_UP)||s.Gamepad.sThumbLY>12000,4);
  set((b&XINPUT_GAMEPAD_DPAD_DOWN)||s.Gamepad.sThumbLY< -12000,5);
  set((b&XINPUT_GAMEPAD_DPAD_LEFT)||s.Gamepad.sThumbLX< -12000,6);
  set((b&XINPUT_GAMEPAD_DPAD_RIGHT)||s.Gamepad.sThumbLX>12000,7);
  set(b&XINPUT_GAMEPAD_X,0);set(b&XINPUT_GAMEPAD_B,1);
  set(b&XINPUT_GAMEPAD_START,3);set(b&XINPUT_GAMEPAD_A,8);set(b&XINPUT_GAMEPAD_Y,10);
 }
 for(auto a:actions)if(frameNumber>=a.first&&frameNumber<a.last&&a.player>=0&&a.player<2&&a.button>=0&&a.button<16)
  pads[a.player]|=1<<a.button;

}
bool restartLevel() {
 menu=DebugMenu{};core.enable(0);
 bool ok=core.unserialize(startState.data(),startState.size());
 if(ok)core.enable(1);
 paused=false;memset(details,0,sizeof(details));
 if(audioDevice)waveOutReset(audioDevice);
 return ok;
}
void paint(HDC dc) {
 RECT r;GetClientRect(windowHandle,&r);FillRect(dc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(235,235,235));
 const char *p1="P1: WASD move | F attack | G transform | Space jump";
 const char *p2="P2: Arrows move | J attack | K transform | L jump";
 TextOutA(dc,12,10,p1,strlen(p1));TextOutA(dc,12,30,p2,strlen(p2));
 int w=r.right-24,h=w*3*coopDisplayHeight/(4*coopGameHeight);
 if(h>r.bottom-110){h=std::max(1L,r.bottom-110);w=h*4*coopGameHeight/(3*coopDisplayHeight);}
 if(!pixels.empty()) {
  auto output=coopDisplayFrame(pixels,width,height);drawCoopHud(output,coopDisplayWidth,coopDisplayHeight,details);menu.draw(output,coopDisplayWidth,coopDisplayHeight,core.debug_flags());
  BITMAPINFO b={};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=coopDisplayWidth;
  b.bmiHeader.biHeight=-(int)coopDisplayHeight;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;
  SetStretchBltMode(dc,COLORONCOLOR);
  StretchDIBits(dc,(r.right-w)/2,58,w,h,0,0,coopDisplayWidth,coopDisplayHeight,output.data(),&b,DIB_RGB_COLORS,SRCCOPY);
 }
 char text[256];snprintf(text,sizeof(text),"P1 LIFE %u  POW %u | P2 LIFE %u  POW %u | F5 reset | F6 save | F8 load | F9 pause",
  details[6],details[8],details[7],details[9]);
 TextOutA(dc,12,r.bottom-30,text,strlen(text));
}
bool saveGame();
bool loadGame();
LRESULT CALLBACK windowProc(HWND w,UINT m,WPARAM a,LPARAM b) {
 switch(m) {
 case WM_DESTROY:running=false;return 0;
 case WM_KEYDOWN:
  if(a==VK_ESCAPE){DestroyWindow(w);return 0;}
  if(!(b&(1L<<30))){
   if(a==VK_F9){paused=!paused;if(audioDevice)waveOutReset(audioDevice);InvalidateRect(w,nullptr,FALSE);}
   if(a==VK_F5)restartLevel();
   if(a==VK_F6 && !saveGame())MessageBoxA(w,"Could not save game.","Save",MB_ICONERROR);
   if(a==VK_F8 && !loadGame())MessageBoxA(w,"No compatible save found.","Load",MB_ICONERROR);
  }return 0;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);paint(dc);EndPaint(w,&ps);return 0;}
 }return DefWindowProc(w,m,a,b);
}
void outputAudio() {
 if(!audioDevice||sound.empty())return;auto &h=headers[audioSlot];
 if(h.dwFlags&WHDR_PREPARED){if(!(h.dwFlags&WHDR_DONE))return;waveOutUnprepareHeader(audioDevice,&h,sizeof(h));}
 auto &v=audioBuffers[audioSlot];v=sound;h={};h.lpData=(LPSTR)v.data();h.dwBufferLength=v.size()*2;
 waveOutPrepareHeader(audioDevice,&h,sizeof(h));waveOutWrite(audioDevice,&h,sizeof(h));audioSlot=(audioSlot+1)%12;
}
std::vector<unsigned char> readFile(const char *path) {
 FILE*f=fopen(path,"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
 if(n<=0){fclose(f);return {};}
 std::vector<unsigned char> v(n);bool ok=fread(v.data(),1,v.size(),f)==v.size();fclose(f);return ok?v:std::vector<unsigned char>{};
}

bool saveGame() {
 auto n=core.serialize_size();unsigned extra=core.state_size();
 std::vector<unsigned char> data(16+n+extra);uint32_t header[]={0x53483343,1,(uint32_t)n,extra};
 memcpy(data.data(),header,16);
 core.menu_set(menu.pack());
 if(!core.serialize(data.data()+16,n)||!core.save(data.data()+16+n,extra))return false;
 FILE *f=fopen("coop-save.tmp","wb");if(!f)return false;
 bool ok=fwrite(data.data(),1,data.size(),f)==data.size();if(fclose(f))ok=false;
 return ok && MoveFileExA("coop-save.tmp","coop-save.state",MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
bool loadGame() {
 auto data=readFile("coop-save.state");if(data.size()<16)return false;
 uint32_t h[4];memcpy(h,data.data(),16);
 if(h[0]!=0x53483343||h[1]!=1||h[2]!=core.serialize_size()||h[3]!=core.state_size()||data.size()!=16ull+h[2]+h[3])return false;
 if(!core.unserialize(data.data()+16,h[2])||!core.restore(data.data()+16+h[2],h[3]))return false;
 menu.unpack(core.menu_get());if(audioDevice)waveOutReset(audioDevice);return true;
}

bool correctRom(const std::vector<unsigned char>& rom) {
 if(rom.size()!=2097152)return false;
 unsigned char digest[32];BCRYPT_ALG_HANDLE alg=nullptr;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
 BCRYPT_HASH_HANDLE hash=nullptr;DWORD objectSize=0,returned=0;
 auto result=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objectSize,sizeof(objectSize),&returned,0);
 std::vector<unsigned char> object(objectSize);
 if(result>=0)result=BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0);
 if(result>=0)result=BCryptHashData(hash,(PUCHAR)rom.data(),(ULONG)rom.size(),0);
 if(result>=0)result=BCryptFinishHash(hash,digest,sizeof(digest),0);
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);if(result<0)return false;
 char hex[65];for(int i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);
 return !strcmp(hex,"8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710");
}
int fail(const char *message) {fprintf(stderr,"%s\n",message);if(!testFrames)MessageBoxA(nullptr,message,"Splatterhouse3 co-op",MB_ICONERROR);return 1;}
int main(int argc,char **argv) {
 char path[MAX_PATH];GetModuleFileNameA(nullptr,path,MAX_PATH);char *slash=strrchr(path,'\\');if(slash){*slash=0;SetCurrentDirectoryA(path);}
 const char *script=nullptr;
 for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--test")&&i+1<argc)testFrames=atoi(argv[++i]);else if(!strcmp(argv[i],"--script")&&i+1<argc)script=argv[++i];}
 if(script){FILE*f=fopen(script,"r");Action a;if(!f)return fail("Input script missing.");while(fscanf(f,"%d %d %d %d",&a.first,&a.last,&a.player,&a.button)==4)actions.push_back(a);fclose(f);}
 auto rom=readFile("Splatterhouse 3 (USA).md");if(!correctRom(rom))return fail("The original Splatterhouse3 USA ROM is missing or differs from the supported version.");
 
 core.dll=LoadLibraryA("engine\\genesis_plus_gx_libretro.dll");if(!core.dll)return fail("Unable to load the experimental emulator core.");
#define LOAD(name) core.name=(decltype(core.name))GetProcAddress(core.dll,"retro_" #name);if(!core.name)return fail("Missing core export: " #name);
 LOAD(init);LOAD(run);LOAD(deinit);LOAD(set_environment);LOAD(set_video_refresh);LOAD(set_audio_sample);LOAD(set_audio_sample_batch);LOAD(set_input_poll);LOAD(set_input_state);LOAD(set_controller_port_device);LOAD(load_game);LOAD(get_system_av_info);LOAD(unserialize);LOAD(serialize);LOAD(serialize_size);
#undef LOAD
#define LOAD(name) core.name=(decltype(core.name))GetProcAddress(core.dll,"sh_coop_" #name);if(!core.name)return fail("Missing co-op export: " #name);
 LOAD(debug);LOAD(debug_flags);LOAD(debug_stage);LOAD(level);LOAD(menu_get);LOAD(menu_set);LOAD(paused);LOAD(ready);LOAD(enable);LOAD(status);LOAD(state_size);LOAD(save);LOAD(restore);
#undef LOAD
 core.set_environment(environment);core.set_video_refresh(video);core.set_audio_sample(audioSample);core.set_audio_sample_batch(audioBatch);core.set_input_poll(inputPoll);core.set_input_state(inputState);core.init();
 retro_game_info game={"Splatterhouse 3 (USA).md",rom.data(),rom.size(),nullptr};if(!core.load_game(&game))return fail("ROM could not be loaded.");
 core.set_controller_port_device(0,RETRO_DEVICE_JOYPAD);core.set_controller_port_device(1,RETRO_DEVICE_JOYPAD);
 startState.resize(core.serialize_size());
 if(!core.serialize(startState.data(),startState.size())||!restartLevel())return fail("Game could not start.");
 retro_system_av_info av={};core.get_system_av_info(&av);
 if(!testFrames) {
  SetProcessDPIAware();WNDCLASSA wc={};wc.lpfnWndProc=windowProc;wc.hInstance=GetModuleHandle(nullptr);wc.lpszClassName="Splatterhouse3Coop";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassA(&wc);
  windowHandle=CreateWindowA(wc.lpszClassName,"Splatterhouse 3 - Co-op prototype",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1000,850,nullptr,nullptr,wc.hInstance,nullptr);
  HMODULE xi=LoadLibraryA("xinput1_4.dll");if(!xi)xi=LoadLibraryA("xinput9_1_0.dll");if(xi)getPad=(GetPad)GetProcAddress(xi,"XInputGetState");
  WAVEFORMATEX wf={};wf.wFormatTag=WAVE_FORMAT_PCM;wf.nChannels=2;wf.nSamplesPerSec=(DWORD)av.timing.sample_rate;wf.wBitsPerSample=16;wf.nBlockAlign=4;wf.nAvgBytesPerSec=wf.nSamplesPerSec*4;
  waveOutOpen(&audioDevice,WAVE_MAPPER,&wf,0,0,CALLBACK_NULL);timeBeginPeriod(1);
 }
 LARGE_INTEGER freq,now;QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&now);double deadline=double(now.QuadPart)/freq.QuadPart;
 while(running&&(!testFrames||frameNumber<testFrames)) {
  MSG msg;while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessage(&msg);}
  if(!running)break;
  if(paused&&!testFrames){Sleep(15);QueryPerformanceCounter(&now);deadline=double(now.QuadPart)/freq.QuadPart;continue;}
  readInputs();sound.clear();
  if(!core.ready()){menu=DebugMenu{};menu.previous=pads[0]|pads[1];core.run();}
  else if(!menu.update(pads[0]|pads[1],core.debug,core.debug_stage,core.level(),core.paused()))core.run();
  else if(audioDevice)waveOutReset(audioDevice);
  core.menu_set(menu.pack());
  core.status(details);
  if(!testFrames) {
   outputAudio();InvalidateRect(windowHandle,nullptr,FALSE);UpdateWindow(windowHandle);
   deadline+=1.0/av.timing.fps;QueryPerformanceCounter(&now);double remaining=deadline-double(now.QuadPart)/freq.QuadPart;
   if(remaining>0)Sleep((DWORD)(remaining*1000));else if(remaining< -0.2)deadline=double(now.QuadPart)/freq.QuadPart;
  }frameNumber++;
 }
 if(testFrames) {
  pixels=coopDisplayFrame(pixels,width,height);width=coopDisplayWidth;height=coopDisplayHeight;
  drawCoopHud(pixels,width,height,details);menu.draw(pixels,width,height,core.debug_flags());
  unsigned status[10];core.status(status);FILE*f=fopen("diagnostics/launcher-status.txt","w");
  if(f){for(auto v:status)fprintf(f,"%u ",v);fprintf(f,"\n");for(auto v:details)fprintf(f,"%u ",v);fclose(f);}
  BITMAPFILEHEADER bf={};BITMAPINFOHEADER bi={};bi.biSize=sizeof(bi);bi.biWidth=width;bi.biHeight=-(int)height;bi.biPlanes=1;bi.biBitCount=32;bi.biSizeImage=pixels.size()*4;bf.bfType=0x4d42;bf.bfOffBits=sizeof(bf)+sizeof(bi);bf.bfSize=bf.bfOffBits+bi.biSizeImage;
  f=fopen("diagnostics/launcher.bmp","wb");if(f){fwrite(&bf,sizeof(bf),1,f);fwrite(&bi,sizeof(bi),1,f);fwrite(pixels.data(),4,pixels.size(),f);fclose(f);}
 }
 if(audioDevice){waveOutReset(audioDevice);for(auto &h:headers)if(h.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(audioDevice,&h,sizeof(h));waveOutClose(audioDevice);}
 if(!testFrames)timeEndPeriod(1);core.deinit();FreeLibrary(core.dll);return 0;
}
