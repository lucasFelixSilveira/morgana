local json = {}

local function decode_string(str)
    return str:gsub('\\(.)', {
        ['"'] = '"',
        ['\\'] = '\\',
        ['/'] = '/',
        ['b'] = '\b',
        ['f'] = '\f',
        ['n'] = '\n',
        ['r'] = '\r',
        ['t'] = '\t'
    })
end

function json.decode(str)
    local i = 1

    local function skip_spaces()
        while i <= #str and str:sub(i, i):match('%s') do
            i = i + 1
        end
    end

    local function parse()
        skip_spaces()

        if i > #str then
            error("JSON vazio ou incompleto")
        end

        local c = str:sub(i, i)

        -- String
        if c == '"' then
            i = i + 1
            local start = i
            while i <= #str and str:sub(i, i) ~= '"' do
                if str:sub(i, i) == '\\' then
                    i = i + 1
                end
                i = i + 1
            end
            local result = str:sub(start, i - 1)
            i = i + 1
            return decode_string(result)

        -- Número
        elseif c:match('[%d%-]') then
            local start = i
            if c == '-' then i = i + 1 end
            while i <= #str and str:sub(i, i):match('%d') do i = i + 1 end
            if str:sub(i, i) == '.' then
                i = i + 1
                while i <= #str and str:sub(i, i):match('%d') do i = i + 1 end
            end
            if str:sub(i, i):match('[eE]') then
                i = i + 1
                if str:sub(i, i):match('[%+%-]') then i = i + 1 end
                while i <= #str and str:sub(i, i):match('%d') do i = i + 1 end
            end
            local num = tonumber(str:sub(start, i - 1))
            return num

        -- Array
        elseif c == '[' then
            i = i + 1
            local arr = {}
            skip_spaces()

            if str:sub(i, i) == ']' then
                i = i + 1
                return arr
            end

            while i <= #str do
                table.insert(arr, parse())
                skip_spaces()

                if str:sub(i, i) == ']' then
                    i = i + 1
                    return arr
                elseif str:sub(i, i) == ',' then
                    i = i + 1
                    skip_spaces()
                else
                    error("Erro no array")
                end
            end
            error("Array incompleto")

        -- Objeto
        elseif c == '{' then
            i = i + 1
            local obj = {}
            skip_spaces()

            if str:sub(i, i) == '}' then
                i = i + 1
                return obj
            end

            while i <= #str do
                skip_spaces()
                if str:sub(i, i) ~= '"' then
                    error("Chave do objeto inválida")
                end
                local key = parse()

                skip_spaces()
                if str:sub(i, i) ~= ':' then
                    error("Esperado ':' após chave")
                end
                i = i + 1

                obj[key] = parse()
                skip_spaces()

                if str:sub(i, i) == '}' then
                    i = i + 1
                    return obj
                elseif str:sub(i, i) == ',' then
                    i = i + 1
                    skip_spaces()
                else
                    error("Erro no objeto")
                end
            end
            error("Objeto incompleto")

        -- Valores especiais
        elseif c == 't' then
            if str:sub(i, i + 3) == 'true' then
                i = i + 4
                return true
            end
            error("Valor inválido")

        elseif c == 'f' then
            if str:sub(i, i + 4) == 'false' then
                i = i + 5
                return false
            end
            error("Valor inválido")

        elseif c == 'n' then
            if str:sub(i, i + 3) == 'null' then
                i = i + 4
                return nil
            end
            error("Valor inválido")

        else
            error("Caractere inválido: " .. c)
        end
    end

    local success, result = pcall(function()
        return parse()
    end)

    if success then
        skip_spaces()
        if i <= #str then
            error("Caracteres extras após JSON")
        end
        return result
    else
        return nil, result
    end
end

-- Função de teste simples
function json.test()
    local tests = {
        '{"data": [{"kind": 1,"name": "main","params": [],"body": []}]}',
        '{"name": "John", "age": 30}',
        '[1, 2, 3]',
        'true',
        'false',
        'null',
        '42.5'
    }

    for _, test in ipairs(tests) do
        print("Testando: " .. test)
        local result, err = json.decode(test)
        if result then
            print("  ✓ Sucesso")
        else
            print("  ✗ Erro: " .. err)
        end
    end
end

return json
