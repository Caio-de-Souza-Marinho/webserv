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
	  ${SRC_DIR}/Request.cpp \
	  ${SRC_DIR}/RequestParser.cpp \
	  ${SRC_DIR}/Mock-Config.cpp \
	  ${SRC_DIR}/Response.cpp \
	  ${SRC_DIR}/ResponseBuilder.cpp \
	  ${SRC_DIR}/Router.cpp \
	  ${SRC_DIR}/Tester.cpp \
	  ${SRC_DIR}/Router-Tester.cpp

OBJS	= ${SRCS:${SRC_DIR}/%.cpp=${OBJ_DIR}/%.o}

all:		${NAME}

${NAME}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.cpp
	@mkdir -p $(dir $@)
	${CC} ${FLAGS} ${INCLUDES} -c $< -o $@

test: ${NAME}
	valgrind --leak-check=full --show-leak-kinds=all ./${NAME}

clean:
	rm -rf ${OBJ_DIR}

fclean:	clean
	rm -f ${NAME}

re:	fclean all

run:	re
	./${NAME} ${TEST_FILE}

.PHONY:	all clean fclean re run
