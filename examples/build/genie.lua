solution "px"
    configurations { "Debug", "Release" }
    platforms {"x32", "x64"}
    language "C++"
    includedirs {"..","deps"}
    location "build"
    objdir "build/obj"

exampleList = {
"px_render_example_gltf.cpp",
"px_render_example_imgui.cpp",
"px_render_example_rtt.cpp",
"px_render_example_triangle.cpp",
"px_sched_example1.cpp",
"px_sched_example2.cpp",
"px_sched_example3.cpp",
"px_sched_example4.cpp",
"px_sched_example5.cpp",
"px_sched_example6.cpp",
"px_sched_example7.cpp",
"px_sched_example8.cpp",
}

for _,a in ipairs(exampleList) do
  project(a)
  kind "ConsoleApp"
  files{"../"..a}
end
