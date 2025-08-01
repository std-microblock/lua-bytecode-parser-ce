set_project("ce-custom-lua-decryptor")

set_languages("c++2b")
set_warnings("all")
add_rules("mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

target("ce-custom-lua-decryptor")
    set_kind("binary")
    set_encodings("utf-8")
    add_files("src/*.cc", "src/*/**.cc") 
