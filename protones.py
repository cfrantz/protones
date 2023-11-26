import argparse
import importlib
import sys
import site
import IPython
import termios
from threading import Thread

import application
from application import gui

flags = argparse.ArgumentParser(prog="protones", description="Description")
flags.add_argument("--interactive", "-i", action="store_true", help="Start an interactive Python shell")
flags.add_argument("--plugin", "-p", type=str, help="Load a plugin")
flags.add_argument("nesfile", nargs='?', help="NES ROM to run")

class AbslFlag(argparse.Action):
    """Absl Flags / argparse fusion."""

    @staticmethod
    def setup(parser):
        """Setup a parser with all known absl flags."""
        for name, flag in application.get_all_flags().items():
            if flag.is_retired:
                continue
            parser.add_argument('--'+name, action=AbslFlag, help=flag.help)

    def __call__(self, parser, namespace, values, option_string=None):
        flag = application.find_command_line_flag(self.dest)
        flag.parse(values)
        setattr(namespace, self.dest, values)

class ProtoNES(application.ProtoNES):
    def __init__(self):
        super().__init__()
        self.plugins = []

    def interactive_shell(self):
        """Start an interactive python shell."""
        app = self
        IPython.embed()
        app.running = False

    def load_plugin(self, name):
        """Load a module as a plugin."""
        plugin = importlib.import_module(name)
        self.plugins.append(plugin.create(self))

    def menu_bar_hook(self):
        for p in self.plugins:
            p.menu_bar()

    def menu_hook(self, name):
        for p in self.plugins:
            p.menu(name)

    def draw(self):
        super().draw()
        for p in self.plugins:
            p.draw()

    def run(self):
        self.running = True
        self.nes.reset()
        while self.running:
            f = self.nes.frame
            self.nes.emulate_frame()
            self.maybe_save_history(f)
            for p in self.plugins:
                p.run_per_frame()
            self.base_draw()
            if not self.process_events():
                self.running = False
        self.nes.shutdown()


def main(args):
    # We really need to run ProtoNES in the main thread and that means the
    # interactive shell will end up in a daemon thread.  That means the app
    # can just exit without IPython having a chaince to restore the terminal
    # paramaters.
    #
    # We'll keep our own copy of terminal state and restore it upon app exit.
    term_state = termios.tcgetattr(sys.stdout)
    try:
        app = ProtoNES()
        app.init()
        if args.plugin:
            app.load_plugin(args.plugin)
        if args.nesfile:
            app.load(args.nesfile)

        if args.interactive:
            thread = Thread(target=app.interactive_shell, daemon=True)
            thread.start()
        app.run()
    finally:
        termios.tcsetattr(sys.stdout, termios.TCSANOW, term_state)
    return 0

if __name__ == '__main__':
    application.set_name(flags.prog)
    site.addsitedir(application.resource_dir())
    AbslFlag.setup(flags)
    args = flags.parse_args()
    sys.exit(main(args))
