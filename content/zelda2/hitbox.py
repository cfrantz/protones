######################################################################
# Hitboxes for Link, Enemies and Projectiles in Zelda 2
######################################################################
import bimpy

instance = None

class EnemyHitbox(object):
    SIZETABLE = 0xe8fa
    HITBOX = 0x60FF0000
    SOLID = 0xFF000000
    WHITE = 0xFFFFFFFF
    TRANSPARENT = bimpy.Vec4(0, 0, 0, 0)
    ENEMY_LIST_SIZE = 40 #36
    ENEMY_LIST_RAM = 0x6d00

    def enemy_list_size(self, enemies=36, total_size=0x2a1):
        offset = 0
        table = {}
        table['projectile_init']    = (offset, 12*2);      offset += 12*2
        table['projectile_table1']  = (offset, 9);         offset += 9
        table['enemy_hp']           = (offset, enemies);   offset += enemies
        table['enemy_init']         = (offset, enemies*2); offset += enemies * 2
        table['enemy_ai']           = (offset, enemies*2); offset += enemies * 2
        table['enemy_xp']           = (offset, enemies);   offset += enemies
        table['enemy_vuln']         = (offset, enemies);   offset += enemies
        table['enemy_size']         = (offset, enemies);   offset += enemies
        table['enemy_misc']         = (offset, enemies);   offset += enemies
        table['enemy_display']      = (offset, enemies*2); offset += enemies * 2
        table['projectile_table2']  = (offset, 9);         offset += 9
        table['projectile_table3']  = (offset, 12);        offset += 12
        table['projectile_display'] = (offset, 9*2);       offset += 9*2
        table['sprite_table']       = (offset, 0)
        self.sizecodes = self.ENEMY_LIST_RAM + table['enemy_size'][0]

    def __init__(self, root, index):
        self.root = root
        self.index = index
        self.name = "enemy%d" % index
        self.hb = bimpy.Vec2(0, 0)
        self.hbsz = bimpy.Vec2(0, 0)
        self.dragging = False
        self.locked = False
        self.root.nes.cpu.SetReadCallback(0x2a + self.index, self.MemCb)
        self.root.nes.cpu.SetReadCallback(0x4e + self.index, self.MemCb)
        self.root.nes.cpu.SetReadCallback(0x3c + self.index, self.MemCb)
        self.enemy_list_size(self.ENEMY_LIST_SIZE)
        global instance
        instance = self

    def MemCb(self, cpu, addr, val):
        """Trap CPU memory reads so we can place enemies arbitrarily."""
        if self.exists and (self.dragging or self.locked):
            if addr == 0x2a + self.index:
                val = int(self.ypos)
            elif addr == 0x4e + self.index:
                val = int(self.xpos) & 0xFF
            elif addr == 0x3c + self.index:
                val = int(self.xpos) >> 8
        return val

    def Update(self):
        """Read enemy or projectile data out of NES memory."""
        mem = self.root.nes.mem
        # Game State 0x0b is sideview mode.  If we aren't in sideview mode,
        # just exit.
        if mem[0x736] != 0x0b:
            self.exists = False
            self.dragging = False
            self.locked = False
            return

        # Screen scroll position.
        scroll = mem.ReadWord(0x72c, 0x72a)
        if self.dragging or self.locked:
            # If we're moving an enemy, write the desired position
            mem[0x2a + self.index] = int(self.ypos)
            mem.WriteWord(0x4e + self.index, 0x3c + self.index, int(self.xpos))
        else:
            # Otherwise read where the game says the enemy is.
            self.ypos = mem[0x2a + self.index]
            self.xpos = mem.ReadWord(0x4e + self.index, 0x3c + self.index)

        self.xscr = self.xpos - scroll
        if self.xscr < 0 or self.xscr > 255:
            self.xscr = -1
        # Get the enemy id or projectile id
        if self.index < 6:
            self.exists = mem[0xb6 + self.index]
            self.enemyid = mem[0xa1 + self.index]
        else:
            self.enemyid = mem[0x87 + self.index - 6]
            self.exists = mem[0x87 + self.index - 6]
            if self.exists > 15:
                self.exists = 0
        self.entype = mem[self.sizecodes + self.enemyid]

        # Compute the hitbox.
        ofs = (self.entype * 4) & 0xFF
        self.hb.x = self.xscr + mem.ReadSignedByte(self.SIZETABLE + ofs + 0)
        self.hb.y = self.ypos + mem.ReadSignedByte(self.SIZETABLE + ofs + 2)
        self.hbsz.x = mem[self.SIZETABLE + ofs + 1]
        self.hbsz.y = mem[self.SIZETABLE + ofs + 3]

    def DrawImage(self):
        if self.exists and self.xscr != -1:
            scale = bimpy.Vec2(self.root.scale * self.root.aspect,
                               self.root.scale)
            bimpy.add_rect_filled(self.hb*scale, (self.hb+self.hbsz)*scale, self.HITBOX)
            bimpy.add_rect(self.hb*scale, (self.hb+self.hbsz)*scale,
                           self.SOLID | self.HITBOX)
            bimpy.add_text(self.hb*scale, self.WHITE, str(self.index+1))

            bimpy.set_cursor_screen_pos(self.hb*scale)
            bimpy.invisible_button(self.name, self.hbsz*scale)
            if bimpy.is_item_clicked(1):
                self.locked = not self.locked
            if bimpy.is_item_active():
                self.dragging = True
                if bimpy.is_mouse_dragging():
                    delta = bimpy.getio().mouse_delta / scale
                    self.xpos += delta.x
                    self.ypos += delta.y
            else:
                self.dragging = False


