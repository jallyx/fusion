import io
import os

import table_ast
import table_lexer
import table_parser

def to_cxx_files(indir, outdir):
    def is_expired(infile, outfile):
        return not os.path.exists(outfile) or \
            os.stat(outfile).st_mtime < os.stat(infile).st_mtime

    def parse_ast(infile):
        lexer = table_lexer.TableLexer()
        lexer.parse(infile)
        parser = table_parser.TableParser()
        return parser.parse(lexer)

    def parse_blanks(infile):
        blanks = []
        with open(infile, 'r') as fo:
            for i, line in enumerate(fo.readlines()):
                if line.isspace():
                    blanks.append(i + 1)
        return blanks

    def generate(indir, outdir, filename):
        filepart = os.path.splitext(filename)[0]
        outpath = os.path.abspath(os.path.join(outdir, filepart))
        infile = os.path.abspath(os.path.join(indir, filename))
        if is_expired(infile, outpath+'.h'):
            print('cxx files %s ...' % infile)
            root, blanks = parse_ast(infile), parse_blanks(infile)
            prefix = '#pragma once\n\n'
            GeneratorH(blanks).Generate(root, outpath+'.h', prefix)
            prefix = '#include "jsontable/table_helper.h"\n' + \
                     '#include "%s.h"\n' % filepart
            GeneratorCPP().Generate(root, outpath+'.cpp', prefix)

    if os.path.exists(indir):
        if not os.path.exists(outdir):
            os.mkdir(outdir)
        for filename in os.listdir(indir):
            generate(indir, outdir, filename)
        with open(os.path.join(outdir, 'table_list.h'), 'w') as fo:
            fo.write('#pragma once\n\n')
            for filename in os.listdir(indir):
                filepart = os.path.splitext(filename)[0]
                fo.write('#include "%s.h"\n' % filepart)

def to_cxx_type(node):
    if node.container is None:
        if isinstance(node.parts[0], table_ast.NsIdList):
            return to_cxx_ns(node.parts[0])
        if node.parts[0].type == 'STRING':
            return 'std::string'
        return node.parts[0].value
    if node.container == 'Sequence':
        return 'std::vector<' + \
            to_cxx_type(node.parts[0]) + '>'
    if node.container == 'Associative':
        return 'std::unordered_map<' + \
            to_cxx_type(node.parts[0]) + ', ' + \
            to_cxx_type(node.parts[1]) + '>'

def to_cxx_ns(node):
    return '::'.join([ns.value for ns in node.idList])

class GeneratorH:
    def __init__(self, blanks):
        self.blanks, self.i = blanks, 0
        self.lineno = 0

    def Generate(self, root, outfile, prefix = ''):
        with open(outfile, 'w') as self.fo:
            self.fo.write(prefix)
            for entity in root.externalDeclarations:
                self.__generateentity(entity, 0)

    def __generateentity(self, entity, level):
        if isinstance(entity, table_ast.TableDefinition):
            return self.__generatetableblock(entity, level)
        if isinstance(entity, table_ast.StructDefinition):
            return self.__generatestructblock(entity, level)
        if isinstance(entity, table_ast.EnumDefinition):
            return self.__generateenumblock(entity, level)

        if isinstance(entity, table_ast.TableMemberVarDeclaration) or \
           isinstance(entity, table_ast.StructMemberVarDeclaration):
            return self.__generatestructmember(entity, level)
        if isinstance(entity, table_ast.EnumDeclaration):
            return self.__generateenummember(entity, level)

        if isinstance(entity, table_ast.Preprocessor):
            return self.__writeline2file(level,
                entity.text.value.rstrip(), entity.text.lineno)

        if isinstance(entity, table_ast.Comment):
            return self.__writeline2file(level,
                entity.text.value.rstrip(), entity.text.lineno,
                canInline = True, strSpacer = '  ')

    def __generatetableblock(self, entity, level):
        self.__writeline2file(level, 'struct ' + entity.name.value,
            entity.name.lineno)
        self.__generateblockmember(entity, level)

    def __generatestructblock(self, entity, level):
        inheritBase = entity.inheritBase
        self.__writeline2file(level, 'struct ' + entity.name.value +
            (' : ' + to_cxx_ns(inheritBase) if inheritBase else ''),
            entity.name.lineno)
        self.__generateblockmember(entity, level)

    def __generateenumblock(self, entity, level):
        self.__writeline2file(level, 'enum ' + entity.name.value,
            entity.name.lineno)
        self.__generateblockmember(entity, level)

    def __generateblockmember(self, entity, level):
        self.__writeline2file(level, '{')
        for member in entity.declarationList.declarationList:
            self.__generateentity(member, level + 1)
        self.__writeline2file(level, '}')

    def __generatestructmember(self, member, level):
        self.__writeline2file(level, to_cxx_type(member.memType) + ' ' +
            self.__MemberVarVarDeclaratorList2Str(member.memVarList) + ';',
            member.memVarList.varDeclaratorList[0].lineno)

    def __generateenummember(self, member, level):
        self.__writeline2file(level, member.memName.value +
            self.__enumValue2formatStr(member.memValue) + ',',
            member.memName.lineno)

    def __writeline2file(self, level, linedata, lineno = -1, **kwargs):
        while len(self.blanks) > self.i and self.blanks[self.i] < lineno:
            self.fo.write('\n')
            self.i += 1
        if kwargs and kwargs['canInline'] and self.lineno == lineno:
            self.fo.seek(self.fo.tell() - len(os.linesep))
            self.fo.write(kwargs['strSpacer'])
        elif level != 0:
            self.fo.write('\t' * level)
        self.fo.write(linedata)
        self.fo.write('\n')
        if lineno != -1:
            self.lineno = lineno

    @staticmethod
    def __MemberVarVarDeclaratorList2Str(node):
        return ', '.join([var.value for var in node.varDeclaratorList])

    @classmethod
    def __enumValue2formatStr(cls, node):
        return (' = ' + (to_cxx_ns(node) if isinstance(node, table_ast.NsIdList)
            else node)) if node else ''

