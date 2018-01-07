PARSE_STATE = (
    PARSE_STATE_NULL,
    PARSE_STATE_BLOCK,
    PARSE_STATE_MEMBER,
) = range(3)

ENTITY_TYPE = (
    ENTITY_TYPE_ROOT,
    ENTITY_TYPE_TABLE,
    ENTITY_TYPE_STRUCT,
    ENTITY_TYPE_ENUM,
    ENTITY_TYPE_SPACE,
    ENTITY_TYPE_HEADER,
    ENTITY_TYPE_COMMENT,
) = range(7)

RULE_TYPE = (
    RULE_TYPE_REQUIRED,
    RULE_TYPE_OPTIONAL,
) = range(2)

DATA_TYPE = (
    DATA_TYPE_CHAR,
    DATA_TYPE_INT8,
    DATA_TYPE_UINT8,
    DATA_TYPE_INT16,
    DATA_TYPE_UINT16,
    DATA_TYPE_INT32,
    DATA_TYPE_UINT32,
    DATA_TYPE_INT64,
    DATA_TYPE_UINT64,
    DATA_TYPE_BOOL,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRING,
) = range(13)

def get_rule_type(label):
    labels = (
        'required',
        'optional',
    )
    if len(RULE_TYPE) != len(labels):
        raise InternalError, "get_rule_type() labels error"
    try:
        return labels.index(label)
    except ValueError:
        return -1

def get_data_type(label):
    labels = (
        'char',
        'int8',
        'uint8',
        'int16',
        'uint16',
        'int32',
        'uint32',
        'int64',
        'uint64',
        'bool',
        'float',
        'double',
        'string',
    )
    if len(DATA_TYPE) != len(labels):
        raise InternalError, "get_data_type() labels error"
    try:
        return labels.index(label)
    except ValueError:
        return -1

def map_data_type(stype):
    type = get_data_type(stype)
    if type == DATA_TYPE_STRING:
        return 'std::string'
    return stype

class InternalError(Exception):
    pass

class DataError(Exception):
    pass