class LinkHitbox(object):
    SIZETABLE = 0xe8fa
    HITBOX = 0x60FF0000
    SHIELD = 0x6000FF00
    SWORD = 0x600000FF
    SOLID = 0xFF000000

    def __init__(self, root):
        self.root = root
        self.name = "link"
        self.hb = bimpy.Vec2(0, 0)
        self.hbsz = bimpy.Vec2(0, 0)
        self.sh = bimpy.Vec2(0, 0)
        self.shsz = bimpy.Vec2(0, 0)
        self.sw = bimpy.Vec2(0, 0)
        self.swsz = bimpy.Vec2(0, 0)
        self.dragging = False
        self.locked = False

    def Update(self):
        mem = self.root.nes.mem
        if mem[0x736] != 0x0b:
            self.exists = False
            self.dragging = False
            self.locked = False
            return

        self.exists = True
        scroll = mem.ReadWord(0x72c, 0x72a)
        if self.dragging or self.locked:
            mem[0x29] = int(self.ypos)
            mem.WriteWord(0x4d, 0x3b, int(self.xpos))
        else:
            self.ypos = mem[0x29]
            self.xpos = mem.ReadWord(0x4d, 0x3b)

        self.xscr = self.xpos - scroll
        if self.xscr < 0 or self.xscr > 255:
            self.xscr = -1
        self.controller = mem[0x80]
        self.standing = mem[0x17]
        self.facing = mem[0x9f] - 1
        self.swordx = mem[0x47e]
        self.swordy = mem[0x480]

        # Link's hitbox
        self.hb.x = self.xscr + 9
        self.hb.y = self.ypos + mem[0xe971 + self.standing]
        self.hbsz.x = 13
        self.hbsz.y = mem[0xe973 + self.standing]
                                                                                 
        # Compute shield defense box
        self.sh.x = self.xscr + ((8+14) if self.facing == 0 else (8-1))
        self.sh.y = self.ypos + (2 if self.standing else 17)
        self.shsz.x = 5
        self.shsz.y = 12

        # Compute sword attack box
        self.sw.x = self.swordx + (-8 if self.swordx > self.xscr else 2)
        self.sw.y = self.swordy + (0 if self.controller == 9 else 7)
        self.swsz.x = 14
        self.swsz.y = 3

    def DrawImage(self):
        if not self.exists:
            return

        scale = bimpy.Vec2(self.root.scale * self.root.aspect,
                           self.root.scale)
        bimpy.add_rect_filled(self.hb*scale, (self.hb+self.hbsz)*scale, self.HITBOX)
        bimpy.add_rect(self.hb*scale, (self.hb+self.hbsz)*scale, self.SOLID | self.HITBOX)
        bimpy.add_rect_filled(self.sh*scale, (self.sh+self.shsz)*scale, self.SHIELD)
        bimpy.add_rect(self.sh*scale, (self.sh+self.shsz)*scale, self.SOLID | self.SHIELD)
        if self.swordy != 0xF8:
            bimpy.add_rect_filled(self.sw*scale, (self.sw+self.swsz)*scale, self.SWORD)
            bimpy.add_rect(self.sw*scale, (self.sw+self.swsz)*scale, self.SOLID | self.SWORD)

        bimpy.set_cursor_screen_pos(self.hb*scale)
        if bimpy.invisible_button(self.name, self.hbsz*scale):
            # We don't want to allow locking link
            #self.locked = not self.locked
            pass
        if bimpy.is_item_active():
            self.dragging = True
            if bimpy.is_mouse_dragging():
                delta = bimpy.getio().mouse_delta / scale
                self.xpos += delta.x
                self.ypos += delta.y
        else:
            self.dragging = False

