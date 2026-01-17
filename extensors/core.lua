local io = require 'io'

local core = {};

core.kind = {
    ret = 0,
    _function = 1,
    desconstructor = 2,
    alias = 3,
    allocation = 4,
    store = 5,
    load = 6,
    vector_allocation = 7,
    sample = 8,
    mock = 9,
    get_pointer_element = 10
};

core.desconstructor = {
    that = 0
}

core.os = {
    windows = "Windows",
    linux = "Linux",
    macos = "Darwin"
}

core.already_used_mprint = false;
core.mprint = function(msg)
    if core.already_used_mprint then
        print("morgana:core (lua) " .. msg)
    else
        print("\nmorgana:core (lua) " .. msg)
    end
end

core.getos = function()
    local fh = io.popen("uname -s 2>/dev/null")
    if fh then
        local os = fh:read("*a"):gsub("%s+", "")
        fh:close()
        if os ~= "" then return os end
    end
    return core.os.windows
end

return core
