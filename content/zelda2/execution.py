from application import gui
import collections

class Execution(object):

    def __init__(self, app):
        self.app = app;
        self.visible = False
        # Eight banks, plus a value to represent idle time.
        self.bank = collections.defaultdict(int)
        self.total = 0
        self.known_idle = {
            0x9D59, 0x9DB0, 0xA73D, 0xA7B1, 0xAB73, 0xB08F, 0xD4B2,
            # Central dispatch loop.  Not really idle, but close enough.
            0xc010, 0xc013, 0xc015, 0xc017, 0xc019,
        }
        self.cpuaddr = collections.defaultdict(int)


    def calculate(self):
        self.bank = collections.defaultdict(int)
        self.cpuaddr.clear()
        self.total = 0
        for addr, cycles in self.app.nes.frame_profile.items():
            b = addr >> 16
            addr &= 0xFFFF
            if addr in self.known_idle:
                b = -1

            self.cpuaddr[addr] += cycles
            self.bank[b] += cycles
            self.total += cycles

    def _enable(self):
        self.bank = collections.defaultdict(int)

    def _disable(self):
        pass

    def enable(self, value):
        self.visible = value
        if value:
            self._enable()
        else:
            self._disable()

    def buckets(self):
        mapper = self.app.nes.cartridge.mapper
        banks = self.app.nes.cartridge.prglen // 16384
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


    def draw(self):
        if not self.visible:
            return
        self.calculate()
        gui.begin('Zelda2 Execution Statistics')
        #gui.plot_histogram('', self.bank, 0, 'Bank', 0, 29000,
        #        graph_size=gui.Vec2(400, 100))
        for i, count in self.buckets().items():
            frac = count / self.total
            gui.progress_bar(frac, gui.Vec2(400, 40), "")
            gui.same_line()
            if i == -1:
                i = "Idle"
            gui.text("%s: %.2f%%" % (i, 100.0*frac))

        pairs = sorted(self.cpuaddr.items(), key=lambda x:x[1], reverse=True)
        for addr, count in pairs[0:20]:
            gui.text("%04x: %d (%.2f%%)" % (addr, count, count/self.total*100))
        #m = max(self.cpuaddr)
        #i = self.cpuaddr.index(m)
        #print("Spent %d clocks at %x (known=%s)" % (m, i, i in self.known_idle))
        #gui.plot_histogram('', self.cpuaddr, 0, 'CPU Addr', 0, m)
                #graph_size=gui.Vec2(400, 100))
        gui.end()
