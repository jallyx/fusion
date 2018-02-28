import sys
from exporter import *

reload(sys)
sys.setdefaultencoding('utf-8')

to_lua_source(r'..\..\..\..\..\mid_data\graphml', r'..\..\..\server\build\Debug\graphml')
