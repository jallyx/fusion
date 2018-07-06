import sys
import ply.yacc

import table_ast
import table_lexer

class TableParser:
    tokens = None

    def p_translation_unit_1(self, p):
        'translation_unit : external_declaration'
        p[0] = table_ast.TranslationUnit(None, p[1])

    def p_translation_unit_2(self, p):
        'translation_unit : translation_unit external_declaration'
        p[0] = table_ast.TranslationUnit(p[1], p[2])

    def p_external_declaration_1(self, p):
        'external_declaration : SEMI'
        pass

    def p_external_declaration_2(self, p):
        'external_declaration : PREPROCESSOR'
        p[0] = table_ast.Preprocessor(p[1])

    def p_external_declaration_3(self, p):
        'external_declaration : COMMENT'
        p[0] = table_ast.Comment(p[1])

    def p_external_declaration_4(self, p):
        '''external_declaration : table_definition
                                | struct_definition
                                | enum_definition'''
        p[0] = p[1]

    def p_table_definition(self, p):
        'table_definition : TABLE ID \
                            LBRACE table_declaration_list RBRACE \
                            extra_table_info'
        p[0] = table_ast.TableDefinition(p[2], p[4], p[6])

    def p_struct_definition(self, p):
        '''struct_definition : STRUCT ID optional_inherit_base \
                               LBRACE struct_declaration_list RBRACE'''
        p[0] = table_ast.StructDefinition(p[2], p[3], p[5])

    def p_enum_definition(self, p):
        'enum_definition : ENUM ID LBRACE enum_declaration_list RBRACE'
        p[0] = table_ast.EnumDefinition(p[2], p[4])

    def p_table_declaration_list_1(self, p):
        'table_declaration_list : table_declaration'
        p[0] = table_ast.TableDeclarationList(None, p[1])

    def p_table_declaration_list_2(self, p):
        'table_declaration_list : table_declaration_list table_declaration'
        p[0] = table_ast.TableDeclarationList(p[1], p[2])

    def p_struct_declaration_list_1(self, p):
        'struct_declaration_list : struct_declaration'
        p[0] = table_ast.StructDeclarationList(None, p[1])

    def p_struct_declaration_list_2(self, p):
        'struct_declaration_list : struct_declaration_list struct_declaration'
        p[0] = table_ast.StructDeclarationList(p[1], p[2])

    def p_enum_declaration_list_1(self, p):
        'enum_declaration_list : enum_declaration'
        p[0] = table_ast.EnumDeclarationList(None, p[1])

    def p_enum_declaration_list_2(self, p):
        'enum_declaration_list : enum_declaration_list enum_declaration'
        p[0] = table_ast.EnumDeclarationList(p[1], p[2])

    def p_table_declaration_1(self, p):
        'table_declaration : SEMI'
        pass

    def p_table_declaration_2(self, p):
        'table_declaration : COMMENT'
        p[0] = table_ast.Comment(p[1])

    def p_table_declaration_3(self, p):
        'table_declaration : member_type_declaration'
        p[0] = p[1]

    def p_table_declaration_4(self, p):
        '''table_declaration : REQUIRED member_var_declaration
                             | OPTIONAL member_var_declaration'''
        p[0] = table_ast.TableMemberVarDeclaration(p[1], *p[2])

    def p_struct_declaration_1(self, p):
        'struct_declaration : SEMI'
        pass

    def p_struct_declaration_2(self, p):
        'struct_declaration : COMMENT'
        p[0] = table_ast.Comment(p[1])

    def p_struct_declaration_3(self, p):
        'struct_declaration : member_type_declaration'
        p[0] = p[1]

    def p_struct_declaration_4(self, p):
        'struct_declaration : member_var_declaration'
        p[0] = table_ast.StructMemberVarDeclaration(*p[1])

    def p_enum_declaration_1(self, p):
        '''enum_declaration : COMMA
                            | SEMI'''
        pass

    def p_enum_declaration_2(self, p):
        'enum_declaration : COMMENT'
        p[0] = table_ast.Comment(p[1])

    def p_enum_declaration_3(self, p):
        'enum_declaration : ID'
        p[0] = table_ast.EnumDeclaration(p[1])

    def p_enum_declaration_4(self, p):
        '''enum_declaration : ID EQUALS ns_id
                            | ID EQUALS NUMBER'''
        p[0] = table_ast.EnumDeclaration(p[1], p[3])

    def p_member_type_declaration_1(self, p):
        '''member_type_declaration : table_definition
                                   | struct_definition
                                   | enum_definition'''
        p[0] = p[1]

    def p_member_var_declaration(self, p):
        'member_var_declaration : type_specifier var_declarator_list'
        p[0] = (p[1], p[2])

    def p_type_specifier_1(self, p):
        '''type_specifier : CHAR
                          | BOOL
                          | FLOAT
                          | DOUBLE
                          | STRING
                          | INT8
                          | UINT8
                          | INT16
                          | UINT16
                          | INT32
                          | UINT32
                          | INT64
                          | UINT64
                          | ns_id'''
        p[0] = table_ast.MemberVarTypeSpecifier(None, p[1])

    def p_type_specifier_2(self, p):
        'type_specifier : type_specifier LBRACKET RBRACKET'
        p[0] = table_ast.MemberVarTypeSpecifier('Sequence', p[1])

    def p_type_specifier_3(self, p):
        'type_specifier : MAP LT type_specifier COMMA type_specifier GT'
        p[0] = table_ast.MemberVarTypeSpecifier('Associative', p[3], p[5])

    def p_var_declarator_list_1(self, p):
        'var_declarator_list : ID'
        p[0] = table_ast.MemberVarVarDeclaratorList(None, p[1])

    def p_var_declarator_list_2(self, p):
        'var_declarator_list : var_declarator_list COMMA ID'
        p[0] = table_ast.MemberVarVarDeclaratorList(p[1], p[3])

    def p_extra_meta_info_1(self, p):
        'extra_meta_info : ID EQUALS ID'
        p[0] = table_ast.ExtraMetaInfoList(None, p[1], p[3])

    def p_extra_table_info_2(self, p):
        'extra_table_info : extra_meta_info COMMA ID EQUALS ID'
        p[0] = table_ast.ExtraMetaInfoList(p[1], p[3], p[5])

    def p_ns_id_1(self, p):
        'ns_id : ID'
        p[0] = table_ast.NsIdList(None, p[1])

    def p_ns_id_2(self, p):
        'ns_id : ns_id NSCOLON ID'
        p[0] = table_ast.NsIdList(p[1], p[3])

    def p_inherit_base(self, p):
        'inherit_base : COLON ns_id'
        p[0] = p[2]

    def p_optional_inherit_base(self, p):
        '''optional_inherit_base : inherit_base
                                 | empty'''
        p[0] = p[1]

    def p_empty(self, p):
        'empty :'
        pass

    def p_error(self, t):
        if t:
            strpos = '%s(%d,%d)' % (self.lexer.file,
                t.lineno, self.lexer.find_column(self.lexer.input, t))
            print('%s: Syntax error at %s.' % (strpos, repr(t.value.value
                if isinstance(t.value, table_lexer.TableLexer.Token)
                else t.value)))
        else:
            print("%s: Syntax error at EOF." % self.lexer.file)
        sys.exit(1)

    def __init__(self, **kwargs):
        if not self.tokens:
            self.tokens = table_lexer.TableLexer.tokens
        self.yacc = ply.yacc.yacc(module=self, **kwargs)

    def parse(self, lexer):
        self.lexer = lexer
        return self.yacc.parse(lexer=self.lexer.lexer)
