function MoveToBase(bb, speed)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.pos = obj.pos - speed
        obj:PrintMessage(string.format("move to base %d ...", obj.pos))
        return obj:IsNearBase() and AINodeStatus.Finished or AINodeStatus.Running
    end)
    return node
end

function MoveToRes(bb, speed)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.pos = obj.pos + speed
        obj:PrintMessage(string.format("move to res %d ...", obj.pos))
        return obj:IsNearRes() and AINodeStatus.Finished or AINodeStatus.Running
    end)
    return node
end

function StartReturnRes(bb)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.status = SCVStatus.Returning
        obj:PrintMessage('start return res ...')
        return AINodeStatus.Finished
    end)
    return node
end

function StartGatherRes(bb)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.status = SCVStatus.Gathering
        obj:PrintMessage('start gather res ...')
        return AINodeStatus.Finished
    end)
    return node
end

function ReturnRes(bb, number)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.res = obj.res - number
        obj:PrintMessage(string.format("return res %d ...", obj.res))
        return obj:IsEmptyRes() and AINodeStatus.Finished or AINodeStatus.Running
    end)
    node:SetFinishScriptable(function(obj)
        obj.status = SCVStatus.Invalid
    end)
    return node
end

function GatherRes(bb, number)
    local node = AIScriptableNode(bb)
    node:SetKernelScriptable(function(obj)
        obj.res = obj.res + number
        obj:PrintMessage(string.format("gather res %d ...", obj.res))
        return obj:IsFullRes() and AINodeStatus.Finished or AINodeStatus.Running
    end)
    node:SetFinishScriptable(function(obj)
        obj.status = SCVStatus.Invalid
    end)
    return node
end
