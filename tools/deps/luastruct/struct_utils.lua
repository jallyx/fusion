require('struct_helper')

function SaveToBinary(entity, func)
    local stringBuilder = StringBuilder:new()
    func(stringBuilder, entity)
    return stringBuilder.tostr()
end

function LoadFromBinary(func, s, r)
    local stringBuffer = StringBuffer:new(s, r)
    return func(stringBuffer)
end

function NewInstanceFromProto(proto)
    local entity = {}
    setmetatable(entity, proto)
    proto.__index = proto
    return entity
end
