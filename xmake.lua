set_project("ce-custom-lua-decryptor")

set_languages("c++2b")
set_warnings("all")
add_rules("mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_requires("cpptrace")
target("ce-custom-lua-decryptor")
    set_kind("binary")
    add_packages("cpptrace")
    set_encodings("utf-8")
    add_files("src/*.cc", "src/*/**.cc") 
