class Node:
    def __init__(self):
        pass

class Preprocessor(Node):
    def __init__(self, text):
        self.text = text

class Comment(Node):
    def __init__(self, text):
        self.text = text

class TableMemberVarDeclaration(Node):
    def __init__(self, memRule, memType, memVarList):
        self.memRule = memRule
        self.memType = memType
        self.memVarList = memVarList

class TableDeclarationList(Node):
    def __init__(self, other, declaration):
        self.declarationList = other and other.declarationList or []
        if declaration:
            self.declarationList.append(declaration)

class TableDefinition(Node):
    def __init__(self, name, declarationList, metaInfoList):
        self.name = name
        self.declarationList = declarationList
        self.metaInfoList = metaInfoList

class StructMemberVarDeclaration(Node):
    def __init__(self, memType, memVarList):
        self.memType = memType
        self.memVarList = memVarList

class StructDeclarationList(Node):
    def __init__(self, other, declaration):
        self.declarationList = other and other.declarationList or []
        if declaration:
            self.declarationList.append(declaration)

class StructDefinition(Node):
    def __init__(self, name, inheritBase, declarationList):
        self.name = name
        self.inheritBase = inheritBase
        self.declarationList = declarationList

class EnumDeclaration(Node):
    def __init__(self, memName, memValue = None):
        self.memName = memName
        self.memValue = memValue

class EnumDeclarationList(Node):
    def __init__(self, other, declaration):
        self.declarationList = other and other.declarationList or []
        if declaration:
            self.declarationList.append(declaration)

class EnumDefinition(Node):
    def __init__(self, name, declarationList):
        self.name = name
        self.declarationList = declarationList

class MemberVarTypeSpecifier(Node):
    # container: None, 'Sequence', 'Associative'
    def __init__(self, container, *parts):
        self.container = container
        self.parts = parts

class MemberVarVarDeclaratorList(Node):
    def __init__(self, other, varName):
        self.varDeclaratorList = other and other.varDeclaratorList or []
        self.varDeclaratorList.append(varName)

class ExtraMetaInfoList(Node):
    def __init__(self, other, infoName, infoValue):
        self.metaInfoList = other and other.metaInfoList or []
        self.metaInfoList.append((infoName, infoValue))

class NsIdList(Node):
    def __init__(self, other, id):
        self.idList = other and other.idList or []
        self.idList.append(id)

class TranslationUnit(Node):
    def __init__(self, other, externalDeclaration):
        self.externalDeclarations = other and other.externalDeclarations or []
        if externalDeclaration:
            self.externalDeclarations.append(externalDeclaration)
