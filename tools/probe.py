"""Headless Genesis probe. Keeps the supplied ROM unchanged."""
import ctypes as C
import hashlib
from pathlib import Path
import sys
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / 'engine/genesis_plus_gx_libretro.dll'
class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]
class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]

class Probe:
    def __init__(self, core_path=CORE):
        self.core = C.CDLL(str(core_path))
        self.buttons = [set(), set()]
        self.options = {}
        self.format = 0
        self.frame = None
        self.callbacks = []
        def env(cmd, data):
            if cmd in (9, 31):
                C.cast(data, C.POINTER(C.c_char_p))[0] = str(ROOT).encode()
                return True
            if cmd == 10:
                self.format = C.cast(data, C.POINTER(C.c_int))[0]
                return True
            if cmd == 16:
                arr = C.cast(data, C.POINTER(Variable)); i = 0
                while arr[i].key:
                    self.options[arr[i].key] = arr[i].value.split(b'; ', 1)[-1].split(b'|')[0]
                    i += 1
                return True
            if cmd == 15:
                var = C.cast(data, C.POINTER(Variable)).contents
                var.value = self.options.get(var.key)
                return bool(var.value)
            return cmd in (6, 11, 35, 37)
        def video(data, w, h, pitch):
            if data:
                self.frame = (C.string_at(data, h*pitch), w, h, pitch)
        signatures = {
            'environment': (C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p), env),
            'video_refresh': (C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t), video),
            'audio_sample_batch': (C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t), lambda data,n:n),
            'audio_sample': (C.CFUNCTYPE(None, C.c_int16, C.c_int16), lambda l,r:None),
            'input_poll': (C.CFUNCTYPE(None), lambda:None),
            'input_state': (C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint), lambda p,d,i,b:int(p<2 and b in self.buttons[p])),
        }
        for name, (sig, fn) in signatures.items():
            cb = sig(fn); self.callbacks.append(cb)
            getattr(self.core, 'retro_set_'+name)(cb)
        self.core.retro_init()
        self.rom_path = ROOT / 'Splatterhouse 3 (USA).md'
        data = self.rom_path.read_bytes()
        assert hashlib.sha256(data).hexdigest() == '8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710', 'Unsupported ROM revision'
        self.rom = C.create_string_buffer(data)
        game = Game(str(self.rom_path).encode(), C.addressof(self.rom), len(data), None)
        self.core.retro_load_game.argtypes = [C.POINTER(Game)]
        self.core.retro_load_game.restype = C.c_bool
        assert self.core.retro_load_game(C.byref(game))
        self.core.retro_set_controller_port_device(0, 1)
        self.core.retro_set_controller_port_device(1, 1)
        self.core.retro_get_memory_data.restype = C.c_void_p
        self.ram_ptr = self.core.retro_get_memory_data(2)
        self.core.retro_serialize_size.restype = C.c_size_t
        for name in ('retro_serialize', 'retro_unserialize'):
            fn = getattr(self.core,name)
            fn.argtypes = [C.c_void_p, C.c_size_t]; fn.restype = C.c_bool

    def run(self, n=1, buttons=()):
        self.buttons[0] = set(buttons)
        for _ in range(n): self.core.retro_run()

    def ram(self):
        raw = C.string_at(self.ram_ptr, 65536)
        # Genesis Plus GX stores 68000 memory with each word byte-swapped.
        data = bytearray(raw); data[0::2] = raw[1::2]; data[1::2] = raw[0::2]
        return bytes(data)

    def write(self, addr, data):
        for i,b in enumerate(data): C.c_ubyte.from_address(self.ram_ptr+((addr+i)^1)).value = b

    def save(self):
        n = self.core.retro_serialize_size(); buf = C.create_string_buffer(n)
        assert self.core.retro_serialize(buf,n)
        return buf.raw

    def restore(self, data):
        buf = C.create_string_buffer(data)
        assert self.core.retro_unserialize(buf,len(data))

    def screenshot(self, path):
        data,w,h,pitch = self.frame
        if self.format == 1:
            im = Image.frombytes('RGB',(w,h),data,'raw','BGRX',pitch)
        else:
            pixels = bytearray()
            for y in range(h):
                for x in range(w):
                    v = int.from_bytes(data[y*pitch+x*2:y*pitch+x*2+2], 'little')
                    if self.format == 2: pixels.extend(((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31))
                    else: pixels.extend((((v>>10)&31)*255//31,((v>>5)&31)*255//31,(v&31)*255//31))
            im = Image.frombytes('RGB',(w,h),bytes(pixels))
        im.save(path)

if __name__ == '__main__':
    out = ROOT/'diagnostics'; out.mkdir(exist_ok=True)
    p = Probe()
    print('ROM SHA256:', hashlib.sha256(p.rom_path.read_bytes()).hexdigest(), flush=True)
    for i in range(12):
        p.run(120)
        p.run(1, (3,))
        p.screenshot(out/f'boot-{i:02}.png')
        (out/f'boot-{i:02}.ram').write_bytes(p.ram())
        (out/f'boot-{i:02}.state').write_bytes(p.save())
    print('Saved boot sequence to',out)
