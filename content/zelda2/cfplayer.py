######################################################################
# Implement Memory Display
######################################################################
import bimpy
import re
from pprint import pprint

class CFplayer(object):
    WHITE = bimpy.Vec4(1.0, 1.0, 1.0, 1.0)
    RED = bimpy.Vec4(1.0, 0.0, 0.0, 1.0)
    GREEN = bimpy.Vec4(0.0, 1.0, 0.0, 1.0)

    def __init__(self, root):
        self.root = root
        self.symtab = self.load_mapfile()
        self.visible = bimpy.Bool()

    def load_mapfile(self):
        config = dict(f.split('=', 1) for f in self.root.extra_flags)
        mapfile = config.get('mapfile')
        if mapfile:
            return self.parse_mapfile(mapfile)
        else:
            return {}


    def parse_mapfile(self, filename):
        result = {}
        with open(filename, 'rt') as f:
            state = 0
            for line in f:
                line = re.sub(r'[\r\n]+', '', line)
                line = re.sub(r'\s+$', '', line)
                if line.startswith('Exports list by name:'):
                    state = 1
                    continue
                if line == '':
                    state = 0
                    continue
                if re.fullmatch(r'[-]+', line):
                    continue
                if state == 1:
                    vals = re.split(r'\s+', line)
                    while len(vals) in (3, 6):
                        result[vals[0]] = int(vals[1], 16)
                        vals = vals[3:]
        return result

    def Draw(self):
        """Draw our Memory dialog box."""
        if not self.visible.value:
            return

        mem = self.root.nes.mem
        frame = mem[0x12]

        bimpy.begin('CFplayer Memory Values')
        bimpy.text('Frame = %02x' % frame)
        if self.symtab:
            self.player()
        else:
            bimpy.text('No symtab. Did you forget --extra=mapfile=/path/to/mapfile ?')
        bimpy.end()

    def hexdump(self, values):
        return ' '.join('%02x' % x for x in values)

    def player(self):
        mem = self.root.nes.mem
        np = self.symtab['_cfplayer_now_playing']
        bimpy.text('%04x:_cfplayer_now_playing: %04x (%02x %02x)' % (
            np, mem.ReadWord(np, np+1),
            mem[0xeb], mem[0x7fb]))

        np = self.symtab['_sfx_now_playing']
        for i in range(4):
            bimpy.text('%04x:_sfx_now_playing: %04x (%02x %02x)' % (
                np, mem.ReadWord(np+i*2, np+i*2+1),
                mem[0xec+i], mem[0x7fc+i]))

        bimpy.text('')

        n = self.symtab['channel_seq_pos'] - self.symtab['channel_delay']
        symbols = [
            'channel_delay',
            'channel_seq_pos',
            'channel_meas_pos',
            'channel_note',
            'channel_volume',
            'channel_instrument',
            'channel_env_state',
            'channel_env_vol',
            'channel_env_arp',
            'channel_env_pitch',
            'channel_env_duty',
            'channel_owner',
        ]
        for s in symbols:
            addr = self.symtab[s]
            data = mem.Read(addr, n)
            bimpy.text('%04x:%-20s: %s' % (addr, s, self.hexdump(data)))
