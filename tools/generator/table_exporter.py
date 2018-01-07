from table_attrib import *

class ExporterLua:
    @classmethod
    def Export(cls, entities, filepath, prefix = ''):
        filehandle = open(filepath, 'wb')
        filehandle.write(prefix)
        cls.Proto(filehandle).Export(entities)
        filehandle.write('\n'+'-'*80+'\n')
        cls.Method(filehandle).Export(entities)
        filehandle.close()

    class Proto:
        def __init__(self, filehandle):
            self.filehandle = filehandle
            self.stacktype, self.scopetype = [], [{}]

        def Export(self, entities):
            for entity in entities:
                self.__exportentity(entity, 0, ENTITY_TYPE_ROOT)

        def __exportentity(self, entity, level, ptype):
            if 'members' in entity:
                return self.__exportblock(entity, level, ptype)
            if 'name' in entity:
                return self.__exportmember(entity, level, ptype)

            etype = entity['type']
            if etype == ENTITY_TYPE_SPACE:
                return self.filehandle.write('\n')
            if etype == ENTITY_TYPE_HEADER:
                return self.filehandle.write(self.__require(entity['data']) + '\n')
            if etype == ENTITY_TYPE_COMMENT:
                return self.filehandle.write('\t'*level + self.__comment(entity['data']) + '\n')

        def __exportblock(self, entity, level, ptype):
            self.__pushtypelayer(entity['name'])
            self.filehandle.write('\t'*level + entity['name'] + ' = {\n')

            etype = entity['type']
            if etype == ENTITY_TYPE_ENUM:
                self.__resetenumstate()
            for member in entity['members']:
                self.__exportentity(member, level+1, etype)

            delimiter = ',' if ptype != ENTITY_TYPE_ROOT else ''
            self.filehandle.write('\t'*level + '}' + delimiter + '\n')
            self.__poptypelayer()

        def __exportmember(self, member, level, ptype):
            scomment = (' '*2 + self.__comment(member['comment'])) if 'comment' in member else ''
            if ptype == ENTITY_TYPE_TABLE or ptype == ENTITY_TYPE_STRUCT:
                sdefine = '%s = %s' % (member['name'], self.__getmembervalue(member))
            elif ptype == ENTITY_TYPE_ENUM:
                sdefine = '%s = %s' % (member['name'], self.__newenumvalue(member))
            if 'sdefine' in locals():
                self.filehandle.write('\t'*level + sdefine + ',' + scomment + '\n')

        def __resetenumstate(self):
            self.enumRefer, self.enumValue = '', -1

        def __newenumvalue(self, member):
            if not 'value' in member:
                self.enumValue += 1
            else:
                svalue = member['value']
                if svalue.isdigit():
                    self.enumRefer, self.enumValue = '', int(svalue)
                else:
                    evalue = self.__getenumvalue(svalue)
                    if evalue is not None:
                        self.enumRefer, self.enumValue = evalue.refer, evalue.value
                    else:
                        self.enumRefer, self.enumValue = self.__rewritescope(svalue), 0
            self.__insertenumvalue(member['name'], self.enumRefer, self.enumValue)
            return self.__packenumvalue(self.enumRefer, self.enumValue)

        def __pushtypelayer(self, stype):
            self.stacktype.append(stype)
            self.scopetype.append({})

        def __poptypelayer(self):
            self.stacktype.pop()
            self.scopetype.pop()

        def __insertenumvalue(self, stype, refer, value):
            class EnumValue:
                def __init__(self, refer, value):
                    self.refer, self.value = refer, value
            self.stacktype.append(stype)
            for depth in xrange(len(self.stacktype)):
                self.scopetype[depth]['::'.join(self.stacktype[depth:])] = EnumValue(refer, value)
            self.stacktype.pop()

        def __getenumvalue(self, svalue):
            for layer in reversed(self.scopetype):
                if svalue in layer: return layer[svalue]
            return None

        def __getmembervalue(self, member):
            def rewritemembervalue(self, member):
                mtype, svalue = get_data_type(member['type']), member['value']
                evalue = self.__getenumvalue(svalue)
                if evalue is not None:
                    return self.__packenumvalue(evalue.refer, evalue.value)
                if mtype == DATA_TYPE_STRING and svalue[0] == '"':
                    return svalue
                return self.__rewritescope(svalue)
            if 'repeated' in member:
                return '{}'
            if 'value' in member:
                return rewritemembervalue(self, member)
            mtype = get_data_type(member['type'])
            if mtype == -1: return 'nil'
            elif mtype == DATA_TYPE_STRING: return "''"
            elif mtype == DATA_TYPE_CHAR: return "'\0'"
            elif mtype == DATA_TYPE_BOOL: return 'false'
            else: return '0'

        @staticmethod
        def __packenumvalue(refer, value):
            if not refer: return str(value)
            if not value: return refer
            return '%s+%d' % (refer, value)

        @staticmethod
        def __rewritescope(svalue):
            return svalue.replace('::', '.')

        @staticmethod
        def __comment(comment):
            return comment.replace('//', '--', 1)

        @staticmethod
        def __require(header):
            start = strpbrk(header, ['"', '<']) + 1
            end = strpbrk(header, ['"', '>', '.h"', '.h>'], start)
            return "require('" + header[start:end] + "')"

    class Method:
        def __init__(self, filehandle):
            self.filehandle = filehandle
            self.stacktype, self.scopetype = [], [{}]

        def Export(self, entities):
            for entity in entities:
                self.__exportentity(entity)

        def __exportentity(self, entity):
            if not 'members' in entity:
                return None

            self.__pushtypelayer(entity['name'])
            for member in entity['members']:
                self.__exportentity(member)

            etype = entity['type']
            if etype == ENTITY_TYPE_TABLE or etype == ENTITY_TYPE_STRUCT:
                self.__exportstructentity(entity)
            elif etype == ENTITY_TYPE_ENUM:
                self.__exportenumentity(entity)

            self.__poptypelayer()

        def __exportstructentity(self, entity):
            name = self.__getstacktype()

            optionals = []
            for member in entity['members']:
                if not 'members' in member and 'name' in member:
                    optionals.append(member)

            self.filehandle.write('\n')
            self.filehandle.write('%s.LoadFromStringBuffer = function(stringBuffer)\n' % name)
            self.filehandle.write('\tlocal entity = {}\n')
            self.__exportloadfromstringbuffer(optionals)
            self.filehandle.write('\treturn entity\n')
            self.filehandle.write('end\n')

            self.filehandle.write('\n')
            self.filehandle.write('%s.SaveToStringBuilder = function(stringBuilder, entity)\n' % name)
            self.__exportsavetostringbuilder(optionals)
            self.filehandle.write('end\n')

        def __exportenumentity(self, entity):
            name = self.__getstacktype()

            self.filehandle.write('\n')
            self.filehandle.write('%s.LoadFromStringBuffer = function(stringBuffer)\n' % name)
            self.filehandle.write('\treturn stringBuffer:ReadInt32()\n')
            self.filehandle.write('end\n')

            self.filehandle.write('\n')
            self.filehandle.write('%s.SaveToStringBuilder = function(stringBuilder, entity)\n' % name)
            self.filehandle.write('\tstringBuilder:WriteInt32(entity)\n')
            self.filehandle.write('end\n')

        def __exportloadfromstringbuffer(self, members):
            for member in members:
                sname, stype = member['name'], member['type']
                mtype = get_data_type(stype)
                if mtype == -1:
                    func = '%s.LoadFromStringBuffer' % self.__getscopetype(stype)
                else:
                    func = 'StringBuffer.Read%s' % self.__getfuncsuffix(mtype)
                if 'repeated' in member:
                    self.filehandle.write('\tentity.%s = stringBuffer:ReadArray(%s);\n' % (sname, func))
                else:
                    if mtype == -1:
                        self.filehandle.write('\tentity.%s = %s(stringBuffer);\n' % (sname, func))
                    else:
                        callfunc = func.replace('StringBuffer.', 'stringBuffer:')
                        self.filehandle.write('\tentity.%s = %s();\n' % (sname, callfunc))

        def __exportsavetostringbuilder(self, members):
            for member in members:
                sname, stype = member['name'], member['type']
                mtype = get_data_type(stype)
                if mtype == -1:
                    func = '%s.SaveToStringBuilder' % self.__getscopetype(stype)
                else:
                    func = 'StringBuilder.Write%s' % self.__getfuncsuffix(mtype)
                if 'repeated' in member:
                    self.filehandle.write('\tstringBuilder:WriteArray(entity.%s, %s);\n' % (sname, func))
                else:
                    if mtype == -1:
                        self.filehandle.write('\t%s(stringBuilder, entity.%s);\n' % (func, sname))
                    else:
                        callfunc = func.replace('StringBuilder.', 'stringBuilder:')
                        self.filehandle.write('\t%s(entity.%s);\n' % (callfunc, sname))

        def __pushtypelayer(self, stype):
            self.stacktype.append(stype)
            self.scopetype.append({})
            for depth in xrange(len(self.stacktype)):
                self.scopetype[depth]['::'.join(self.stacktype[depth:])] = '.'.join(self.stacktype)

        def __poptypelayer(self):
            self.stacktype.pop()
            self.scopetype.pop()

        def __getstacktype(self):
            return '.'.join(self.stacktype)

        def __getscopetype(self, stype):
            for layer in reversed(self.scopetype):
                if stype in layer: return layer[stype]
            return stype

        @staticmethod
        def __getfuncsuffix(mtype):
            if mtype == DATA_TYPE_CHAR: return "Char"
            if mtype == DATA_TYPE_INT8: return "Int8"
            if mtype == DATA_TYPE_UINT8: return "UInt8"
            if mtype == DATA_TYPE_INT16: return "Int16"
            if mtype == DATA_TYPE_UINT16: return "UInt16"
            if mtype == DATA_TYPE_INT32: return "Int32"
            if mtype == DATA_TYPE_UINT32: return "UInt32"
            if mtype == DATA_TYPE_INT64: return "Int64"
            if mtype == DATA_TYPE_UINT64: return "UInt64"
            if mtype == DATA_TYPE_BOOL: return "Bool"
            if mtype == DATA_TYPE_FLOAT: return "Float"
            if mtype == DATA_TYPE_DOUBLE: return "Double"
            if mtype == DATA_TYPE_STRING: return "String"

def strpbrk(haystack, strlist, start=None, end=None):
    poslist = [haystack.find(s, start, end) for s in strlist]
    poslist = [pos for pos in poslist if pos != -1]
    return min(poslist) if poslist else -1
