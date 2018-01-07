import os

def strip_file(filepath):
    if os.path.splitext(filepath)[1] in ('.h', '.cpp', '.lua', '.py'):
        fp = open(filepath, 'r+b')
        lines = fp.readlines()
        fp.truncate(0)
        fp.seek(0)
        for line in lines:
            fp.write(line.rstrip().replace('\t', '\x20' * 4) + '\n')
        fp.close()

def strip_directory(dirpath):
    if os.path.exists(dirpath):
        for entry in os.listdir(dirpath):
            entrypath = os.path.join(dirpath, entry)
            if os.path.isfile(entrypath):
                strip_file(entrypath)
            elif os.path.isdir(entrypath):
                strip_directory(entrypath)
