// Shared Windows startup, display settings and keyboard/XInput controls.
#pragma once
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <xinput.h>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cstdio>
namespace coopdesktop {
static std::wstring root,data,ini,romFile,gameTitle,explicitRom;
static std::string romUtf8;
static bool testing=false,cancelled=false,full=false,attaching=false;
static HWND host=nullptr,controlWindow=nullptr;
static RECT normalRect{};
static unsigned frameWidth=256,frameHeight=256;
static int keys[2][16]{},capture=-1;
static unsigned padKeys[2][16]{},previousPad[2]{};
static DWORD (WINAPI *getPad)(DWORD,XINPUT_STATE*)=nullptr;
static std::function<bool(const std::vector<unsigned char>&)> validate;
static std::function<void()> externalControls;
struct Binding {int id;const wchar_t *label;};
static std::vector<Binding> bindings;
static std::wstring wide(const std::string&s){int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);std::wstring r(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&r[0],n);if(n)r.resize(n-1);return r;}
static std::string utf8(const std::wstring&s){int n=WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,nullptr,0,nullptr,nullptr);std::string r(n,'\0');WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,&r[0],n,nullptr,nullptr);if(n)r.resize(n-1);return r;}
static std::wstring readSetting(const wchar_t*section,const wchar_t*key,const wchar_t*fallback=L"") {wchar_t b[32768];GetPrivateProfileStringW(section,key,fallback,b,32768,ini.c_str());return b;}
static void writeSetting(const wchar_t*section,const wchar_t*key,const std::wstring&v){WritePrivateProfileStringW(section,key,v.c_str(),ini.c_str());}
static int number(const wchar_t*section,const wchar_t*key,int fallback){return GetPrivateProfileIntW(section,key,fallback,ini.c_str());}
static bool exists(const std::wstring&p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES && !(a&FILE_ATTRIBUTE_DIRECTORY);}
static std::wstring absolute(const std::wstring&p){wchar_t b[32768];DWORD n=GetFullPathNameW(p.c_str(),32768,b,nullptr);return n&&n<32768?std::wstring(b):p;}
static std::vector<unsigned char> readRom(const std::wstring&p){FILE*f=_wfopen(p.c_str(),L"rb");if(!f)return {};fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);std::vector<unsigned char>b;if(size>0 && size<=32*1024*1024){b.resize(size);if(fread(b.data(),1,b.size(),f)!=b.size())b.clear();}fclose(f);return b;}
static void init(const wchar_t*id,const wchar_t*title,bool test){
 SetProcessDPIAware();gameTitle=title;testing=test;wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);root=exe;root.resize(root.find_last_of(L"\\/"));
 int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::wstring custom;
 for(int i=1;i<argc;i++){if(!wcscmp(argv[i],L"--rom")&&i+1<argc)explicitRom=absolute(argv[++i]);else if(!wcscmp(argv[i],L"--settings-dir")&&i+1<argc)custom=absolute(argv[++i]);}
 LocalFree(argv);
 if(!custom.empty())data=custom;
 else if(testing)data=root+L"\\diagnostics\\desktop-test";
 else {wchar_t local[MAX_PATH];if(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,local)!=S_OK)local[0]=0;data=std::wstring(local)+L"\\Linn1024Coop\\"+id;}
 SHCreateDirectoryExW(nullptr,data.c_str(),nullptr);ini=data+L"\\launcher.ini";
 // A Unicode INI keeps ROM paths intact even outside the system code page.
 HANDLE settings=CreateFileW(ini.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(settings!=INVALID_HANDLE_VALUE){const unsigned char bom[]={255,254};DWORD written;WriteFile(settings,bom,2,&written,nullptr);CloseHandle(settings);}
 // Existing standalone controls and saves remain usable after this upgrade.
 if(!testing&&custom.empty()){
  CopyFileW((root+L"\\controls.ini").c_str(),(data+L"\\controls.ini").c_str(),TRUE);
  SHCreateDirectoryExW(nullptr,(data+L"\\saves").c_str(),nullptr);
  WIN32_FIND_DATAW f{};HANDLE find=FindFirstFileW((root+L"\\saves\\*").c_str(),&f);
  if(find!=INVALID_HANDLE_VALUE){do{if(!(f.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))CopyFileW((root+L"\\saves\\"+f.cFileName).c_str(),(data+L"\\saves\\"+f.cFileName).c_str(),TRUE);}while(FindNextFileW(find,&f));FindClose(find);}
  CopyFileW((root+L"\\coop-save.state").c_str(),(data+L"\\coop-save.state").c_str(),TRUE);
 }
 if(!testing)SetCurrentDirectoryW(data.c_str());
 HMODULE xi=LoadLibraryW(L"xinput1_4.dll");if(!xi)xi=LoadLibraryW(L"xinput9_1_0.dll");if(xi)getPad=(decltype(getPad))GetProcAddress(xi,"XInputGetState");
}
static std::wstring pickRom(){
 wchar_t path[32768]{};if(romFile.size()<32768)wcscpy(path,romFile.c_str());
 std::wstring title=L"Choose your original USA ROM - "+gameTitle;
 OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=host;ofn.lpstrTitle=title.c_str();
 ofn.lpstrFilter=L"Game ROMs (*.nes;*.smc;*.sfc;*.bin;*.gen;*.md)\0*.nes;*.smc;*.sfc;*.bin;*.gen;*.md\0All files\0*.*\0\0";
 ofn.lpstrFile=path;ofn.nMaxFile=32768;ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
 return GetOpenFileNameW(&ofn)?std::wstring(path):std::wstring();
}
static bool loadRom(std::vector<unsigned char>&out,const wchar_t*legacy,std::function<bool(const std::vector<unsigned char>&)>check){
 validate=check;romFile=!explicitRom.empty()?explicitRom:readSetting(L"Game",L"ROM");
 if(romFile.empty() && exists(root+L"\\"+legacy))romFile=root+L"\\"+legacy;
 for(;;){
  if(!romFile.empty()){out=readRom(romFile);if(validate(out)){romFile=absolute(romFile);romUtf8=utf8(romFile);writeSetting(L"Game",L"ROM",romFile);return true;}}
  if(testing)return false;
  if(!romFile.empty())MessageBoxW(host,L"This file is missing or is not the supported original USA ROM. Please choose the uncompressed ROM for this game.",gameTitle.c_str(),MB_OK|MB_ICONINFORMATION);
  romFile=pickRom();if(romFile.empty()){cancelled=true;return false;}
 }
}
static HMODULE loadCore(const wchar_t*relative){return LoadLibraryW((root+L"\\"+relative).c_str());}
static bool busy(){return controlWindow!=nullptr;}
static void geometry(unsigned w,unsigned h){if(w&&h){frameWidth=w;frameHeight=h;}}
static void saveWindow(){if(!host||full||attaching||IsIconic(host))return;RECT r{};GetClientRect(host,&r);if(r.right>0&&r.bottom>0){writeSetting(L"Display",L"Width",std::to_wstring(r.right));writeSetting(L"Display",L"Height",std::to_wstring(r.bottom));}}
static void fullscreen(){
 if(!host)return;full=!full;
 if(full){GetWindowRect(host,&normalRect);SetMenu(host,nullptr);SetWindowLongPtr(host,GWL_STYLE,WS_POPUP|WS_VISIBLE);MONITORINFO m{sizeof(m)};GetMonitorInfo(MonitorFromWindow(host,MONITOR_DEFAULTTONEAREST),&m);SetWindowPos(host,HWND_TOP,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED);}
 else {SetWindowLongPtr(host,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);}
 writeSetting(L"Display",L"Fullscreen",full?L"1":L"0");
}
static HMENU normalMenu=nullptr;
static void toggleFull(){bool was=full;fullscreen();if(was){SetMenu(host,normalMenu);SetWindowPos(host,nullptr,normalRect.left,normalRect.top,normalRect.right-normalRect.left,normalRect.bottom-normalRect.top,SWP_FRAMECHANGED|SWP_NOZORDER);}}
static void resizeClient(int width,int height){
 if(full)toggleFull();MONITORINFO m{sizeof(m)};GetMonitorInfo(MonitorFromWindow(host,MONITOR_DEFAULTTONEAREST),&m);
 RECT r{0,0,width,height};AdjustWindowRectEx(&r,WS_OVERLAPPEDWINDOW,TRUE,0);
 int w=std::min<int>(r.right-r.left,m.rcWork.right-m.rcWork.left),h=std::min<int>(r.bottom-r.top,m.rcWork.bottom-m.rcWork.top);
 SetWindowPos(host,nullptr,m.rcWork.left+(m.rcWork.right-m.rcWork.left-w)/2,m.rcWork.top+(m.rcWork.bottom-m.rcWork.top-h)/2,w,h,SWP_NOZORDER);
 saveWindow();
}
static std::wstring keyName(int key){if(!key)return L"Unbound";wchar_t b[80]{};unsigned scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);if(key>=VK_PRIOR&&key<=VK_DOWN)scan|=0x100;GetKeyNameTextW(scan<<16,b,80);return *b?b:L"Key "+std::to_wstring(key);}
static std::wstring padName(unsigned b){const unsigned ids[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768};const wchar_t*names[]={L"Up",L"Down",L"Left",L"Right",L"Start",L"Back",L"Left stick",L"Right stick",L"LB",L"RB",L"A",L"B",L"X",L"Y"};for(int i=0;i<14;i++)if(ids[i]==b)return names[i];return L"Unbound";}
static void refreshControls(){if(!controlWindow)return;for(int p=0;p<2;p++)for(unsigned r=0;r<bindings.size();r++){int id=bindings[r].id;SetDlgItemTextW(controlWindow,100+p*100+r,keyName(keys[p][id]).c_str());SetDlgItemTextW(controlWindow,120+p*100+r,padName(padKeys[p][id]).c_str());}}
static void saveBinding(int p,int id,bool pad){std::wstring section=std::wstring(p?L"P2":L"P1")+(pad?L"Gamepad":L"Keyboard");writeSetting(section.c_str(),std::to_wstring(id).c_str(),std::to_wstring(pad?padKeys[p][id]:keys[p][id]));capture=-1;refreshControls();SetDlgItemTextW(controlWindow,1,L"Choose a binding. Esc cancels; Delete clears it.");}
static LRESULT CALLBACK controlProc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(m==WM_COMMAND){int id=LOWORD(a);if(id>=100&&id<240){capture=id;SetDlgItemTextW(w,1,(id%100>=20)?L"Press a button on this player's controller. Esc cancels.":L"Press a keyboard key. Esc cancels; Delete clears.");SetFocus(w);}return 0;}
 if(m==WM_KEYDOWN && capture>=0){int p=(capture-100)/100,r=(capture-100)%100;bool pad=r>=20;r%=20;if(a==VK_ESCAPE){capture=-1;refreshControls();return 0;}if(a==VK_F2||a==VK_F5||a==VK_F6||a==VK_F8||a==VK_F9||a==VK_F11)return 0;if(a==VK_DELETE){if(pad)padKeys[p][bindings[r].id]=0;else keys[p][bindings[r].id]=0;saveBinding(p,bindings[r].id,pad);}else if(!pad){keys[p][bindings[r].id]=(int)a;saveBinding(p,bindings[r].id,false);}return 0;}
 if(m==WM_TIMER && getPad){for(int p=0;p<2;p++){XINPUT_STATE state{};unsigned now=getPad(p,&state)==ERROR_SUCCESS?state.Gamepad.wButtons:0,edge=now&~previousPad[p];previousPad[p]=now;if(capture>=0 && (capture-100)/100==p && (capture-100)%100>=20 && edge){int r=(capture-100)%100-20;padKeys[p][bindings[r].id]=edge&(~edge+1);saveBinding(p,bindings[r].id,true);}}return 0;}
 if(m==WM_CLOSE){DestroyWindow(w);return 0;}if(m==WM_DESTROY){KillTimer(w,1);controlWindow=nullptr;capture=-1;EnableWindow(host,TRUE);SetForegroundWindow(host);return 0;}
 return DefWindowProcW(w,m,a,b);
}
static void openControls(){
 if(externalControls){externalControls();return;}if(controlWindow){SetForegroundWindow(controlWindow);return;}
 WNDCLASSW wc{};wc.lpfnWndProc=controlProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"CoopDesktopControls";wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
 controlWindow=CreateWindowW(wc.lpszClassName,(gameTitle+L" - Controls").c_str(),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,810,140+int(bindings.size())*30,host,nullptr,wc.hInstance,nullptr);
 CreateWindowW(L"STATIC",L"Choose a binding. Esc cancels; Delete clears it.",WS_CHILD|WS_VISIBLE,12,10,775,25,controlWindow,(HMENU)1,nullptr,nullptr);
 for(int p=0;p<2;p++){int x=12+p*390;CreateWindowW(L"STATIC",p?L"PLAYER 2      Keyboard         Gamepad":L"PLAYER 1      Keyboard         Gamepad",WS_CHILD|WS_VISIBLE,x,42,380,25,controlWindow,nullptr,nullptr,nullptr);for(unsigned r=0;r<bindings.size();r++){CreateWindowW(L"STATIC",bindings[r].label,WS_CHILD|WS_VISIBLE,x,77+r*30,140,25,controlWindow,nullptr,nullptr,nullptr);CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE,x+145,72+r*30,105,27,controlWindow,(HMENU)(INT_PTR)(100+p*100+r),nullptr,nullptr);CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE,x+257,72+r*30,113,27,controlWindow,(HMENU)(INT_PTR)(120+p*100+r),nullptr,nullptr);}}
 EnumChildWindows(controlWindow,[](HWND child,LPARAM)->BOOL{SendMessage(child,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return TRUE;},0);
 refreshControls();EnableWindow(host,FALSE);SetTimer(controlWindow,1,20,nullptr);
}
static void configure(const int defaults[2][16],const unsigned padDefaults[16],std::initializer_list<Binding>rows){
 bindings=rows;for(int p=0;p<2;p++)for(int b=0;b<16;b++){std::wstring key=std::to_wstring(b);keys[p][b]=number(p?L"P2Keyboard":L"P1Keyboard",key.c_str(),defaults[p][b]);padKeys[p][b]=number(p?L"P2Gamepad":L"P1Gamepad",key.c_str(),padDefaults[b]);}
}
static unsigned buttons(int p){unsigned bits=0;for(int b=0;b<16;b++)if(keys[p][b] && (GetAsyncKeyState(keys[p][b])&0x8000))bits|=1u<<b;XINPUT_STATE s{};if(getPad && getPad(p,&s)==ERROR_SUCCESS){for(int b=0;b<16;b++)if(padKeys[p][b] && (s.Gamepad.wButtons&padKeys[p][b]))bits|=1u<<b;if(s.Gamepad.sThumbLX<-12000)bits|=1<<6;if(s.Gamepad.sThumbLX>12000)bits|=1<<7;if(s.Gamepad.sThumbLY>12000)bits|=1<<4;if(s.Gamepad.sThumbLY<-12000)bits|=1<<5;}return bits;}
static void attach(HWND window,unsigned w,unsigned h,std::function<void()>controls={}){
 host=window;externalControls=controls;geometry(w,h);normalMenu=GetMenu(host);if(!normalMenu)normalMenu=CreateMenu();
 HMENU settings=CreatePopupMenu(),display=CreatePopupMenu();AppendMenuW(settings,MF_STRING,9001,L"Choose ROM...");AppendMenuW(settings,MF_STRING,9002,L"Controls...\tF2");
 for(int i=1;i<=4;i++)AppendMenuW(display,MF_STRING,9010+i,(std::to_wstring(i)+L"x window").c_str());AppendMenuW(display,MF_STRING,9003,L"Fullscreen\tF11 / Alt+Enter");AppendMenuW(settings,MF_POPUP,(UINT_PTR)display,L"Screen size");AppendMenuW(settings,MF_STRING,9004,L"Open saves and settings folder");AppendMenuW(normalMenu,MF_POPUP,(UINT_PTR)settings,L"Settings");SetMenu(host,normalMenu);DrawMenuBar(host);
 attaching=true;resizeClient(std::max(256,number(L"Display",L"Width",w*3)),std::max(224,number(L"Display",L"Height",h*3)));attaching=false;
 if(number(L"Display",L"Fullscreen",0))toggleFull();
}
static bool handle(HWND w,UINT m,WPARAM a,LPARAM b){
 if((m==WM_SYSKEYDOWN&&a==VK_RETURN)||(m==WM_KEYDOWN&&a==VK_F11&&!(b&(1L<<30)))){toggleFull();return true;}
 if(m==WM_EXITSIZEMOVE){saveWindow();return false;}
 if(m==WM_CLOSE || m==WM_DESTROY){saveWindow();return false;}
 if(m!=WM_COMMAND || LOWORD(a)<9000)return false;
 int id=LOWORD(a);
 if(id>=9011&&id<=9014)resizeClient(frameWidth*(id-9010),frameHeight*(id-9010));
 if(id==9002)openControls();if(id==9003)toggleFull();
 if(id==9004)ShellExecuteW(host,L"open",data.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
 if(id==9001){for(;;){std::wstring chosen=pickRom();if(chosen.empty())break;auto bytes=readRom(chosen);if(validate(bytes)){romFile=chosen;writeSetting(L"Game",L"ROM",chosen);break;}MessageBoxW(host,L"Choose the supported original USA ROM for this game.",gameTitle.c_str(),MB_ICONINFORMATION);}}
 return true;
}
}
