local function Map(path, mapRules)
    for pattern, func in pairs(mapRules) do
        local results = string.match(path, pattern)
        if results ~= nil then
            return func(path, results)
        end
    end
    return nil;
end

local function Parse(taskline, tasks)
    local key = taskline[1]
    return tasks[key].Parse(taskline)
end

local function Execute(taskline, tasks)
    local key = taskline[1]
    return tasks[key].Execute(taskline)
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

BuildSystem = {}

BuildSystem.fileIter = nil
BuildSystem.nextFile = nil
BuildSystem.tasklines = List:New()

function BuildSystem:Reset()
    self.fileIter = nil
    self.nextFile = nil
    self.tasklines = List:New()
end

function BuildSystem:CompileNext(mapRules, tasks)
    if self.fileIter == nil then
        self.fileIter = GetContentDirIterator()
        self.nextFile = self.fileIter()
    end

    local done = false
    while (not done) and (self.nextFile ~= nil) do
        local tl = Map(self.nextFile, mapRules)
        if tl ~= nil then
            local inputs, outputs = Parse(tl, tasks)
            if AreInputsNewer(inputs, outputs) then
                local result = Execute(tl, tasks)
                if type(result) == "table" then
                    for _, newTaskLine in ipairs(result) do
                        self.tasklines:InsertTail(newTaskLine)
                    end
                else
                    -- TODO: Handle success/fail return code
                end
                done = true
            end
        end
        self.nextFile = self.fileIter()
    end
    if done then
        return true
    end

    while (not done) and (not self.tasklines:IsEmpty()) do
        local taskline = self.tasklines:RemoveHead()
        local inputs, outputs = Parse(taskline, tasks)
        if AreInputsNewer(inputs, outputs) then
            local result = Execute(tl, tasks)
            if type(result) == "table" then
                for _, newTaskLine in ipairs(result) do
                    self.tasklines:InsertTail(newTaskLine)
                end
            else
                -- TODO: Handle success/fail return code
            end
            done = true
        end
    end
    return done
end
