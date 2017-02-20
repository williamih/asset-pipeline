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

local function AreInputsNewer(inputs, additionalInputs, outputs)
    local latestOutputTimestamp = 0;
    for _, output in ipairs(outputs) do
        latestOutputTimestamp = math.max(latestOutputTimestamp,
                                         GetFileTimestamp(output))
    end
    for _, input in ipairs(inputs) do
        if GetFileTimestamp(input) > latestOutputTimestamp then
            return true
        end
    end
    if additionalInputs then
        for _, input in ipairs(additionalInputs) do
            if GetFileTimestamp(input) > latestOutputTimestamp then
                return true
            end
        end
    end
   return false
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

local function GetArrayIterator(paths)
    local max = #paths
    local index = 1
    return function()
        if index <= max then
            local value = paths[index]
            index = index + 1
            return value
        end
        return nil
    end
end

local function OnSuccess(inputs, additionalInputs, outputs)
    ClearCompileError(inputs, additionalInputs, outputs)
    for _, output in ipairs(outputs) do
        NotifyAssetCompile(output)
        ClearDependencies(output)
        for _, input in ipairs(inputs) do
            RecordDependency(output, input)
        end
        for _, additionalInput in ipairs(additionalInputs) do
            RecordDependency(output, additionalInput)
        end
    end
end

local function OnFailure(inputs, additionalInputs, outputs, errorMessage)
    RecordCompileError(inputs, additionalInputs, outputs, errorMessage)
end

local function FileReadAllBytes(path)
    local inp = io.open(path, "rb")
    if not inp then return nil end
    local data = inp:read("*all")
    assert(inp:close())
    return data
end

local function GetAuxiliaryInputs(inputs, closure)
    assert(closure)
    local auxInputsSet = {}
    local failedSet = nil
    local queue = List:New()
    for _, inputPath in ipairs(inputs) do
        queue:InsertTail(inputPath)
    end
    while not queue:IsEmpty() do
        local inputPath = queue:RemoveHead()
        local contents = FileReadAllBytes(inputPath)
        if contents then
            local additionalPaths = closure(inputPath, contents)
            for _, auxPath in ipairs(additionalPaths) do
                queue:InsertTail(auxPath)
                auxInputsSet[auxPath] = true
            end
        else
            if not failedSet then failedSet = {} end
            failedSet[inputPath] = true
        end
    end
    local auxiliaryInputs = {}
    for k, _ in pairs(auxInputsSet) do
        auxiliaryInputs[#auxiliaryInputs+1] = k
    end
    local failed = nil
    if failedSet then
        failed = {}
        for k, _ in pairs(failedSet) do
            failed[#failed+1] = k
        end
    end
    return auxiliaryInputs, failed
end

local function GetAdditionalInputsDoNotExistMessage(paths)
    local result = ""
    for _, path in ipairs(paths) do
        result = result..string.format("Referenced file '%s' does not exist.\n", path)
    end
    return result
end

BuildSystem = {}

BuildSystem.fileIter = nil
BuildSystem.nextPath = nil
BuildSystem.stack = List:New()

function BuildSystem:Setup(paths)
    if paths == nil then
        self.fileIter = GetManifestIterator()
    else
        self.fileIter = GetArrayIterator(paths)
    end
    self.nextPath = self.fileIter()
    self.stack = List:New()
end

function BuildSystem:CompileNext(mapRules)
    local done = false
    local success = false

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
            local inputs, outputs, closure = funcTable.Parse(path, unpack(matchResults))
            local auxiliaryInputs = nil
            local failedPaths = nil
            if closure then
                auxiliaryInputs, failedPaths = GetAuxiliaryInputs(inputs, closure)
            end
            if failedPaths then
                self.stack:RemoveTail()
                local message = GetAdditionalInputsDoNotExistMessage(failedPaths)
                OnFailure(inputs, auxiliaryInputs, outputs, message)
                done = true
                success = false
            else
                local mustCompileInputs = false
                for _, input in ipairs(inputs) do
                    if InputNeedsCompile(input, mapRules) then
                        self.stack:InsertTail(input)
                        mustCompileInputs = true
                    end
                end
                if auxiliaryInputs then
                    for _, input in ipairs(auxiliaryInputs) do
                        if InputNeedsCompile(input, mapRules) then
                            self.stack:InsertTail(input)
                            mustCompileInputs = true
                        end
                    end
                end
                if not mustCompileInputs then
                    self.stack:RemoveTail()
                    if AreInputsNewer(inputs, auxiliaryInputs, outputs) then
                        success, errorMessage = funcTable.Execute(inputs, outputs)
                        if success then
                            OnSuccess(inputs, auxiliaryInputs, outputs)
                        else
                            OnFailure(inputs, auxiliaryInputs, outputs, errorMessage)
                        end
                        done = true
                    end
                end
            end
        end
    end

    return done, success
end
