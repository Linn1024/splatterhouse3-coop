// BizHawk/Libretro frontend adapter for the emulator-assisted Splatterhouse3 prototype.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include "../engine/libretro/libretro-common/include/libretro.h"
#include "../coop_hud.h"
#include "../debug_menu.h"
namespace {
struct Engine {
 HMODULE dll=nullptr; bool initialized=false,loaded=false;
 void (*init)(),(*deinit)(),(*run)(),(*unload_game)();
 void (*set_environment)(retro_environment_t);void (*set_video_refresh)(retro_video_refresh_t);
 void (*set_audio_sample)(retro_audio_sample_t);void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
 void (*set_input_poll)(retro_input_poll_t);void (*set_input_state)(retro_input_state_t);
 void (*set_controller_port_device)(unsigned,unsigned);bool (*load_game)(const retro_game_info*);
 void (*get_system_av_info)(retro_system_av_info*);
 size_t (*serialize_size)();bool (*serialize)(void*,size_t);bool (*unserialize)(const void*,size_t);
 void (*enable)(unsigned),(*status)(unsigned*);
 unsigned (*ready)(),(*paused)(),(*debug_flags)(),(*level)(),(*menu_get)();
 int (*debug)(unsigned),(*debug_stage)(unsigned);void (*menu_set)(unsigned);
 unsigned (*state_size)();int (*save)(void*,unsigned),(*restore)(const void*,unsigned);
} c;
retro_environment_t env=nullptr;retro_video_refresh_t video=nullptr;
retro_audio_sample_t sample=nullptr;retro_audio_sample_batch_t audio=nullptr;
retro_input_poll_t poll=nullptr;retro_input_state_t input=nullptr;
std::wstring directory;std::map<std::string,std::string> opts;
std::vector<unsigned char> startup;std::vector<uint32_t> pixels;
unsigned format=0,width=320,height=224,pads[2]={};
retro_system_av_info av={};
DebugMenu menu;
constexpr unsigned displayWidth=coopDisplayWidth,displayHeight=coopDisplayHeight;
std::vector<uint32_t> displayFrame(){return coopDisplayFrame(pixels,width,height);}

void message(const char *text){if(env){retro_message m={text,240};env(RETRO_ENVIRONMENT_SET_MESSAGE,&m);}}
bool exists(const std::wstring& p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
std::wstring parent(std::wstring p){auto n=p.find_last_of(L"\\/");return n==std::wstring::npos?L"":p.substr(0,n+1);}
void locate(){
 HMODULE m=nullptr;wchar_t path[32768];
 GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)&locate,&m);
 GetModuleFileNameW(m,path,32768);directory=parent(path);
 if(exists(directory+L"splatterhouse_engine.dll"))return;
 const char *reported=nullptr;
 if(env&&env(RETRO_ENVIRONMENT_GET_LIBRETRO_PATH,&reported)&&reported){
  int n=MultiByteToWideChar(CP_UTF8,0,reported,-1,nullptr,0);std::wstring p(n,L'\0');
  MultiByteToWideChar(CP_UTF8,0,reported,-1,&p[0],n);p.resize(n-1);
  DWORD attr=GetFileAttributesW(p.c_str());
  p=(attr!=INVALID_FILE_ATTRIBUTES&&(attr&FILE_ATTRIBUTE_DIRECTORY))?p+L"\\":parent(p);
  if(exists(p+L"splatterhouse_engine.dll")){directory=p;return;}
 }
 // BizHawk 2.4 loads a renamed temporary copy of the outer DLL.
 GetModuleFileNameW(nullptr,path,32768);directory=parent(path)+L"Libretro\\Cores\\Splatterhouse3Coop\\";
}
bool correctRom(const retro_game_info *g){
 if(!g||!g->data||g->size!=2097152)return false;
 BCRYPT_ALG_HANDLE a=nullptr;BCRYPT_HASH_HANDLE h=nullptr;DWORD n=0,returned=0;unsigned char digest[32];
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
 NTSTATUS r=BCryptGetProperty(a,BCRYPT_OBJECT_LENGTH,(PUCHAR)&n,sizeof(n),&returned,0);
 std::vector<unsigned char> object(n);
 if(r>=0)r=BCryptCreateHash(a,&h,object.data(),n,nullptr,0,0);
 if(r>=0)r=BCryptHashData(h,(PUCHAR)g->data,(ULONG)g->size,0);
 if(r>=0)r=BCryptFinishHash(h,digest,32,0);
 if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(a,0);if(r<0)return false;
 char hex[65];for(int i=0;i<32;i++)sprintf(hex+i*2,"%02x",digest[i]);
 return !strcmp(hex,"8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710");
}
bool innerEnv(unsigned cmd,void *data){switch(cmd){
 case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:*(unsigned*)data=0;return true;
 case RETRO_ENVIRONMENT_SET_VARIABLES:
  for(auto *v=(retro_variable*)data;v->key;v++){std::string s=v->value;auto i=s.find(';');if(i!=std::string::npos){s=s.substr(i+1);while(!s.empty()&&s[0]==' ')s.erase(0,1);opts[v->key]=s.substr(0,s.find('|'));}}return true;
 case RETRO_ENVIRONMENT_GET_VARIABLE:{auto*v=(retro_variable*)data;auto i=opts.find(v->key);v->value=i==opts.end()?nullptr:i->second.c_str();return v->value!=nullptr;}
 case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:*(bool*)data=false;return true;
 case RETRO_ENVIRONMENT_GET_CAN_DUPE:*(bool*)data=true;return true;
 case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
 case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:format=*(unsigned*)data;return format<=2;
 case RETRO_ENVIRONMENT_SET_GEOMETRY:case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
 case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:return true;
 case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:return env&&env(cmd,data);
 default:return false;
}}
void innerVideo(const void *p,unsigned w,unsigned h,size_t pitch){
 if(!p)return;width=w;height=h;pixels.resize(w*h);
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  uint32_t rgb;if(format==RETRO_PIXEL_FORMAT_XRGB8888)rgb=((const uint32_t*)((const char*)p+y*pitch))[x];
  else {unsigned v=((const uint16_t*)((const char*)p+y*pitch))[x],r,g,b;
   if(format==RETRO_PIXEL_FORMAT_RGB565){r=(v>>11)&31;g=((v>>5)&63)*255/63;}else{r=(v>>10)&31;g=((v>>5)&31)*255/31;}
   b=v&31;rgb=((r*255/31)<<16)|(g<<8)|(b*255/31);
  }pixels[y*w+x]=rgb;
 }
}
size_t innerAudio(const int16_t *p,size_t n){if(audio)return audio(p,n);if(sample)for(size_t i=0;i<n;i++)sample(p[i*2],p[i*2+1]);return n;}
void innerSample(int16_t l,int16_t r){int16_t p[]={l,r};innerAudio(p,1);}
void innerPoll(){}
int16_t innerInput(unsigned p,unsigned device,unsigned,unsigned id){if(p>1||device!=RETRO_DEVICE_JOYPAD)return 0;return id==RETRO_DEVICE_ID_JOYPAD_MASK?pads[p]:((pads[p]>>id)&1);}
bool restart(){menu=DebugMenu{};c.enable(0);if(!c.unserialize(startup.data(),startup.size()))return false;c.enable(1);pads[0]=pads[1]=0;return true;}
void cleanup(){if(c.dll){if(c.loaded)c.unload_game();if(c.initialized)c.deinit();FreeLibrary(c.dll);}c=Engine{};menu=DebugMenu{};startup.clear();pixels.clear();opts.clear();}
}
extern "C" {
RETRO_API unsigned retro_api_version(){return RETRO_API_VERSION;}
RETRO_API void retro_set_environment(retro_environment_t cb){env=cb;
 static retro_input_descriptor d[]={
 {0,1,0,3,"P1 Pause"},{1,1,0,3,"P2 Pause"},{0,1,0,4,"P1 Up"},{0,1,0,5,"P1 Down"},{0,1,0,6,"P1 Left"},{0,1,0,7,"P1 Right"},
 {0,1,0,0,"P1 Attack"},{0,1,0,1,"P1 Transform"},{0,1,0,8,"P1 Jump"},
 {1,1,0,4,"P2 Up"},{1,1,0,5,"P2 Down"},{1,1,0,6,"P2 Left"},{1,1,0,7,"P2 Right"},
 {1,1,0,0,"P2 Attack"},{1,1,0,1,"P2 Transform"},{1,1,0,8,"P2 Jump"},{0,0,0,0,nullptr}};
 if(env)env(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS,d);
}
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb){video=cb;}
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb){sample=cb;}
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb){audio=cb;}
RETRO_API void retro_set_input_poll(retro_input_poll_t cb){poll=cb;}
RETRO_API void retro_set_input_state(retro_input_state_t cb){input=cb;}
RETRO_API void retro_get_system_info(retro_system_info *i){*i={"Splatterhouse3 Campaign Co-op","0.2","bin|gen|md",false,false};}
RETRO_API void retro_init(){locate();}
RETRO_API void retro_deinit(){cleanup();}
RETRO_API void retro_set_controller_port_device(unsigned,unsigned){}
RETRO_API bool retro_load_game(const retro_game_info *game){
 cleanup();locate();if(!correctRom(game)){message("Splatterhouse3 co-op requires the supported, unmodified USA ROM.");return false;}
 c.dll=LoadLibraryW((directory+L"splatterhouse_engine.dll").c_str());if(!c.dll){message("Missing splatterhouse_engine.dll beside the adapter.");return false;}
#define LOAD(n) c.n=(decltype(c.n))GetProcAddress(c.dll,"retro_" #n);if(!c.n){cleanup();return false;}
 LOAD(init);LOAD(deinit);LOAD(run);LOAD(unload_game);LOAD(set_environment);LOAD(set_video_refresh);LOAD(set_audio_sample);LOAD(set_audio_sample_batch);LOAD(set_input_poll);LOAD(set_input_state);LOAD(set_controller_port_device);LOAD(load_game);LOAD(get_system_av_info);LOAD(serialize_size);LOAD(serialize);LOAD(unserialize);
#undef LOAD
#define LOAD(n) c.n=(decltype(c.n))GetProcAddress(c.dll,"sh_coop_" #n);if(!c.n){cleanup();return false;}
 LOAD(debug);LOAD(debug_flags);LOAD(debug_stage);LOAD(level);LOAD(menu_get);LOAD(menu_set);LOAD(paused);LOAD(ready);LOAD(enable);LOAD(status);LOAD(state_size);LOAD(save);LOAD(restore);
#undef LOAD
 c.set_environment(innerEnv);c.set_video_refresh(innerVideo);c.set_audio_sample(innerSample);c.set_audio_sample_batch(innerAudio);c.set_input_poll(innerPoll);c.set_input_state(innerInput);c.init();c.initialized=true;
 if(!c.load_game(game)){cleanup();return false;}c.loaded=true;
 c.set_controller_port_device(0,RETRO_DEVICE_JOYPAD);c.set_controller_port_device(1,RETRO_DEVICE_JOYPAD);
 startup.resize(c.serialize_size());
 if(!c.serialize(startup.data(),startup.size())||!restart()){message("Splatterhouse3 could not start.");cleanup();return false;}
 c.get_system_av_info(&av);av.geometry={displayWidth,displayHeight,displayWidth,displayHeight,(4.0f/3.0f)*coopGameHeight/coopDisplayHeight};
 retro_pixel_format fmt=RETRO_PIXEL_FORMAT_XRGB8888;if(!env||!env(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,&fmt)){cleanup();return false;}
 return true;
}
RETRO_API bool retro_load_game_special(unsigned,const retro_game_info*,size_t){return false;}
RETRO_API void retro_unload_game(){cleanup();}
RETRO_API void retro_get_system_av_info(retro_system_av_info*i){*i=av;}
RETRO_API unsigned retro_get_region(){return RETRO_REGION_NTSC;}
RETRO_API void retro_reset(){if(c.loaded&&!restart())message("Splatterhouse3 restart failed.");}
RETRO_API void retro_run(){
 if(!c.loaded)return;
 if(poll)poll();
 for(unsigned p=0;p<2;p++){pads[p]=0;if(input)for(unsigned b=0;b<16;b++)if(input(p,RETRO_DEVICE_JOYPAD,0,b))pads[p]|=1<<b;}
 if(!c.ready()){menu=DebugMenu{};menu.previous=pads[0]|pads[1];c.run();}
 else if(!menu.update(pads[0]|pads[1],c.debug,c.debug_stage,c.level(),c.paused()))c.run();
 else {static const int16_t silence[1472]={};innerAudio(silence,736);}
 c.menu_set(menu.pack());
 if(video&&!pixels.empty()){unsigned status[10];c.status(status);auto output=displayFrame();drawCoopHud(output,displayWidth,displayHeight,status);menu.draw(output,displayWidth,displayHeight,c.debug_flags());video(output.data(),displayWidth,displayHeight,displayWidth*4);}
}
// Include the last video frame for immediate display after loading.
RETRO_API size_t retro_serialize_size(){return c.loaded?32+c.serialize_size()+c.state_size()+320*240*4:0;}
RETRO_API bool retro_serialize(void *data,size_t size){
 if(!c.loaded||!data||size<retro_serialize_size()||width>320||height>240)return false;
 uint32_t h[8]={0x53484248,1,(uint32_t)c.serialize_size(),c.state_size(),0,width,height,0};memcpy(data,h,32);
 auto*p=(unsigned char*)data+32;
 c.menu_set(menu.pack());
 if(!c.serialize(p,h[2])||!c.save(p+h[2],h[3]))return false;
 p+=h[2]+h[3];memset(p,0,320*240*4);if(pixels.size()==width*height)memcpy(p,pixels.data(),pixels.size()*4);return true;
}
RETRO_API bool retro_unserialize(const void *data,size_t size){
 if(!c.loaded||!data||size<32)return false;uint32_t h[8];memcpy(h,data,32);
 if(h[0]!=0x53484248||h[1]!=1||h[2]!=c.serialize_size()||h[3]!=c.state_size()||h[4]||!h[5]||h[5]>320||!h[6]||h[6]>240||h[7]||size<retro_serialize_size())return false;
 auto*p=(const unsigned char*)data+32;
 // Reject malformed extension before mutating either half; roll back on a core failure.
 std::vector<unsigned char> oldCore(c.serialize_size()),oldExtra(c.state_size());
 if(!c.serialize(oldCore.data(),oldCore.size())||!c.save(oldExtra.data(),oldExtra.size()))return false;
 if(!c.restore(p+h[2],h[3]))return false;
 if(!c.unserialize(p,h[2])){c.unserialize(oldCore.data(),oldCore.size());c.restore(oldExtra.data(),oldExtra.size());return false;}
 menu.unpack(c.menu_get());width=h[5];height=h[6];pixels.resize(width*height);memcpy(pixels.data(),p+h[2]+h[3],pixels.size()*4);return true;
}
RETRO_API void retro_cheat_reset(){}
RETRO_API void retro_cheat_set(unsigned,bool,const char*){}
RETRO_API void *retro_get_memory_data(unsigned){return nullptr;}
RETRO_API size_t retro_get_memory_size(unsigned){return 0;}
}
