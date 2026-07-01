NAME	= webserv
CC	= c++
FLAGS	= -Wall -Wextra -Werror -std=c++98

INCLUDES = -I include

SRC_DIR	= src
OBJ_DIR	= obj

TEST_FILE = config/default.conf

# Colors
RED	= \033[1;31m
GREEN	= \033[1;32m
YELLOW	= \033[1;33m
BLUE	= \033[1;34m
MAG	= \033[1;35m
CYAN	= \033[1;36m
RESET	= \033[0m

SRCS	= ${SRC_DIR}/main.cpp \
	  ${SRC_DIR}/Logger.cpp \
	  ${SRC_DIR}/MimeTypes.cpp \
	  ${SRC_DIR}/ConfigParser.cpp \
	  ${SRC_DIR}/Request.cpp \
	  ${SRC_DIR}/RequestParser.cpp \
	  ${SRC_DIR}/Response.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilder.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderHandlers.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderErrors.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderAutoindex.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderFile.cpp \
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderPath.cpp \
	  ${SRC_DIR}/ResponseBuilder/MultipartParser.cpp \
	  ${SRC_DIR}/Router.cpp \
	  ${SRC_DIR}/WebServer.cpp \
	  ${SRC_DIR}/WebServer-Init.cpp \
	  ${SRC_DIR}/WebServer-CGI.cpp \
	  ${SRC_DIR}/Client.cpp \
	  ${SRC_DIR}/Server.cpp \
	  ${SRC_DIR}/CGIHandler.cpp \
	  ${SRC_DIR}/SessionManager.cpp \

OBJS	= ${SRCS:${SRC_DIR}/%.cpp=${OBJ_DIR}/%.o}

all:		${NAME}

${NAME}:	${OBJS}
	@echo "${CYAN}Compiling source files...${RESET}"
	@${CC} ${FLAGS} ${OBJS} -o ${NAME}
	@echo "${GREEN}Build complete!${RESET}"

${OBJ_DIR}/%.o:	${SRC_DIR}/%.cpp
	@mkdir -p $(dir $@)
	@${CC} ${FLAGS} ${INCLUDES} -c $< -o $@

TEST_DIR = tests/components/

TESTS_SRCS = ${TEST_DIR}/TestRunner.cpp \
	     ${TEST_DIR}/TestUtils.cpp \
	     ${TEST_DIR}/TestRequestParser.cpp \
	     ${TEST_DIR}/TestRouter.cpp \
	     ${TEST_DIR}/TestConfigParser.cpp \
	     ${TEST_DIR}/TestResponseBuilder.cpp \
	     ${TEST_DIR}/TestMultipartParser.cpp \
	     ${TEST_DIR}/TestSessionManager.cpp \
	     ${SRC_DIR}/Request.cpp \
	     ${SRC_DIR}/RequestParser.cpp \
	     ${SRC_DIR}/Logger.cpp \
	     ${SRC_DIR}/Router.cpp \
	     ${SRC_DIR}/ConfigParser.cpp \
	     ${SRC_DIR}/Response.cpp \
	     ${SRC_DIR}/MimeTypes.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilder.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilderHandlers.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilderErrors.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilderAutoindex.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilderFile.cpp \
	     ${SRC_DIR}/ResponseBuilder/ResponseBuilderPath.cpp \
	     ${SRC_DIR}/ResponseBuilder/MultipartParser.cpp \
	     ${SRC_DIR}/SessionManager.cpp \

test: re
	@c++ -Wall -Wextra -Werror -std=c++98 ${TESTS_SRCS} -I include -o my_tester
	./my_tester

clean:
	@echo "${RED}Removing object files...${RESET}"
	@rm -rf ${OBJ_DIR}

fclean:	clean
	@echo "${RED}Removing binary...${RESET}"
	@rm -f ${NAME}

re:	fclean all

run:	re
	./${NAME} 

.PHONY:	all clean fclean re run
