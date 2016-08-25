List = {}

function List:New()
    list = { first = 0, last = -1 }
    setmetatable(list, self)
    self.__index = self
    return list
end

function List:IsEmpty()
    return self.first > self.last
end

function List:InsertHead(value)
    local first = self.first - 1
    self.first = first
    list[first] = value
end

function List:InsertTail(value)
    local last = self.last + 1
    self.last = last
    list[last] = value
end

function List:RemoveHead()
    local first = self.first
    if first > self.last then error("list is empty") end
    local value = list[first]
    list[first] = nil        -- to allow garbage collection
    self.first = first + 1
    return value
end

function List:RemoveTail()
    local last = self.last
    if self.first > last then error("list is empty") end
    local value = list[last]
    list[last] = nil         -- to allow garbage collection
    self.last = last - 1
    return value
end
