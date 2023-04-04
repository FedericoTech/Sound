cmake ^
	-G "Visual Studio 17 2022" ^
	 -A x64 ^
	-DGLFW_BUILD_DOCS=OFF ^
	-S . ^
	-B build/VS2022