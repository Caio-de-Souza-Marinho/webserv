NAME	= webserv
CC	= c++
FLAGS	= -Wall -Wextra -Werror -std=c++98

INCLUDES = -I include

SRC_DIR	= src/
OBJ_DIR	= obj/

TEST_FILE = config/default.conf

SRCS	= ${SRC_DIR}main.cpp \


OBJS	= ${SRCS:${SRC_DIR}%.cpp=${OBJ_DIR}/%.o}

all:		${NAME}

${NAME}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.cpp
	@mkdir -p ${OBJ_DIR}
	${CC} ${FLAGS} ${INCLUDES} -c $< -o $@

clean:
	rm -rf ${OBJ_DIR}

fclean:	clean
	rm -f ${NAME}

re:	fclean all

run:	re
	./${NAME} ${TEST_FILE}

.PHONY: all clean fclean re run
