from table_attrib import *

class GeneratorH:
    def __init__(self):
        pass

    def Generate(self, entities, filepath, prefix = ''):
        self.filehandle = open(filepath, 'wb')
        self.filehandle.write(prefix)
        for entity in entities:
            self.__generateentity(entity, 0, ENTITY_TYPE_ROOT)
        self.filehandle.close()

    def __generateentity(self, entity, level, ptype):
        if 'members' in entity:
            return self.__generateblock(entity, level, ptype)
        if 'name' in entity:
            return self.__generatemember(entity, level, ptype)

        type = entity['type']
        if type == ENTITY_TYPE_SPACE:
            return self.filehandle.write('\n')
        if type == ENTITY_TYPE_HEADER:
            return self.filehandle.write(entity['data'] + '\n')
        if type == ENTITY_TYPE_COMMENT:
            return self.filehandle.write('\t'*level + entity['data'] + '\n')

    def __generateblock(self, entity, level, ptype):
        type = entity['type']
        if type == ENTITY_TYPE_TABLE or type == ENTITY_TYPE_STRUCT:
            self.filehandle.write('\t'*level + 'struct ' + entity['name'] + '\n')
            self.filehandle.write('\t'*level + '{\n')
            self.filehandle.write('\t'*(level+1) + entity['name'] + '();\n')
            self.filehandle.write('\n' if ptype == ENTITY_TYPE_ROOT else '')
        elif type == ENTITY_TYPE_ENUM:
            self.filehandle.write('\t'*level + 'enum ' + entity['name'] + '\n')
            self.filehandle.write('\t'*level + '{\n')

        members = entity['members']
        for member in members:
            self.__generateentity(member, level+1, type)

        self.filehandle.write('\t'*level + '};\n')

    def __generatemember(self, member, level, ptype):
        scomment = (' '*2 + member['comment']) if 'comment' in member else ''
        if ptype == ENTITY_TYPE_TABLE or ptype == ENTITY_TYPE_STRUCT:
            stype = ('std::vector<%s>' if 'repeated' in member else '%s') % (map_data_type(member['type']))
            self.filehandle.write('\t'*level + stype + ' ' + member['name']  + ';' + scomment + '\n')
        elif ptype == ENTITY_TYPE_ENUM:
            svalue = (' = ' + member['value']) if 'value' in member else ''
            self.filehandle.write('\t'*level + member['name'] + svalue + ',' + scomment + '\n')


class GeneratorCPP:
    def __init__(self):
        pass

    def Generate(self, entities, filepath, prefix = ''):
        self.filehandle = open(filepath, 'wb')
        self.filehandle.write(prefix)
        for entity in entities:
            self.__generateentity(entity, '')
        self.filehandle.close()

    def __generateentity(self, entity, prefix):
        if not 'members' in entity:
            return None
        type = entity['type']
        if type != ENTITY_TYPE_TABLE and type != ENTITY_TYPE_STRUCT:
            return None

        name = prefix+entity['name']
        members = entity['members']
        for member in members:
            self.__generateentity(member, name+'::')

        self.filehandle.write('\n')
        self.filehandle.write(name + '::' + entity['name'] + '()\n')

        delimiter = ':'
        for member in members:
            if 'members' in member or not 'name' in member or 'repeated' in member:
                continue
            sname = member['name']
            if 'value' in member:
                self.filehandle.write('%s %s(%s)\n' % (delimiter,sname,member['value']))
            else:
                mtype = get_data_type(member['type'])
                if mtype == -1: continue
                if mtype == DATA_TYPE_STRING: continue
                self.filehandle.write('%s %s(0)\n' % (delimiter,sname))
            delimiter = ','

        self.filehandle.write('{\n}\n')


