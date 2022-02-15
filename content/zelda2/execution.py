import bimpy
import protones
import collections

class Execution(object):

    def __init__(self, root):
        self.root = root;
        self.visible = bimpy.Bool()
        # Eight banks, plus a value to represent idle time.
        self.bank = collections.defaultdict(int)
        self.total = 0
        self.known_idle = {
            0x9D59, 0x9DB0, 0xA73D, 0xA7B1, 0xAB73, 0xB08F, 0xD4B2,
            # Central dispatch loop.  Not really idle, but close enough.
            0xc010, 0xc013, 0xc015, 0xc017, 0xc019,
        }
        self.cpuaddr = collections.defaultdict(int)


    def Calculate(self):
        self.bank = collections.defaultdict(int)
        self.cpuaddr.clear()
        self.total = 0
        for addr, cycles in self.root.nes.frame_profile.items():
            b = addr >> 16
            addr &= 0xFFFF
            if addr in self.known_idle:
                b = -1

            self.cpuaddr[addr] += cycles
            self.bank[b] += cycles
            self.total += cycles

    def _Enable(self):
        self.bank = collections.defaultdict(int)

    def _Disable(self):
        pass

    def Enable(self, value):
        self.visible.value = value
        if value:
            self._Enable()
        else:
            self._Disable()

    def Buckets(self):
        mapper = self.root.nes.cartridge.mapper
        banks = self.root.nes.cartridge.prglen // 16384
        buckets = {i:0 for i in range(-1, banks)}
        if mapper == 5 or mapper == 85:
            for i, count in self.bank.items():
                if i < 0:
                    buckets[i] += count
                else:
                    buckets[i//2] += count
        else:
            for i, count in self.bank.items():
                buckets[i] += count
        return buckets


    def Draw(self):
        if not self.visible.value:
            return
        self.Calculate()
        bimpy.begin('Zelda2 Execution Statistics')
        #bimpy.plot_histogram('', self.bank, 0, 'Bank', 0, 29000,
        #        graph_size=bimpy.Vec2(400, 100))
        for i, count in self.Buckets().items():
            frac = count / self.total
            bimpy.progress_bar(frac, bimpy.Vec2(400, 40), "")
            bimpy.same_line()
            if i == -1:
                i = "Idle"
            bimpy.text("%s: %.2f%%" % (i, 100.0*frac))

        pairs = sorted(self.cpuaddr.items(), key=lambda x:x[1], reverse=True)
        for addr, count in pairs[0:20]:
            bimpy.text("%04x: %d (%.2f%%)" % (addr, count, count/self.total*100))
        #m = max(self.cpuaddr)
        #i = self.cpuaddr.index(m)
        #print("Spent %d clocks at %x (known=%s)" % (m, i, i in self.known_idle))
        #bimpy.plot_histogram('', self.cpuaddr, 0, 'CPU Addr', 0, m)
                #graph_size=bimpy.Vec2(400, 100))
        bimpy.end()
