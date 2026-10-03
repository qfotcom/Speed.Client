.pragma library

function httpStatusFromLine(line) {
    if (!line || line === "—")
        return -1
    var match = String(line).match(/^HTTP\s+(\d+)/)
    if (!match)
        return -1
    return parseInt(match[1], 10)
}

function isHttpSuccess(line) {
    var code = httpStatusFromLine(line)
    return code >= 200 && code < 300
}

function isHttpFailure(line) {
    var code = httpStatusFromLine(line)
    return code >= 400 || code === 0
}
