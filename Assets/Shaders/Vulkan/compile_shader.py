from os import walk
from os import *

files = []
for (dir, dirs, files) in walk():
    files = [os.path.join(dir, f) for f in files if not f.endswith('.txt')]
