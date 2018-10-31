######################################################################
# Implement cheats for Zelda 2
######################################################################
import bimpy

class Cheats(object):
    LEVELS = [('AtkLvl', 0), ('MagLvl', 1), ('LifeLvl', 2)]
    HEALTH = [('Hearts', 13), ('Magic', 12), ('Crystals', 29)]
    SPELLS = [
        ('Shield', 4),
        ('Jump', 5),
        ('Life', 6),
        ('Fairy', 7),
        ('Fire', 8),
        ('Reflect', 9),
        ('Spell', 10),
        ('Thunder', 11),
    ]
    ITEMS = [
        ('Candle', 14),
        ('Glove', 15),
        ('Raft', 16),
        ('Boots', 17),
        ('Flute', 18),
        ('Cross', 19),
        ('Hammer', 20),
        ('Key', 21),
    ]

    def __init__(self, root):
        self.root = root
        self.visible = bimpy.Bool()
        for x, _ in self.LEVELS + self.HEALTH:
            setattr(self, x, bimpy.Int())
        for x, _ in self.SPELLS + self.ITEMS:
            setattr(self, x, bimpy.Bool())
        self.downstab = bimpy.Bool()
        self.upstab = bimpy.Bool()
        self.invincible = bimpy.Bool()
        self.walk_anywhere = bimpy.Bool()
        # Trigger on memory reads to 0x773 and 0x774, which hold the amount
        # of magic and health Link has.
        self.root.nes.cpu.SetReadCallback(0x773, self.Invincible)
        self.root.nes.cpu.SetReadCallback(0x774, self.Invincible)

    def Invincible(self, cpu, addr, val):
        """Implement invicibility by trapping CPU reads to health values."""
        # On each read of 773/774, val will hold the value read from
        # memory.  We can return a different value if we want.
        if self.invincible.value:
            if addr == 0x773:
                val = (32 * self.Magic.value) - 1
            if addr == 0x774:
                val = (32 * self.Hearts.value) - 1
        return val

    def Unpack(self):
        """Read health, spells and inventory out of NES memory."""
        mem = self.root.nes.mem
        addr = 0x777
        for x, offset in self.LEVELS + self.HEALTH:
            item = getattr(self, x)
            item.value = mem[addr + offset]
        for x, offset in self.SPELLS + self.ITEMS:
            item = getattr(self, x)
            item.value = mem[addr + offset] != 0

        self.downstab.value = (mem[addr+31] & 0x10) != 0
        self.upstab.value = (mem[addr+31] & 0x04) != 0
        self.walk_anywhere.value = self.root.nes.cartridge.ReadPrg(0x71e) == 0

    def Pack(self):
        """Write health, spells and inventory into NES memory."""
        mem = self.root.nes.mem
        addr = 0x777
        for x, offset in self.LEVELS + self.HEALTH:
            item = getattr(self, x)
            mem[addr + offset] = item.value
        for x, offset in self.SPELLS + self.ITEMS:
            item = getattr(self, x)
            mem[addr + offset] = item.value

        tech = mem[addr + 31] & ~0x14;
        if self.downstab.value:
            tech |= 0x10
        if self.upstab.value:
            tech |= 0x04
        mem[addr + 31] = tech
        self.root.nes.cartridge.WritePrg(0x71e,
                0 if self.walk_anywhere.value else 2)

    def Draw(self):
        """Draw our Cheats dialog box."""
        if not self.visible.value:
            return

        self.Unpack()
        bimpy.begin('Zelda2 Cheats')
        bimpy.push_item_width(70);
        for i, (x, _) in enumerate(self.LEVELS):
            if i:
                bimpy.same_line()
            bimpy.input_int(x, getattr(self, x), 1, 1)

        for i, (x, _) in enumerate(self.HEALTH):
            if i:
                bimpy.same_line()
            bimpy.input_int(x, getattr(self, x), 1, 1)
        bimpy.pop_item_width()

        bimpy.separator()
        bimpy.columns(2, 'items', True)
        for x, _ in self.SPELLS:
            bimpy.checkbox(x, getattr(self, x))

        bimpy.next_column()
        for x, _ in self.ITEMS:
            bimpy.checkbox(x, getattr(self, x))

        bimpy.columns(1)
        bimpy.separator()
        bimpy.checkbox('Downstab', self.downstab)
        bimpy.same_line()
        bimpy.checkbox('Upstab', self.upstab)
        bimpy.same_line()
        if bimpy.button('Everything'):
            for x, _ in self.LEVELS + [('Hearts', 0), ('Magic', 0)]:
                getattr(self, x).value = 8
            for x, _ in self.SPELLS + self.ITEMS:
                getattr(self, x).value = True
            self.upstab.value = True
            self.downstab.value = True

        bimpy.checkbox('Invincible', self.invincible)
        bimpy.checkbox('Walk anywhere on overworld', self.walk_anywhere)

        bimpy.end()
        self.Pack()
