import os
import graphml2lua

def to_lua_source(indir, outdir):
    def export_file(indir, outdir, filename):
        infile = os.path.abspath(os.path.join(indir, filename))
        outfile = os.path.abspath(os.path.join(outdir, filename + ".lua"))
        print "lua %s ..." % infile
        graphml2lua.dump(graphml2lua.load(infile), outfile)

    if not os.path.exists(indir):
        return False

    if not os.path.exists(outdir):
        os.mkdir(outdir)

    for filename in os.listdir(indir):
        if os.path.isdir(os.path.join(indir, filename)):
            to_lua_source(os.path.join(indir, filename), os.path.join(outdir, filename))
        elif os.path.isfile(os.path.join(indir, filename)):
            export_file(indir, outdir, filename)

    return True
