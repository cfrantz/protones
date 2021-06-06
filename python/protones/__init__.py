# ProtoNES

# When protones starts, it will replace this with a reference
# to the application instance.
app = None

class EmulatorPeer(object):
    def __init__(self):
        print("EmulatorPeer created");

    def emulate_frame(self, nes):
        nes.emulate_frame()
