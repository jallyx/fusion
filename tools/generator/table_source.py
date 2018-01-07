import os
from table_parser import *
from table_generator import *
from table_exporter import *

def to_cplusplus_source(indir, outdir):
    def generate_files(indir, outdir, filename):
        print "cplusplus %s ..." % os.path.abspath(os.path.join(indir, filename))
        helper, utils = '#include "jsontable/table_helper.h"', '#include "jsontable/table_utils.h"'
        entities = Parser().Parse(os.path.join(indir, filename))
        filepart = os.path.splitext(filename)[0]
        outpath = os.path.join(outdir, filepart)
        GeneratorH().Generate(entities, outpath+'.h', '#pragma once\n\n')
        GeneratorCPP().Generate(entities, outpath+'.cpp', '%s\n#include "%s.h"\n' % (helper, filepart))
        GeneratorHelperCPP().Generate(entities, outpath+'_helper.cpp', '%s\n%s\n#include "%s.h"\n' % (helper, utils, filepart))

    if not os.path.exists(indir):
        return False

    if not os.path.exists(outdir):
        os.mkdir(outdir)

    for filename in os.listdir(indir):
        generate_files(indir, outdir, filename)

    filehandle = open(os.path.join(outdir, 'table_list.h'), 'wb')
    filehandle.write('#pragma once\n\n')
    for filename in os.listdir(indir):
        filepart = os.path.splitext(filename)[0]
        filehandle.write('#include "%s.h"\n' % filepart)
    filehandle.close()

    return True

def to_lua_source(indir, outdir):
    def export_file(indir, outdir, filename):
        print "lua %s ..." % os.path.abspath(os.path.join(indir, filename))
        helper, utils = "require('luastruct/struct_helper')", "require('luastruct/struct_utils')"
        entities = Parser().Parse(os.path.join(indir, filename))
        filepart = os.path.splitext(filename)[0]
        outpath = os.path.join(outdir, filepart)
        ExporterLua().Export(entities, outpath+'.lua', '%s\n%s\n\n' % (helper, utils))

    if not os.path.exists(indir):
        return False

    if not os.path.exists(outdir):
        os.mkdir(outdir)

    for filename in os.listdir(indir):
        export_file(indir, outdir, filename)

    filehandle = open(os.path.join(outdir, 'struct_list.lua'), 'wb')
    for filename in os.listdir(indir):
        filepart = os.path.splitext(filename)[0]
        filehandle.write("require('%s')\n" % filepart)
    filehandle.close()

    return True
