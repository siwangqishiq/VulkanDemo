CC := g++

SRC_DIR = src
BUILD_DIR = build
SHADER_DIR = shaders

GLSL_C = glslangValidator

build_dir:
	mkdir -p ${BUILD_DIR}

${SHADER_DIR}/vert.spv:${SHADER_DIR}/triangle.vert
	${GLSL_C} -V ${SHADER_DIR}/triangle.vert -o ${SHADER_DIR}/vert.spv

${SHADER_DIR}/frag.spv:${SHADER_DIR}/triangle.frag
	${GLSL_C} -V ${SHADER_DIR}/triangle.frag -o ${SHADER_DIR}/frag.spv

compile:build_dir ${SHADER_DIR}/vert.spv ${SHADER_DIR}/frag.spv
	${CC} -c ${SRC_DIR}/main.cpp -o ${BUILD_DIR}/main.o -I ../include/

link:compile
	${CC} ${BUILD_DIR}/*.o -o ${BUILD_DIR}/main.exe -Llib -lglfw3dll -lvulkan-1
	
run:link
	${BUILD_DIR}/main
	
clean:
	rm -f ${SHADER_DIR}/*.spv 
	rm -f ${BUILD_DIR}/*.o 
	rm -f ${BUILD_DIR}/main.exe