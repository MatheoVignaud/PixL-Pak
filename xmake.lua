add_rules("mode.debug", "mode.release")

set_languages("c++17")
set_optimize("none")


target("PixL-Paker")
    set_kind("binary")
    add_includedirs("include")
    add_files("main.cpp")