class GeneratorHelperCPP:
    def __init__(self):
        pass

    def Generate(self, entities, filepath, prefix = ''):
        self.filehandle = open(filepath, 'wb')
        self.filehandle.write(prefix)
        for entity in entities:
            self.__generateentity(entity, '')
        self.filehandle.close()

    def __generateentity(self, entity, prefix):
        if not 'members' in entity:
            return None

        name = prefix+entity['name']
        for member in entity['members']:
            self.__generateentity(member, name+'::')

        type = entity['type']
        if type == ENTITY_TYPE_TABLE:
            self.__generatetableentity(entity, prefix)
        elif type == ENTITY_TYPE_STRUCT:
            self.__generatestructentity(entity, prefix)
        elif type == ENTITY_TYPE_ENUM:
            self.__generateenumentity(entity, prefix)

    def __generatetableentity(self, entity, prefix):
        name = entity['name']
        tablename = entity['(name)'] if '(name)' in entity else name
        self.filehandle.write('\n')
        self.filehandle.write('template<> const char *GetTableName<%s>()\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\treturn "%s";\n' % tablename)
        self.filehandle.write('}\n')

        keyitem, requireds, optionals = None, [], []
        for member in entity['members']:
            if not 'members' in member and 'name' in member:
                if member['rule'] == RULE_TYPE_REQUIRED:
                    if not keyitem and 'key' in member:
                        keyitem = member
                    requireds.append(member)
                elif member['rule'] == RULE_TYPE_OPTIONAL:
                    optionals.append(member)

        if keyitem:
            keysname, keystype = keyitem['name'], map_data_type(keyitem['type'])

            self.filehandle.write('\n')
            self.filehandle.write('template<> const char *GetTableKeyName<%s>()\n' % name)
            self.filehandle.write('{\n')
            self.filehandle.write('\treturn "%s";\n' % keysname)
            self.filehandle.write('}\n')

            self.filehandle.write('\n')
            self.filehandle.write('template<> %s GetTableKeyValue(const %s &entity)\n' % (keystype,name))
            self.filehandle.write('{\n')
            self.filehandle.write('\treturn entity.%s;\n' % keysname)
            self.filehandle.write('}\n')

            self.filehandle.write('\n')
            self.filehandle.write('template<> void SetTableKeyValue(%s &entity, %s key)\n' % (name,keystype))
            self.filehandle.write('{\n')
            self.filehandle.write('\tentity.%s = key;\n' % keysname)
            self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> const char *GetTableFieldNameByIndex<%s>(size_t index)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tswitch (index)\n')
        self.filehandle.write('\t{\n')
        self.__genaratefieldnamebyindex(requireds, optionals)
        self.filehandle.write('\t}\n')
        self.filehandle.write('\treturn "";\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> ssize_t GetTableFieldIndexByName<%s>(const char *name)\n' % name)
        self.filehandle.write('{\n')
        self.__genaratefieldindexbyname(requireds, optionals)
        self.filehandle.write('\treturn -1;\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> size_t GetTableFieldNumber<%s>()\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\treturn %d;\n' % (len(requireds) + int(1 if optionals else 0)))
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> std::string GetTableFieldValue(const %s &entity, size_t index)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tswitch (index)\n')
        self.filehandle.write('\t{\n')
        self.__genarateloadfromfield(requireds, optionals)
        self.filehandle.write('\t}\n')
        self.filehandle.write('\treturn "";\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void SetTableFieldValue(%s &entity, size_t index, const char *value)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tswitch (index)\n')
        self.filehandle.write('\t{\n')
        self.__genaratesavetofield(requireds, optionals)
        self.filehandle.write('\t}\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.__genarateloadfromstream(requireds+optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.__genaratesavetostream(requireds+optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.__genarateloadfromjson(requireds+optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.__genaratesavetojson(requireds+optionals)
        self.filehandle.write('}\n')

        if optionals:
            self.filehandle.write('\n')
            self.filehandle.write('template<> void JsonHelper::TableOptionalsFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
            self.filehandle.write('{\n')
            self.__genarateloadfromjson(optionals)
            self.filehandle.write('}\n')

            self.filehandle.write('\n')
            self.filehandle.write('template<> void JsonHelper::TableOptionalsToJson(const %s &entity, rapidjson::Value &value)\n' % name)
            self.filehandle.write('{\n')
            self.__genaratesavetojson(optionals)
            self.filehandle.write('}\n')

    def __generatestructentity(self, entity, prefix):
        name = prefix+entity['name']

        optionals = []
        for member in entity['members']:
            if not 'members' in member and 'name' in member:
                optionals.append(member)

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.__genarateloadfromstream(optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.__genaratesavetostream(optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.__genarateloadfromjson(optionals)
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.__genaratesavetojson(optionals)
        self.filehandle.write('}\n')

    def __generateenumentity(self, entity, prefix):
        name = prefix+entity['name']

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tStreamHelper::EnumFromStream(entity, stream);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tStreamHelper::EnumToStream(entity, stream);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tEnumFromJson(entity, value);\n')
        self.filehandle.write('}\n')

        self.filehandle.write('\n')
        self.filehandle.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.filehandle.write('{\n')
        self.filehandle.write('\tEnumToJson(entity, value);\n')
        self.filehandle.write('}\n')

    def __genaratefieldnamebyindex(self, requireds, optionals):
        for index,member in enumerate(requireds):
            self.filehandle.write('\t\tcase %s: return "%s";\n' % (index,member['name']))
        if optionals:
            self.filehandle.write('\t\tcase %s: return "optionals";\n' % len(requireds))

    def __genaratefieldindexbyname(self, requireds, optionals):
        for index,member in enumerate(requireds):
            self.filehandle.write('\tif (strcmp("%s", name) == 0) return %s;\n' % (member['name'],index))
        if optionals:
            self.filehandle.write('\tif (strcmp("optionals", name) == 0) return %s;\n' % len(requireds))

    def __genarateloadfromfield(self, requireds, optionals):
        for index,member in enumerate(requireds):
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\t\tcase %d: return JsonHelper::BlockVectorToJsonText(entity.%s);\n' % (index,sname))
                else:
                    self.filehandle.write('\t\tcase %d: return JsonHelper::VectorToJsonText(entity.%s);\n' % (index,sname))
            else:
                if mtype == -1:
                    self.filehandle.write('\t\tcase %d: return JsonHelper::BlockToJsonText(entity.%s);\n' % (index,sname))
                else:
                    self.filehandle.write('\t\tcase %d: return FieldHelper::ToString(entity.%s);\n' % (index,sname))
        if optionals:
            self.filehandle.write('\t\tcase %d: return JsonHelper::TableOptionalsToJsonText(entity);\n' % len(requireds))

    def __genaratesavetofield(self, requireds, optionals):
        for index,member in enumerate(requireds):
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\t\tcase %d: JsonHelper::BlockVectorFromJsonText(entity.%s, value); break;\n' % (index,sname))
                else:
                    self.filehandle.write('\t\tcase %d: JsonHelper::VectorFromJsonText(entity.%s, value); break;\n' % (index,sname))
            else:
                if mtype == -1:
                    self.filehandle.write('\t\tcase %d: JsonHelper::BlockFromJsonText(entity.%s, value); break;\n' % (index,sname))
                else:
                    self.filehandle.write('\t\tcase %d: FieldHelper::FromString(entity.%s, value); break;\n' % (index,sname))
        if optionals:
            self.filehandle.write('\t\tcase %d: JsonHelper::TableOptionalsFromJsonText(entity, value); break;\n' % len(requireds))

    def __genarateloadfromstream(self, members):
        for member in members:
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\tStreamHelper::BlockVectorFromStream(entity.%s, stream);\n' % sname)
                else:
                    self.filehandle.write('\tStreamHelper::VectorFromStream(entity.%s, stream);\n' % sname)
            else:
                if mtype == -1:
                    self.filehandle.write('\tLoadFromStream(entity.%s, stream);\n' % sname)
                else:
                    self.filehandle.write('\tStreamHelper::FromStream(entity.%s, stream);\n' % sname)

    def __genaratesavetostream(self, members):
        for member in members:
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\tStreamHelper::BlockVectorToStream(entity.%s, stream);\n' % sname)
                else:
                    self.filehandle.write('\tStreamHelper::VectorToStream(entity.%s, stream);\n' % sname)
            else:
                if mtype == -1:
                    self.filehandle.write('\tSaveToStream(entity.%s, stream);\n' % sname)
                else:
                    self.filehandle.write('\tStreamHelper::ToStream(entity.%s, stream);\n' % sname)

    def __genarateloadfromjson(self, members):
        for member in members:
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\tBlockVectorFromJson(entity.%s, value, "%s");\n' % (sname,sname))
                else:
                    self.filehandle.write('\tVectorFromJson(entity.%s, value, "%s");\n' % (sname,sname))
            else:
                if mtype == -1:
                    self.filehandle.write('\tBlockFromJson(entity.%s, value, "%s");\n' % (sname,sname))
                else:
                    self.filehandle.write('\tFromJson(entity.%s, value, "%s");\n' % (sname,sname))

    def __genaratesavetojson(self, members):
        self.filehandle.write('\tSetJsonObjectValue(value);\n')
        for member in members:
            sname, mtype = member['name'], get_data_type(member['type'])
            if 'repeated' in member:
                if mtype == -1:
                    self.filehandle.write('\tBlockVectorToJson(entity.%s, value, "%s");\n' % (sname,sname))
                else:
                    self.filehandle.write('\tVectorToJson(entity.%s, value, "%s");\n' % (sname,sname))
            else:
                if mtype == -1:
                    self.filehandle.write('\tBlockToJson(entity.%s, value, "%s");\n' % (sname,sname))
                else:
                    self.filehandle.write('\tToJson(entity.%s, value, "%s");\n' % (sname,sname))
