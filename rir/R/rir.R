# the following functions are intended for the API

rir.markOptimize <- function(what) {
    .Call("rir_markOptimize", what);
}

# Returns TRUE if the argument is a rir-compiled closure.
rir.isValidFunction <- function(what) {
    .Call("rir_isValidFunction", what);
}

# prints the disassembled rir function
rir.disassemble <- function(what) {
    invisible(.Call("rir_disassemble", what))
}

# compiles given closure, or expression and returns the compiled version.
rir.compile <- function(what) {
    .Call("rir_compile", what)
}

rir.eval <- function(what, env = globalEnv()) {
    .Call("rir_eval", what, env);
}

# returns the body of rir-compiled function. The body is the vector containing its ast maps and code objects
rir.body <- function(f) {
    .Call("rir_body", f);
}

rir.analysis.signature <- function(f) {
    x <- .Call("rir_analysis_signature", f)
    result = x[[1]]
    names(result) <- x[[2]]
    result
}

# possible whats:
# * "call": call -> closure -> args -> env
# * "builtin": call -> builtin -> args -> env
# * "special": call -> special -> ast -> env
# * "create promise": promise -> env (warning: this causes the promise to be forced!)
# * "force promise": promise -> env
# * "lookup promise": promise -> env
rir.trace <- function(what, f) {
    invisible(.Call("rir_trace", what, f))
}

# possible hooks:
# * "call"
# * "builtin"
# * "special"
# * "force promise"
# * "lookup promise"
rir.trace.install.default <- function(...) {
    debug <- function(...)
        mapply (
            function(name, argument) {
                cat(paste("[", toupper(name), "]\n"), sep = "")
                .Internal(inspect(argument))
            },
            names(list(...)),
            list(...)
        )

    active.hooks <- list(...)
    set.if.active <- function(hook, closure)
        if (length(active.hooks) == 0 || hook %in% active.hooks) {
            rir.trace(hook, closure)
        } else {
            rir.unset_tracer(hook)
        }

    set.if.active("call", function(call, closure, args, env) {
        cat("--- CALL --------------------------------------------------------\n")
        debug(call=call, closure=closure, args=args, env=env)
    })

    set.if.active("builtin", function(call, builtin, args, env) {
        cat("--- BUILTIN -----------------------------------------------------\n")
        debug(call=call, builtin=builtin, args=args, env=env)
    })

    set.if.active("special", function(call, special, ast, env) {
        cat("--- SPECIAL -----------------------------------------------------\n")
        debug(call=call, special=special, ast=ast, env=env)
    })

    # FIXME passing a promise to this tracer will cause it to be forced, which is not what we want
    #set.if.active("create promise", function(promise, env) {
    #    cat("--- CREATE PROMISE ----------------------------------------------\n")
    #    debug(promise=promise, env=env)
    #})

    set.if.active("force promise", function(promise, value, env) {
        cat("--- FORCE PROMISE -----------------------------------------------\n")
        debug(promise=promise, value=value, env=env)
    })

    set.if.active("lookup promise", function(promise, value, env) {
        cat("--- LOOKUP PROMISE ----------------------------------------------\n")
        debug(promise=promise, value=value, env=env)
    })

    invisible(NULL);
}