import types
from table_attrib import *

class Parser:
    def __init__(self):
        pass

    def Parse(self, filepath):
        filehandle = open(filepath, 'rb')
        self.linedatas = filehandle.readlines()
        filehandle.close()

        entities = []
        self.linenumber = 0
        while self.linenumber < len(self.linedatas):
            entities.append(self.__parseline(PARSE_STATE_NULL, ENTITY_TYPE_ROOT))
        return entities

    def __parseline(self, state, ptype):
        stripdata = self.linedatas[self.linenumber].strip()
        self.linenumber += 1

        if len(stripdata) == 0:
            return {'type':ENTITY_TYPE_SPACE}
        if stripdata.startswith('#include'):
            return {'type':ENTITY_TYPE_HEADER, 'data':stripdata}
        if stripdata.startswith('//'):
            return {'type':ENTITY_TYPE_COMMENT, 'data':stripdata}

        if stripdata.startswith('{'):
            if state != PARSE_STATE_BLOCK:
                raise DataError, "not block state at line %s" % self.linenumber
            return True
        if stripdata.startswith('}'):
            if state != PARSE_STATE_MEMBER:
                raise DataError, "not member state at line %s" % self.linenumber
            return False

        label = stripdata.split(None, 1)[0]

        if label == 'table':
            if state != PARSE_STATE_NULL:
                raise DataError, "not null state at line %s" % self.linenumber
            return self.__buildblock(ENTITY_TYPE_TABLE, stripdata)
        if label == 'struct':
            return self.__buildblock(ENTITY_TYPE_STRUCT, stripdata)
        if label == 'enum':
            return self.__buildblock(ENTITY_TYPE_ENUM, stripdata)

        if ptype == ENTITY_TYPE_TABLE:
            return self.__buildtablemember(stripdata)
        if ptype == ENTITY_TYPE_STRUCT:
            return self.__buildstructmember(stripdata)
        if ptype == ENTITY_TYPE_ENUM:
            return self.__buildenummember(stripdata)

    def __buildblock(self, btype, stripdata):
        fields = stripdata.split()
        if (len(fields) != 2 and len(fields) != 3) or (len(fields) == 3 and fields[-1] != '{'):
            raise DataError, "not support syntax at line %s" % self.linenumber
        bstate = PARSE_STATE_BLOCK if len(fields) == 2 else PARSE_STATE_MEMBER
        return {'type':btype, 'name':fields[1], 'members':self.__buildmembers(bstate, btype)}

    def __buildmembers(self, state, btype):
        members = []
        while self.linenumber < len(self.linedatas):
            entity = self.__parseline(state, btype)
            if type(entity) == types.DictType:
                if state == PARSE_STATE_MEMBER:
                    members.append(entity)
            elif type(entity) == types.BooleanType:
                if entity:
                    state = PARSE_STATE_MEMBER
                else:
                    break
        return members

    def __buildtablemember(self, stripdata):
        throw = lambda object: object.__throwfielderror("table")
        fields = stripdata.split(None, 3)
        if len(fields) < 3: throw(self)
        ruletype = get_rule_type(fields[0])
        if ruletype == -1: throw(self)
        member, sname = {'rule':ruletype, 'type':fields[1]}, fields[2].rstrip(',;')
        if sname.endswith('[]'):
            member.update({'repeated':True, 'name':sname[:-2]})
        elif sname.endswith('(key)'):
            member.update({'key':True, 'name':sname[:-5]})
        else:
            member.update({'name':sname})
        if len(fields) == 4:
            member.update(self.__drawmembertail(fields[-1]))
        return member

    def __buildstructmember(self, stripdata):
        throw = lambda object: object.__throwfielderror("struct")
        fields = stripdata.split(None, 2)
        if len(fields) < 2: throw(self)
        member, sname = {'type':fields[0]}, fields[1].rstrip(',;')
        if sname.endswith('[]'):
            member.update({'repeated':True, 'name':sname[:-2]})
        else:
            member.update({'name':sname})
        if len(fields) == 3:
            member.update(self.__drawmembertail(fields[-1]))
        return member

    def __buildenummember(self, stripdata):
        fields = stripdata.split(None, 1)
        member = {'name':fields[0].rstrip(',;')}
        if len(fields) == 2:
            member.update(self.__drawmembertail(fields[-1]))
        return member

    def __drawmembertail(self, attrdata):
        attrs = {}
        if attrdata.startswith('='):
            fields = attrdata.split(None, 2)
            attrs['value'] = fields[1].rstrip(',;')
            attrdata = fields[-1] if len(fields) == 3 else ""
        if attrdata.startswith('//'):
            attrs['comment'] = attrdata
        return attrs

    def __throwmembererror(self, mtype):
        raise DataError, "%s member error at line %s" % (mtype, self.linenumber)
