local function Map(path, mapRules)
    for patternOrPatterns, funcTable in pairs(mapRules) do
        if type(patternOrPatterns) == "table" then
            for _, pattern in ipairs(patternOrPatterns) do
                local results = { string.match(path, pattern) }
                if results[1] ~= nil then
                    return funcTable, results
                end
            end
        else
            local results = { string.match(path, patternOrPatterns) }
            if results[1] ~= nil then
                return funcTable, results
            end
        end
    end
    return nil, nil
end

local function AreInputsNewer(inputs, outputs)
--    local latestOutputTimestamp = 0;
--    for _, output in ipairs(outputs)
--        latestOutputTimestamp = math.max(latestOutputTimestamp,
--                                         GetFileTimestamp(output));
--    end
--    for _, input in ipairs(inputs)
--        if GetFileTimestamp(input) > latestOutputTimestamp then
--            return true;
--        end
--    end
--    return false;
    return true;
end

local function InputNeedsCompile(inputPath, mapRules)
    local funcTable, matchResults = Map(inputPath, mapRules)
    if funcTable == nil then
        return false
    end
    local inputs, outputs = funcTable.Parse(inputPath, unpack(matchResults))
    return AreInputsNewer(inputs, outputs)
end

local function GetManifestIterator()
    return io.lines(GetManifestPath())
end

BuildSystem = {}

BuildSystem.fileIter = nil
BuildSystem.nextPath = nil
BuildSystem.stack = List:New()

function BuildSystem:Reset()
    self.fileIter = nil
    self.nextPath = nil
    self.stack = List:New()
end

function BuildSystem:CompileNext(mapRules)
    if self.fileIter == nil then
        self.fileIter = GetManifestIterator()
        self.nextPath = self.fileIter()
    end

    local done = false
    while (not done) and (not (self.stack:IsEmpty() and self.nextPath == nil)) do
        if self.stack:IsEmpty() then
            self.stack:InsertTail(self.nextPath)
            self.nextPath = self.fileIter()
        end

        local path = self.stack:PeekTail()
        local funcTable, matchResults = Map(path, mapRules)
        if funcTable == nil then
            print(string.format("Warning: no compilation rule found for '%s'", path))
            self.stack:RemoveTail()
        else
            local inputs, outputs = funcTable.Parse(path, unpack(matchResults))
            local mustCompileInputs = false
            for _, input in ipairs(inputs) do
                if InputNeedsCompile(input, mapRules) then
                    self.stack:InsertTail(input)
                    mustCompileInputs = true
                end
            end
            if not mustCompileInputs then
                self.stack:RemoveTail()
                if AreInputsNewer(inputs, outputs) then
                    local result = funcTable.Execute(inputs, outputs)
                    if result then
                        print("Success")
                    else
                        print("Error")
                    end
                    done = true
                end
            end
        end
    end

    return self.stack:IsEmpty() or self.nextPath == nil
end
