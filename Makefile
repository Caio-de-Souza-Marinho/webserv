NAME	= webserv
CC	= c++
FLAGS	= -Wall -Wextra -Werror -std=c++98

INCLUDES = -I include

SRC_DIR	= src
OBJ_DIR	= obj

TEST_FILE = config/default.conf

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
	  ${SRC_DIR}/ResponseBuilder/ResponseBuilderUtils.cpp \
	  ${SRC_DIR}/Router.cpp \
	  ${SRC_DIR}/WebServer.cpp \
	  ${SRC_DIR}/WebServer-Init.cpp \
	  ${SRC_DIR}/WebServer-CGI.cpp \
	  ${SRC_DIR}/Client.cpp \
	  ${SRC_DIR}/Server.cpp \
	  ${SRC_DIR}/CGIHandler.cpp \

OBJS	= ${SRCS:${SRC_DIR}/%.cpp=${OBJ_DIR}/%.o}

all:		${NAME}

${NAME}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.cpp
	@mkdir -p $(dir $@)
	${CC} ${FLAGS} ${INCLUDES} -c $< -o $@

TESTS_SRCS = tests/TestRunner.cpp tests/TestUtils.cpp tests/TestRequestParser.cpp

test:
	c++ -Wall -Wextra -Werror -std=c++98 ${TESTS_SRCS} src/RequestParser.cpp src/Request.cpp src/Logger.cpp -I include -o tester
	./tester

clean:
	rm -rf ${OBJ_DIR}

fclean:	clean
	rm -f ${NAME}

re:	fclean all

run:	re
	./${NAME} ${TEST_FILE}

.PHONY:	all clean fclean re run
