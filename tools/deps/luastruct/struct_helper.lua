StringBuilder = {}
function StringBuilder:new(t)
    local obj = {t = t or {}}
    setmetatable(obj, self)
    self.__index = self
    return obj
end
function StringBuilder:tostr()
    return table.concat(self.t)
end
function StringBuilder:append(str)
    table.insert(self.t, str)
end

function StringBuilder:WriteInt8(val)
    self:append(string.pack('i1', val))
end
function StringBuilder:WriteUInt8(val)
    self:append(string.pack('I1', val))
end
function StringBuilder:WriteInt16(val)
    self:append(string.pack('i2', val))
end
function StringBuilder:WriteUInt16(val)
    self:append(string.pack('I2', val))
end
function StringBuilder:WriteInt32(val)
    self:append(string.pack('i4', val))
end
function StringBuilder:WriteUInt32(val)
    self:append(string.pack('I4', val))
end
function StringBuilder:WriteInt64(val)
    self:append(string.pack('i8', val))
end
function StringBuilder:WriteUInt64(val)
    self:append(string.pack('I8', val))
end
function StringBuilder:WriteChar(val)
    self:append(string.pack('c1', val))
end
function StringBuilder:WriteBool(val)
    self:append(string.pack('i1', val and 1 or 0))
end
function StringBuilder:WriteFloat(val)
    self:append(string.pack('f', val))
end
function StringBuilder:WriteDouble(val)
    self:append(string.pack('d', val))
end
function StringBuilder:WriteString(val)
    self:append(string.pack('s4', val))
end

function StringBuilder:WriteArray(array, func)
    local length = #array
    self:WriteUInt32(length)
    for i = 1, length do
        func(self, array[i])
    end
end

StringBuffer = {}
function StringBuffer:new(s, r)
    local obj = {s = s or "", r = r or 0}
    setmetatable(obj, self)
    self.__index = self
    return obj
end
function StringBuffer:result(val, pos)
    self.r = pos
    return val
end

function StringBuffer:ReadInt8()
    return self:result(string.unpack('i1', self.s, self.r))
end
function StringBuffer:ReadUInt8()
    return self:result(string.unpack('I1', self.s, self.r))
end
function StringBuffer:ReadInt16()
    return self:result(string.unpack('i2', self.s, self.r))
end
function StringBuffer:ReadUInt16()
    return self:result(string.unpack('I2', self.s, self.r))
end
function StringBuffer:ReadInt32()
    return self:result(string.unpack('i4', self.s, self.r))
end
function StringBuffer:ReadUInt32()
    return self:result(string.unpack('I4', self.s, self.r))
end
function StringBuffer:ReadInt64()
    return self:result(string.unpack('i8', self.s, self.r))
end
function StringBuffer:ReadUInt64()
    return self:result(string.unpack('I8', self.s, self.r))
end
function StringBuffer:ReadChar()
    return self:result(string.unpack('c1', self.s, self.r))
end
function StringBuffer:ReadBool()
    return self:result(string.unpack('i1', self.s, self.r)) ~= 0
end
function StringBuffer:ReadFloat()
    return self:result(string.unpack('f', self.s, self.r))
end
function StringBuffer:ReadDouble()
    return self:result(string.unpack('d', self.s, self.r))
end
function StringBuffer:ReadString()
    return self:result(string.unpack('s4', self.s, self.r))
end

function StringBuffer:ReadArray(func)
    local array = {}
    local length = self:ReadUInt32()
    for i = 1, length do
        array[i] = func(self)
    end
    return array
end
