function index(request)
    local method = request.method
    local arguments = request.arguments
    local arg1 = arguments[0]
    return {
        method = method,
        name = arg1
    }
end