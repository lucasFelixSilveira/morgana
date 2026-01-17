local SymbolTableManager = {
    scopes = {},
}

function SymbolTableManager:newScope()
    table.insert(self.scopes, {})
end

function SymbolTableManager:endScope()
    table.remove(self.scopes)
end

function SymbolTableManager:add(name, info)
    local currentScope = self.scopes[#self.scopes]
    if currentScope then currentScope[name] = info
    else error("No one active scope!") end
end

function SymbolTableManager:lookup(name)
    for i = #self.scopes, 1, -1 do
        if self.scopes[i][name] then return self.scopes[i][name] end
    end
    return nil
end

return SymbolTableManager
