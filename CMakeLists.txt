cmake_minimum_required(VERSION 3.21)

project(
	GT7PS1
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "GT7 PS1 Edition - juego de coches 3D para PlayStation 1"
)

psn00bsdk_add_executable(gt7 GPREL
	src/main.c
	src/game.c
	src/render.c
	src/ui.c
)

# Genera build/gt7.bin + build/gt7.cue
psn00bsdk_add_cd_image(
	iso
	gt7
	iso.xml
	DEPENDS gt7 system.cnf
)
