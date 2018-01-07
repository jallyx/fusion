import os
import graphml2lua

def to_lua_source(indir, outdir):
    def export_file(indir, outdir, filename):
        infile = os.path.abspath(os.path.join(indir, filename))
        outfile = os.path.join(outdir, os.path.splitext(filename)[0] + '.lua')
        print "lua %s ..." % infile
        graphml2lua.dump(graphml2lua.load(infile), outfile)

    if not os.path.exists(indir):
        return False

    if not os.path.exists(outdir):
        os.mkdir(outdir)

    for filename in os.listdir(indir):
        export_file(indir, outdir, filename)

    return True
