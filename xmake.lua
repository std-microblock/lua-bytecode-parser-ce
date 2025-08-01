set_project("lua-bytecode-parser")

set_languages("c++2b")
set_warnings("all")
add_rules("mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_requires("cpptrace")

target("lua-bytecode-parser")
    set_kind("static")
    set_encodings("utf-8")
    add_files("src/lua_bytecode_parser.cc")
    add_headerfiles("src/lua_bytecode_parser.h")
    add_includedirs("src")

target("lua-bytecode-parser-example")
    set_kind("binary")
    set_encodings("utf-8")
    add_files("src/example.cc")
    add_deps("lua-bytecode-parser")
    add_packages("cpptrace")
