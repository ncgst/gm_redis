-- Defines which version of the project generator to use, by default
-- (can be overridden by the environment variable PROJECT_GENERATOR_VERSION)
PROJECT_GENERATOR_VERSION = 3

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
if gmcommon == nil then
	error("you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
end

include(gmcommon)

local ROOT = path.getdirectory(_SCRIPT)            -- premake5.lua 所在目录
local REDIS_FOLDER = path.join(ROOT, "cpp_redis")
local TACOPIE_FOLDER = path.join(REDIS_FOLDER, "tacopie")

CreateWorkspace({name = "redis.core", abi_compatible = false})
	CreateProject({serverside = true})
		links({"cpp_redis", "tacopie"})
		includedirs({
			path.join(REDIS_FOLDER, "includes"),
			path.join(TACOPIE_FOLDER, "includes"),
		})
		IncludeLuaShared()

		filter("system:windows")
			links("ws2_32")
		filter("system:linux")
			disablewarnings({"unused-variable"})
            links({ "pthread" })
            buildoptions({ "-pthread" })
            linkoptions({ "-pthread" })
			defines("DEDICATED") -- All linux build focus Linux dedicated servers.
		filter({})

	group("dependencies")
		project("cpp_redis")
			kind("StaticLib")
			includedirs({
				path.join(REDIS_FOLDER, "includes"),
				path.join(TACOPIE_FOLDER, "includes"),
			})
			files({
				path.join(REDIS_FOLDER, "sources/**.cpp"),
				path.join(REDIS_FOLDER, "includes/cpp_redis/**"),
			})
			vpaths({
				["Source files/*"] = REDIS_FOLDER .. "/sources/**.cpp",
				["Header files/*"] = REDIS_FOLDER .. "/includes/cpp_redis/**"
			})
			links("tacopie")

			filter("system:windows")
				files(path.join(REDIS_FOLDER, "sources/network/windows_impl/*.cpp"))
			filter("system:not windows")
				files(path.join(REDIS_FOLDER, "sources/network/unix_impl/*.cpp"))
			filter({})

		project("tacopie")
			kind("StaticLib")
			includedirs(path.join(TACOPIE_FOLDER, "includes"))
			files({
				path.join(TACOPIE_FOLDER, "sources/utils/*.cpp"),
				path.join(TACOPIE_FOLDER, "sources/network/*.cpp"),
				path.join(TACOPIE_FOLDER, "sources/network/common/*.cpp"),
				path.join(TACOPIE_FOLDER, "includes/tacopie/**"),
			})
			vpaths({
				["Source files/*"] = TACOPIE_FOLDER .. "/sources/**.cpp",
				["Header files/*"] = TACOPIE_FOLDER .. "/includes/tacopie/**"
			})

			filter("system:windows")
				files(path.join(TACOPIE_FOLDER, "sources/network/windows/*.cpp"))
			filter("system:not windows")
				files(path.join(TACOPIE_FOLDER, "sources/network/unix/*.cpp"))
			filter({})
