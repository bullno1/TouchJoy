solution "touch-joy"
	location(_ACTION)
	configurations {"Develop"}
	platforms {"x64"}
	targetdir "bin"
	debugdir "data"

	project "touch-joy"
		kind "WindowedApp"
		language "C"

		defines {
			"_CRT_SECURE_NO_WARNINGS"
		}

		files {
			"src/*.h",
			"src/*.c"
		}

		flags {
			"ExtraWarnings",
			"FatalWarnings",
			"OptimizeSize",
			"StaticRuntime",
			"Symbols",
			"WinMain",
			"NoEditAndContinue",
			"NoNativeWChar",
			"NoExceptions",
			"NoFramePointer"
		}

	project "test"
		kind "ConsoleApp"
		language "C"

		defines {
			"_CRT_SECURE_NO_WARNINGS",
			"_TEST",
			"GB_INI_MAX_SECTION_LENGTH=16",
			"GB_INI_MAX_NAME_LENGTH=16"
		}

		files {
			"src/*.h",
			"src/*.c"
		}

		flags {
			"FatalWarnings",
			"OptimizeSize",
			"StaticRuntime",
			"Symbols",
			"NoEditAndContinue",
			"NoNativeWChar",
			"NoExceptions"
		}
