# Define the source file and executable name
TARGET = life
SOURCE = life.c
CC = gcc

# The first target is the default. It depends on the source file.
$(TARGET): $(SOURCE)
	$(CC) -g $(SOURCE) -o $(TARGET) -lncurses

# A phony target to clean up generated files
.PHONY: clean
clean:
	rm -f $(TARGET)