class GeneratorCPP:
    def __init__(self):
        pass

    def Generate(self, root, outfile, prefix = ''):
        with open(outfile, 'w') as self.fo:
            self.fo.write(prefix)
            for entity in root.externalDeclarations:
                self.__generateentity(entity, '')

    def __generateentity(self, entity, prefix):
        if not isinstance(entity, table_ast.TableDefinition) and \
           not isinstance(entity, table_ast.StructDefinition) and \
           not isinstance(entity, table_ast.EnumDefinition):
            return

        if isinstance(entity, table_ast.TableDefinition) or \
           isinstance(entity, table_ast.StructDefinition):
            name = prefix + entity.name.value
            for member in entity.declarationList.declarationList:
                 self.__generateentity(member, name + '::')

        if isinstance(entity, table_ast.TableDefinition):
            self.__generatetableentity(entity)
        elif isinstance(entity, table_ast.StructDefinition):
            self.__generatestructentity(entity, prefix)
        elif isinstance(entity, table_ast.EnumDefinition):
            self.__generateenumentity(entity, prefix)

    def __generatetableentity(self, entity):
        name, keyName, tblName = entity.name.value, None, None
        for metaInfo in entity.metaInfoList.metaInfoList:
            if metaInfo[0].value == 'key':
                keyName = metaInfo[1].value
            elif metaInfo[0].value == 'tblname':
                tblName = metaInfo[1].value

        self.fo.write('\n')
        self.fo.write('template<> const char *GetTableName<%s>()\n' % name)
        self.fo.write('{\n')
        self.fo.write('\treturn "%s";\n' % (tblName or name))
        self.fo.write('}\n')

        keyItem, allMembers, requireds, optionals = None, [], [], []
        for member in entity.declarationList.declarationList:
            if isinstance(member, table_ast.TableMemberVarDeclaration):
                ruleItems = requireds if member.memRule.type == 'REQUIRED' else \
                    (optionals if member.memRule.type == 'OPTIONAL' else None)
                for memVar in member.memVarList.varDeclaratorList:
                    ruleItems.append((member.memType, memVar))
                    allMembers.append(ruleItems[-1])
                    if keyName == memVar.value:
                        keyItem = ruleItems[-1]

        if keyItem:
            keyType, keyName = to_cxx_type(keyItem[0]), keyItem[1].value

            self.fo.write('\n')
            self.fo.write('template<> const char *GetTableKeyName<%s>()\n' % name)
            self.fo.write('{\n')
            self.fo.write('\treturn "%s";\n' % keyName)
            self.fo.write('}\n')

            self.fo.write('\n')
            self.fo.write('template<> %s GetTableKeyValue(const %s &entity)\n' % (keyType,name))
            self.fo.write('{\n')
            self.fo.write('\treturn entity.%s;\n' % keyName)
            self.fo.write('}\n')

            self.fo.write('\n')
            self.fo.write('template<> void SetTableKeyValue(%s &entity, %s key)\n' % (name,keyType))
            self.fo.write('{\n')
            self.fo.write('\tentity.%s = key;\n' % keyName)
            self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> const char *GetTableFieldNameByIndex<%s>(size_t index)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tswitch (index)\n')
        self.fo.write('\t{\n')
        self.__generatefieldnamebyindex(requireds, optionals)
        self.fo.write('\t}\n')
        self.fo.write('\treturn "";\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> ssize_t GetTableFieldIndexByName<%s>(const char *name)\n' % name)
        self.fo.write('{\n')
        self.__generatefieldindexbyname(requireds, optionals)
        self.fo.write('\treturn -1;\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> size_t GetTableFieldNumber<%s>()\n' % name)
        self.fo.write('{\n')
        self.fo.write('\treturn %d;\n' % (len(requireds) + (1 if optionals else 0)))
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> std::string GetTableFieldValue(const %s &entity, size_t index)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tswitch (index)\n')
        self.fo.write('\t{\n')
        self.__generateloadfromfield(requireds, optionals)
        self.fo.write('\t}\n')
        self.fo.write('\treturn "";\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void SetTableFieldValue(%s &entity, size_t index, const char *value)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tswitch (index)\n')
        self.fo.write('\t{\n')
        self.__generatesavetofield(requireds, optionals)
        self.fo.write('\t}\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void InitTableValue(%s &entity)\n' % name)
        self.fo.write('{\n')
        self.__generateinitvalue(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.fo.write('{\n')
        self.__generateloadfromstream(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.fo.write('{\n')
        self.__generatesavetostream(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        self.__generateloadfromjson(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tSetJsonObjectValue(value);\n')
        self.__generatesavetojson(allMembers)
        self.fo.write('}\n')

        if optionals:
            self.fo.write('\n')
            self.fo.write('template<> void JsonHelper::TableOptionalsFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
            self.fo.write('{\n')
            self.__generateloadfromjson(optionals)
            self.fo.write('}\n')

            self.fo.write('\n')
            self.fo.write('template<> void JsonHelper::TableOptionalsToJson(const %s &entity, rapidjson::Value &value)\n' % name)
            self.fo.write('{\n')
            self.fo.write('\tSetJsonObjectValue(value);\n')
            self.__generatesavetojson(optionals)
            self.fo.write('}\n')

    def __generatestructentity(self, entity, prefix):
        name = prefix + entity.name.value

        allMembers = []
        for member in entity.declarationList.declarationList:
            if isinstance(member, table_ast.StructMemberVarDeclaration):
                for memVar in member.memVarList.varDeclaratorList:
                    allMembers.append((member.memType, memVar))

        self.fo.write('\n')
        self.fo.write('template<> void InitTableValue(%s &entity)\n' % name)
        self.fo.write('{\n')
        if entity.inheritBase:
            self.fo.write('\tInitTableValue<%s>(entity);\n' % to_cxx_ns(entity.inheritBase))
        self.__generateinitvalue(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.fo.write('{\n')
        if entity.inheritBase:
            self.fo.write('\tLoadFromStream<%s>(entity, stream);\n' % to_cxx_ns(entity.inheritBase))
        self.__generateloadfromstream(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.fo.write('{\n')
        if entity.inheritBase:
            self.fo.write('\tSaveToStream<%s>(entity, stream);\n' % to_cxx_ns(entity.inheritBase))
        self.__generatesavetostream(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        if entity.inheritBase:
            self.fo.write('\tBlockFromJson<%s>(entity, stream);\n' % to_cxx_ns(entity.inheritBase))
        self.__generateloadfromjson(allMembers)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        if entity.inheritBase:
            self.fo.write('\tBlockToJson<%s>(entity, stream);\n' % to_cxx_ns(entity.inheritBase))
        else:
            self.fo.write('\tSetJsonObjectValue(value);\n')
        self.__generatesavetojson(allMembers)
        self.fo.write('}\n')

    def __generateenumentity(self, entity, prefix):
        name = prefix + entity.name.value

        self.fo.write('\n')
        self.fo.write('template<> void InitTableValue(%s &entity)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tentity = %s(0);\n' % name)
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromStream(%s &entity, std::istream &stream)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tStreamHelper::EnumFromStream(entity, stream);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void SaveToStream(const %s &entity, std::ostream &stream)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tStreamHelper::EnumToStream(entity, stream);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void LoadFromText(%s &entity, const char *text)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tJsonHelper::BlockFromJsonText(entity, text);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> std::string SaveToText(const %s &entity)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\treturn JsonHelper::BlockToJsonText(entity);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockFromJson(%s &entity, const rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tEnumFromJson(entity, value);\n')
        self.fo.write('}\n')

        self.fo.write('\n')
        self.fo.write('template<> void JsonHelper::BlockToJson(const %s &entity, rapidjson::Value &value)\n' % name)
        self.fo.write('{\n')
        self.fo.write('\tEnumToJson(entity, value);\n')
        self.fo.write('}\n')

    def __generatefieldnamebyindex(self, requireds, optionals):
        for index,member in enumerate(requireds):
            self.fo.write('\t\tcase %s: return "%s";\n' % (index,member[1].value))
        if optionals:
            self.fo.write('\t\tcase %s: return "optionals";\n' % len(requireds))

    def __generatefieldindexbyname(self, requireds, optionals):
        for index,member in enumerate(requireds):
            self.fo.write('\tif (strcmp(name, "%s") == 0) return %s;\n' % (member[1].value,index))
        if optionals:
            self.fo.write('\tif (strcmp(name, "optionals") == 0) return %s;\n' % len(requireds))

    def __generateloadfromfield(self, requireds, optionals):
        for index,member in enumerate(requireds):
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockToJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return FieldHelper::ToString(entity.%s);\n' % (index,member[1].value))
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockSequenceToJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return JsonHelper::SequenceToJsonText(entity.%s);\n' % (index,member[1].value))
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockAssociativeToJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return JsonHelper::AssociativeToJsonText(entity.%s);\n' % (index,member[1].value))
        if optionals:
            self.fo.write('\t\tcase %d: return JsonHelper::TableOptionalsToJsonText(entity);\n' % len(requireds))

    def __generatesavetofield(self, requireds, optionals):
        for index,member in enumerate(requireds):
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockFromJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return FieldHelper::FromString(entity.%s);\n' % (index,member[1].value))
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockSequenceFromJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return JsonHelper::SequenceFromJsonText(entity.%s);\n' % (index,member[1].value))
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\t\tcase %d: return JsonHelper::BlockAssociativeFromJsonText(entity.%s);\n' % (index,member[1].value))
                else:
                    self.fo.write('\t\tcase %d: return JsonHelper::AssociativeFromJsonText(entity.%s);\n' % (index,member[1].value))
        if optionals:
            self.fo.write('\t\tcase %d: return JsonHelper::TableOptionalsFromJsonText(entity);\n' % len(requireds))

    def __generateinitvalue(self, members):
        for member in members:
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tInitTableValue(%s);\n' % member[1].value)
                elif member[0].parts[0].type in ('STRING',):
                    self.fo.write('\t%s.clear();\n' % member[1].value)
                else:
                    self.fo.write('\t%s = 0;\n' % member[1].value)
            else:
                self.fo.write('\t%s.clear();\n' % member[1].value)

    def __generateloadfromstream(self, members):
        for member in members:
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tLoadFromStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::FromStream(entity.%s, stream);\n' % member[1].value)
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tStreamHelper::BlockSequenceFromStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::SequenceFromStream(entity.%s, stream);\n' % member[1].value)
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\tStreamHelper::BlockAssociativeFromStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::AssociativeFromStream(entity.%s, stream);\n' % member[1].value)

    def __generatesavetostream(self, members):
        for member in members:
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tSaveToStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::ToStream(entity.%s, stream);\n' % member[1].value)
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tStreamHelper::BlockSequenceToStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::SequenceToStream(entity.%s, stream);\n' % member[1].value)
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\tStreamHelper::BlockAssociativeToStream(entity.%s, stream);\n' % member[1].value)
                else:
                    self.fo.write('\tStreamHelper::AssociativeToStream(entity.%s, stream);\n' % member[1].value)

    def __generateloadfromjson(self, members):
        for member in members:
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockSequenceFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tSequenceFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockAssociativeFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tAssociativeFromJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))

    def __generatesavetojson(self, members):
        for member in members:
            if member[0].container is None:
                if isinstance(member[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
            elif member[0].container == 'Sequence':
                if isinstance(member[0].parts[0].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockSequenceToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tSequenceToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
            elif member[0].container == 'Associative':
                if isinstance(member[0].parts[1].parts[0], table_ast.NsIdList):
                    self.fo.write('\tBlockAssociativeToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
                else:
                    self.fo.write('\tAssociativeToJson(entity.%s, value, "%s");\n' % ((member[1].value,)*2))
