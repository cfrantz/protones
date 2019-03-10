######################################################################
# Implement Memory Display
######################################################################
import bimpy
from .. import debug

def _bin(v):
    result = bin(v)[2:]
    return ('0' * (8-len(result))) + result

class Memory(object):
    WHITE = bimpy.Vec4(1.0, 1.0, 1.0, 1.0)
    RED = bimpy.Vec4(1.0, 0.0, 0.0, 1.0)
    GREEN = bimpy.Vec4(0.0, 1.0, 0.0, 1.0)
    MODES = {
        0x05: 'Overworld',
        0x0b: 'Sideview',
    }

    DEMON = ['_', 's', 'L', 'F']
    POS = ['north', 'east', 'south', 'west']


    def __init__(self, root):
        self.root = root
        self.visible = bimpy.Bool()
        self.root.nes.cpu.SetExecCallback(0xdff5, self.LinkStanding)
        self.root.nes.cpu.SetExecCallback(0x82c3, self.OverworldSpawn)
        self.link_tile = 0
        self.spawn_prob = 0
        self.spawn_cancel = 0
        self.spawn_timer = 255
        self.pause_on_spawn = bimpy.Bool()

    def LinkStanding(self, cpu):
        mem = self.root.nes.mem
        self.link_tile = mem[0x02]
        return cpu.pc

    def OverworldSpawn(self, cpu):
        self.root.nes.pause = self.pause_on_spawn.value
        return cpu.pc

    def Draw(self):
        """Draw our Memory dialog box."""
        if not self.visible.value:
            return

        mem = self.root.nes.mem
        frame = mem[0x12]
        mode = mem[0x736]

        bimpy.begin('Zelda2 Memory Values')

        bimpy.checkbox('Pause on overworld spawn', self.pause_on_spawn)
        bimpy.text('Frame = %03d (0x%02x)' % (frame, frame))
        name = self.MODES.get(mode, 'Unknown')
        bimpy.text('Game mode = %02x (%s)' % (mode, name))
        bimpy.separator()

        if mode == 0x05:
            self.Overworld()
        elif mode == 0x0b:
            self.Sideview()
        else:
            bimpy.text('no data')
        bimpy.end()

    def Sideview(self):
        mem = self.root.nes.mem
        labels = ['Link', 'Enemy1', 'Enemy2', 'Enemy3', 'Enemy4', 'Enemy5', 'Enemy6']

        bimpy.text('            Xpos  Ypos  Xvel  Anim  HP  ID  AI  It')
        bimpy.separator()
        for i, label in enumerate(labels):
            if i == 0:
                exists = 1
                hp = mem[0x774]
                eid = 0
                ai = 0
                it = 0
            else:
                exists = mem[0xb5 + i]
                hp = mem[0xc1 + i]
                eid = mem[0xa0 + i]
                ai = mem[0xae + i]
                it = mem[0x48d + i]

            velx = mem.ReadSignedByte(0x70 + i)
            page = mem[0x3b + i]
            posx = mem[0x4d + i]
            subpx = mem[0x3d6 + i]
            posy = mem[0x29 + i]
            anim = mem[0x80 + i]


            bimpy.text_colored(self.WHITE if exists else self.RED,
               '%6s: %2x.%02x.%02x    %02x  %4x  %4x  %02x  %02x  %02x  %02x' % (
                label, page, posx, subpx, posy, velx, anim, hp, eid, ai, it))

        memloc = {
            0x71f: 'Scroll direction',
            0x720: 'Delay Scroll dir',
            0x732: 'Next screen left',
            0x733: 'Next screen right',
            0x734: 'Next tile left',
            0x735: 'Next tile right',
        }
        for addr, desc in sorted(memloc.items()):
            bimpy.text('%04x: %20s = %02x' % (addr, desc, mem[addr]))

    def Overworld(self):
        mem = self.root.nes.mem
        spawn_timer = mem[0x516]

        bimpy.text('Step counter: %d' % mem[0x26])
        bimpy.text('Spawn timer:  %d' % spawn_timer)
        bimpy.text('Link standing on tile type %02x' % self.link_tile)

        bimpy.text('            Xpos  Ypos  Xvel  Yvel  ID')
        bimpy.separator()
        bimpy.text('Link:         %02x    %02x' % (mem[0x74], mem[0x73]))

        for i in range(8):
            bimpy.text('Spawn %d:      %02x    %02x    %02x    %02x  %02x' % (
                i, mem[0x4e+i], mem[0x2a+i], mem[0x575+i], mem[0x56d+i],
                mem[0x82+i]))

        bimpy.separator()
        self.Spawn()

    def Spawn(self):
        mem = self.root.nes.mem

        spawns = ['_', '_', '_', '_']
        cancel = self.spawn_cancel
        prob = self.spawn_prob
        self.spawn_cancel = mem[0x51c] & 3
        self.spawn_prob = mem[0x51b]
        cancel = self.spawn_cancel
        prob = self.spawn_prob

        
        index = 0
        for y in range(6, -1, -1):
            if self.link_tile == debug.ReadPrgBank(0, 0x8231+y):
                index = y
                break

        far = debug.ReadPrgBank(0, 0x8238+index)
        pattern = 0
        if index:
            index = index * 4 + 3
            while prob < debug.ReadPrgBank(0, 0x824d+index):
                index -= 1

            index = (index & 3) * 4 + 3
            for i in range(4):
                if 3-i != cancel:
                    spawns[3-i] = self.DEMON[
                            debug.ReadPrgBank(0, 0x8265+index-i)]

        bimpy.text('Next Spawn:')
        bimpy.text('Probabilty value: %02x' % prob)
        bimpy.text('Cancel value:     %02x (%s)' % (cancel, self.POS[cancel]))
        bimpy.text('Distance:         %s' % ('far' if far else 'near'))
        bimpy.text('')
        bimpy.text('                  %s' % (spawns[0]))
        bimpy.text('Spawn pattern:  %s   %s' % (spawns[1], spawns[3]))
        bimpy.text('                  %s' % (spawns[2]))


