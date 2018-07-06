import sys
import ply.lex

class TableLexer:
    reserved_map = {}

    reserved = (
        'TABLE', 'STRUCT', 'ENUM', 'MAP', 'REQUIRED', 'OPTIONAL',
        'STRING', 'CHAR', 'BOOL', 'FLOAT', 'DOUBLE', 'INT8', 'UINT8',
        'INT16', 'UINT16', 'INT32', 'UINT32', 'INT64', 'UINT64',
    )

    tokens = reserved + (
        'ID', 'NUMBER', 'PREPROCESSOR', 'COMMENT',
        'LBRACKET', 'RBRACKET', 'LBRACE', 'RBRACE',
        'LT', 'GT',
        'EQUALS', 'COMMA', 'COLON', 'SEMI',
        'NSCOLON',
    )

    t_NUMBER = r'(0[xX][a-fA-F\d]+)|(0[0-7]*)|(-?[1-9]\d*)'

    t_LBRACKET = r'\['
    t_RBRACKET = r'\]'
    t_LBRACE = r'\{'
    t_RBRACE = r'\}'
    t_LT = r'<'
    t_GT = r'>'
    t_EQUALS = r'='
    t_COMMA = r','
    t_COLON = r':'
    t_SEMI = r';'

    t_NSCOLON = r'::'

    t_ignore = '\x20\t\f\v\r'

    def t_ID(self, t):
        r'[A-Za-z_][\w]*'
        t.type = self.reserved_map.get(t.value, 'ID')
        t.value = self.Token(t.type, t.value, t.lineno)
        return t

    def t_PREPROCESSOR(self, t):
        r'\#.*'
        t.value = self.Token(t.type, t.value, t.lineno)
        return t

    def t_COMMENT(self, t):
        r'//.*'
        t.value = self.Token(t.type, t.value, t.lineno)
        return t

    def t_newline(self, t):
        r'\n+'
        t.lexer.lineno += len(t.value)

    def t_error(self, t):
        print('%s(%d,%d): Illegal char %s.' % (self.file, self.lexer.lineno,
            self.find_column(self.input, t), repr(t.value[0])))
        sys.exit(1)

    def __init__(self, **kwargs):
        self.lexer = ply.lex.lex(module=self, **kwargs)
        if not self.reserved_map:
            self.init()

    def parse(self, file):
        self.file = file
        with open(file, 'r') as fo:
            self.input = fo.read()
        self.lexer.input(self.input)

    @classmethod
    def init(cls):
        for r in cls.reserved:
            cls.reserved_map[r.lower()] = r

    @staticmethod
    def find_column(input, token):
        return token.lexpos - input.rfind('\n', 0, token.lexpos)

    class Token:
        def __init__(self, type, value, lineno):
            self.type, self.value, self.lineno = type, value, lineno
