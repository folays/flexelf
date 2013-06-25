NAME	= libpreload-accept.so
CC	= gcc
RM	= rm -f

CFLAGS	= -fPIC

SRC	= flexelf.c demo.c
OBJ	= $(SRC:.c=.o)

all	: $(NAME)

$(NAME)	: $(OBJ)
	$(CC) -o $(NAME) -shared $(OBJ) -lelf

re	: fclean all

clean	:
	-$(RM) $(OBJ) *~

fclean	: clean
	-$(RM) $(NAME)
